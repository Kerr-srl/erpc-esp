/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		tinyproto_transport.cpp
 *
 * \brief		ERPC ESP-IDF Tinyproto transport class - implementation
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#include "tinyproto_transport.hpp"

#include <cassert>

enum event_status {
	EVENT_STATUS_CONNECTED = 1,
	/**
	 * Set on disconnection. Cleared when it has been handled.
	 */
	EVENT_STATUS_DISCONNECTION_EVENT_PENDING = 1 << 1,
	EVENT_STATUS_NEW_FRAME_PENDING = 1 << 2,
};

using namespace erpc::esp;

TinyprotoTransport::TinyprotoTransport(
	void *buffer, size_t buffer_size, write_block_cb_t write_func,
	read_block_cb_t read_func,
	const erpc_esp_transport_tinyproto_config &config)
	: tinyproto_(buffer, buffer_size), write_func_(write_func),
	  read_func_(read_func), config_(config) {
	this->rx_fifo.handle =
		xMessageBufferCreateStatic(sizeof(this->rx_fifo.buffer) - 1,
								   this->rx_fifo.buffer, &this->rx_fifo.buf);

	this->tinyproto_.setConnectEventCallback(TinyprotoTransport::connect_cb);
	this->tinyproto_.setReceiveCallback(TinyprotoTransport::receive_cb);
	this->tinyproto_.setUserData(this);
	this->tinyproto_.setSendTimeout(this->config_.send_timeout);

	// This is the default used by the Python binding
	this->tinyproto_.enableCrc16();

	this->tinyproto_.begin();

	{
		TaskHandle_t ret = nullptr;
		ret = xTaskCreateStatic(this->rx_task, "TinyprotoRx",
								sizeof(this->rx_task_.stack), this,
								this->config_.rx_task_priority,
								this->rx_task_.stack, &this->rx_task_.buffer);
		assert(ret);
		ret = xTaskCreateStatic(this->tx_task, "TinyprotoTx",
								sizeof(this->tx_task_.stack), this,
								this->config_.tx_task_priority,
								this->tx_task_.stack, &this->tx_task_.buffer);
		assert(ret);
	}
	{
		assert(!this->events.handle);
		this->events.handle = xEventGroupCreateStatic(&this->events.buf);
		assert(this->events.handle);
	}
}

erpc_status_t TinyprotoTransport::connect(TickType_t timeout) {
	erpc_status_t status = kErpcStatus_Success;

	auto ret = xEventGroupWaitBits(this->events.handle, EVENT_STATUS_CONNECTED,
								   pdFALSE, pdFALSE, timeout);
	if (ret == pdFALSE) {
		status = kErpcStatus_Timeout;
	}

	return status;
}

void TinyprotoTransport::rx_task(void *user_data) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	while (1) {
		pthis->tinyproto_.run_rx(pthis->read_func_);
		// we don't explicitly yield as we do in tx_task, since we expect the
		// user provided read_func_ to "block for a while".
	}
	vTaskDelete(NULL);
}

void TinyprotoTransport::tx_task(void *user_data) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	while (1) {
		int sent_bytes =
			tiny_fd_run_tx(pthis->tinyproto_.getHandle(), pthis->write_func_);
		if (sent_bytes == 0) {
			// to avoid starving other tasks. In particular to ensure that the
			// watchdog is triggered See
			// https://github.com/espressif/esp-idf/issues/1646
			vTaskDelay(1);
		}
	}
	vTaskDelete(NULL);
}

void TinyprotoTransport::receive_cb(void *user_data, uint8_t addr,
									tinyproto::IPacket &pkt) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	size_t sent =
		xMessageBufferSend(pthis->rx_fifo.handle, pkt.data(), pkt.size(), 0);
	assert(sent == pkt.size());

	xEventGroupSetBits(pthis->events.handle, EVENT_STATUS_NEW_FRAME_PENDING);
}

void TinyprotoTransport::connect_cb(void *user_data, uint8_t addr,
									bool connected) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	if (connected) {
		xEventGroupSetBits(pthis->events.handle, EVENT_STATUS_CONNECTED);
		xEventGroupClearBits(pthis->events.handle,
							 EVENT_STATUS_DISCONNECTION_EVENT_PENDING);
	} else {
		xEventGroupSetBits(pthis->events.handle,
						   EVENT_STATUS_DISCONNECTION_EVENT_PENDING);
		xEventGroupClearBits(pthis->events.handle, EVENT_STATUS_CONNECTED);
	}

	if (pthis->config_.on_connect_status_change_cb) {
		pthis->config_.on_connect_status_change_cb(connected);
	}
}

erpc_status_t TinyprotoTransport::send(MessageBuffer *message) {
	int ret = this->tinyproto_.write((const char *)message->get(),
									 message->getUsed());
	if (ret < 0) {
		return kErpcStatus_SendFailed;
	}
	return kErpcStatus_Success;
}
erpc_status_t TinyprotoTransport::receive(MessageBuffer *message) {
	erpc_status_t status = kErpcStatus_Success;

	if (!xMessageBufferIsEmpty(this->rx_fifo.handle)) {
		/*
		 * EVENT_STATUS_NEW_FRAME_PENDING could be set multiple times.
		 * There could be multiple frames pending...
		 * So, as long as there is a frame, just read it.
		 */
		size_t received = xMessageBufferReceive(
			this->rx_fifo.handle, message->get(), message->getLength(), 0);
		assert(received != 0);
		message->setUsed(received);
	} else if (!(xEventGroupGetBits(this->events.handle) &
				 EVENT_STATUS_CONNECTED)) {
		// not connected yet
		status = kErpcStatus_ConnectionClosed;
	} else {
		/*
		 * No frame yet. Wait to receive. Once received, the bit
		 * EVENT_STATUS_NEW_FRAME_PENDING is cleared.
		 */
		EventBits_t event =
			xEventGroupWaitBits(this->events.handle,
								EVENT_STATUS_DISCONNECTION_EVENT_PENDING |
									EVENT_STATUS_NEW_FRAME_PENDING,
								pdTRUE, pdFALSE, this->config_.receive_timeout);

		bool event_received = false;
		if (event & EVENT_STATUS_NEW_FRAME_PENDING) {
			size_t received = xMessageBufferReceive(
				this->rx_fifo.handle, message->get(), message->getLength(), 0);
			assert(received != 0);
			message->setUsed(received);

			event_received = true;
		}
		if (event & EVENT_STATUS_DISCONNECTION_EVENT_PENDING) {
			xMessageBufferReset(this->rx_fifo.handle);
			/*
			 * Disconnected. Why do we return kErpcStatus_Timeout instead of
			 * kErpcStatus_ConnectionClosed?
			 * Well, eRPC unblocks clients only when the error is timeout...
			 * See
			 * https://github.com/EmbeddedRPC/erpc/blob/cd8ffc8c7f08cb6fb123b86422ecbb738a26a69c/erpc_c/infra/erpc_transport_arbitrator.cpp#L79-L90
			 */
			status = kErpcStatus_Timeout;

			event_received = true;
		}

		if (!event_received) {
			// None of the events we waited for is set. xEventGroupWaitBits
			// timed out
			status = kErpcStatus_Timeout;
		}
	}

	return status;
}

bool TinyprotoTransport::hasMessage(void) {
	return !xMessageBufferIsEmpty(this->rx_fifo.handle);
}
