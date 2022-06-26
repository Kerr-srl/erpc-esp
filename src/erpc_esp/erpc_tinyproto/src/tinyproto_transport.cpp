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
 * \copyright	Copyright 2022 Kerr s.r.l. - All Rights Reserved.
 */

#include "tinyproto_transport.hpp"

#include "erpc_esp/utils.h"

#include <cassert>

enum event_status {
	/**
	 * Set on TinyprotoTransport::open.
	 * Cleared in TinyprotoTransport::close.
	 * When cleared, TinyprotoTransport::rx_task and TinyprotoTransport::tx_task
	 * will terminate themselves.
	 */
	EVENT_STATUS_OPENED = 1,
	/**
	 * Set on open => closed transition.
	 * Cleared in TinyprotoTransport::open.
	 * Mainly used to give blocking functions a way to unblock themselves
	 * immediately on closure.
	 */
	EVENT_STATUS_CLOSED = 1 << 1,
	/**
	 * Set when the connection is established and cleared when the connection is
	 * dropped.
	 */
	EVENT_STATUS_CONNECTED = 1 << 2,
	/**
	 * Set on connected => disconnected transition.
	 * Cleared when a new connection is established.
	 * Mainly used to give blocking functions a way to unblock themselves
	 * immediately on disconnection.
	 */
	EVENT_STATUS_DISCONNECTED = 1 << 3,
	/**
	 * Event bit for TinyprotoTransport::receive, which may run in any user
	 * thread.
	 * Set when a new frame has been received and enqueued in the
	 * TinyprotoTransport::rx_fifo
	 * Cleared by TinyprotoTransport::receive when the new frame is read from
	 * the message buffer.
	 */
	EVENT_STATUS_NEW_FRAME_PENDING = 1 << 4,
	/**
	 * Cleared on TinyprotoTransport::open
	 * Set by TX thread just before its termination.
	 */
	EVENT_STATUS_TX_THREAD_CLOSED = 1 << 5,
	/**
	 * Cleared on TinyprotoTransport::open
	 * Set by RX thread just before its termination.
	 */
	EVENT_STATUS_RX_THREAD_CLOSED = 1 << 6,
};

using namespace erpc::esp;

TinyprotoTransport::TinyprotoTransport(
	void *buffer, size_t buffer_size, write_block_cb_t write_func,
	read_block_cb_t read_func,
	const erpc_esp_transport_tinyproto_config &config)
	: tinyproto_(buffer, buffer_size), write_func_(write_func),
	  read_func_(read_func), config_(config) {
	this->rx_fifo_.handle =
		xMessageBufferCreateStatic(sizeof(this->rx_fifo_.buffer) - 1,
								   this->rx_fifo_.buffer, &this->rx_fifo_.buf);

	this->tinyproto_.setConnectEventCallback(TinyprotoTransport::connect_cb);
	this->tinyproto_.setReceiveCallback(TinyprotoTransport::receive_cb);
	this->tinyproto_.setUserData(this);
	this->tinyproto_.setSendTimeout(this->config_.send_timeout);

	// This is the default used by the Python binding
	this->tinyproto_.enableCrc16();

	{
		assert(!this->events_.handle);
		this->events_.handle = xEventGroupCreateStatic(&this->events_.buf);
		assert(this->events_.handle);
	}
}

void TinyprotoTransport::open() {
	assert(!(xEventGroupGetBits(this->events_.handle) & EVENT_STATUS_OPENED));

	xEventGroupClearBits(this->events_.handle,
						 EVENT_STATUS_CLOSED | EVENT_STATUS_RX_THREAD_CLOSED |
							 EVENT_STATUS_TX_THREAD_CLOSED);
	xEventGroupSetBits(this->events_.handle, EVENT_STATUS_OPENED);

	this->tinyproto_.begin();
	{
		TaskHandle_t ret = nullptr;
		ret = xTaskCreateStatic(this->rx_task, "TinyprotoRx",
								sizeof(this->rx_task_.stack) /
									sizeof(this->rx_task_.stack[0]),
								this, this->config_.rx_task_priority,
								this->rx_task_.stack, &this->rx_task_.buffer);
		assert(ret);
		ret = xTaskCreateStatic(this->tx_task, "TinyprotoTx",
								sizeof(this->tx_task_.stack) /
									sizeof(this->tx_task_.stack[0]),
								this, this->config_.tx_task_priority,
								this->tx_task_.stack, &this->tx_task_.buffer);
		assert(ret);
	}
}
void TinyprotoTransport::close() {
	assert(xEventGroupGetBits(this->events_.handle) & EVENT_STATUS_OPENED);

	xEventGroupClearBits(this->events_.handle, EVENT_STATUS_OPENED);
	xEventGroupSetBits(this->events_.handle, EVENT_STATUS_CLOSED);
	// wait until both tx and rx thread have gracefully terminated
	xEventGroupWaitBits(this->events_.handle,
						EVENT_STATUS_RX_THREAD_CLOSED |
							EVENT_STATUS_TX_THREAD_CLOSED,
						pdFALSE, pdFALSE, portMAX_DELAY);
	this->tinyproto_.end();
}

erpc_status_t TinyprotoTransport::wait_connected(TickType_t timeout) {
	erpc_status_t status = kErpcStatus_Success;
	assert(xEventGroupGetBits(this->events_.handle) & EVENT_STATUS_OPENED);

	EventBits_t event_bits = xEventGroupWaitBits(
		this->events_.handle, EVENT_STATUS_CONNECTED | EVENT_STATUS_CLOSED,
		pdFALSE, pdFALSE, timeout);

	if (event_bits & EVENT_STATUS_CLOSED) {
		status = kErpcStatus_ConnectionClosed;
	} else if (event_bits & EVENT_STATUS_CONNECTED) {
		status = kErpcStatus_Success;
	} else {
		status = kErpcStatus_Timeout;
	}

	return status;
}

void TinyprotoTransport::rx_task(void *user_data) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	while (xEventGroupGetBits(pthis->events_.handle) & EVENT_STATUS_OPENED) {
		pthis->tinyproto_.run_rx(pthis->read_func_);
		// we don't explicitly yield as we do in tx_task, since we expect the
		// user provided read_func_ to "block for a while".
	}

	xEventGroupSetBits(pthis->events_.handle, EVENT_STATUS_RX_THREAD_CLOSED);
	vTaskDelete(NULL);
}

void TinyprotoTransport::tx_task(void *user_data) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	while (xEventGroupGetBits(pthis->events_.handle) & EVENT_STATUS_OPENED) {
		int sent_bytes =
			tiny_fd_run_tx(pthis->tinyproto_.getHandle(), pthis->write_func_);
		if (sent_bytes == 0) {
			// to avoid starving other tasks. In particular to ensure that the
			// watchdog is triggered See
			// https://github.com/espressif/esp-idf/issues/1646
			vTaskDelay(1);
		}
	}

	xEventGroupSetBits(pthis->events_.handle, EVENT_STATUS_TX_THREAD_CLOSED);
	vTaskDelete(NULL);
}

static erpc_esp_freertos_critical_section_lock lock =
	ERPC_ESP_FREERTOS_CRITICAL_SECTION_LOCK_INIT;

void TinyprotoTransport::receive_cb(void *user_data, uint8_t addr,
									tinyproto::IPacket &pkt) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	erpc_esp_freertos_critical_enter(&lock);
	size_t sent =
		xMessageBufferSend(pthis->rx_fifo_.handle, pkt.data(), pkt.size(), 0);
	erpc_esp_freertos_critical_exit(&lock);
	assert(sent == pkt.size());

	xEventGroupSetBits(pthis->events_.handle, EVENT_STATUS_NEW_FRAME_PENDING);
}

void TinyprotoTransport::connect_cb(void *user_data, uint8_t addr,
									bool connected) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	if (connected) {
		xEventGroupSetBits(pthis->events_.handle, EVENT_STATUS_CONNECTED);
		xEventGroupClearBits(pthis->events_.handle, EVENT_STATUS_DISCONNECTED);
	} else {
		xEventGroupSetBits(pthis->events_.handle, EVENT_STATUS_DISCONNECTED);
		xEventGroupClearBits(pthis->events_.handle, EVENT_STATUS_CONNECTED);
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
	assert(xEventGroupGetBits(this->events_.handle) & EVENT_STATUS_OPENED);

	if (!(xEventGroupGetBits(this->events_.handle) &
		  (EVENT_STATUS_CONNECTED))) {
		xMessageBufferReset(this->rx_fifo_.handle);
		// not connected yet
		return kErpcStatus_ConnectionClosed;
	}

	/*
	 * NOTE: for some reason access to the message buffer must be protected
	 * in critical section.
	 * Otherwise sometimes even thought xMessageBufferIsEmpty returns false
	 * the subsequent call to xMessageBufferReceive returns 0 (i.e. zero bytes
	 * read from the message queue).
	 * I was able to hit the problem using the esp_log example in this
	 * repository, which performs a massive number of eRPC communication.
	 * So the idea is that we can hit the problem we a lot of data is passing
	 * through the MessageBuffer.
	 *
	 * Since xMessageBuffer is by contract single producer single
	 * consumer, if this `receive` function were called concurrently in multiple
	 * tasks, then the necessity of using critical sections would make sense,
	 * as suggested in the [MessageBuffer
	 * documentation](https://www.freertos.org/RTOS-message-buffer-example.html).
	 * But I have checked and this "receive" function doesn't seem to be called
	 * concurrently...
	 *
	 * Maybe it's this bug? https://github.com/aws/amazon-freertos/issues/1837
	 * I tried to apply the proposed workaround, but it doesn't seem to work.
	 */
	erpc_esp_freertos_critical_enter(&lock);
	if (!xMessageBufferIsEmpty(this->rx_fifo_.handle)) {
		/*
		 * EVENT_STATUS_NEW_FRAME_PENDING could be set multiple times.
		 * There could be multiple frames pending.
		 * So, as long the message buffer is not empty, we simply read it
		 * and don't wait for the EVENT_STATUS_NEW_FRAME_PENDING event.
		 */
		size_t received = xMessageBufferReceive(
			this->rx_fifo_.handle, message->get(), message->getLength(), 0);
		erpc_esp_freertos_critical_exit(&lock);
		assert(received != 0);
		message->setUsed(received);
		xEventGroupClearBits(this->events_.handle,
							 EVENT_STATUS_NEW_FRAME_PENDING);
		return kErpcStatus_Success;
	} else {
		erpc_esp_freertos_critical_exit(&lock);
	}

	{
		erpc_status_t status = kErpcStatus_Success;
		/*
		 * No frame yet. Wait to receive.
		 */
		EventBits_t event = xEventGroupWaitBits(
			this->events_.handle,
			EVENT_STATUS_NEW_FRAME_PENDING | EVENT_STATUS_DISCONNECTED |
				EVENT_STATUS_CLOSED,
			pdFALSE, pdFALSE, this->config_.receive_timeout);

		bool event_received = false;
		if (event & EVENT_STATUS_NEW_FRAME_PENDING) {
			/*
			 * Is it possible to lose events due to clearing this bit at
			 * this stage, instead of passing pdTRUE to the xClearOnExit
			 * parameter of xEventGroupWaitBits? Yes. But, as commented above,
			 * if multiple frames are received quickly,
			 * EVENT_STATUS_NEW_FRAME_PENDING may be set multiple times, in
			 * which case we also lose events.
			 * That's why we also initially check whether the message buffer
			 * is empty or not and if not read what's left in there.
			 * We don't rely on EVENT_STATUS_NEW_FRAME_PENDING as the *only*
			 * "trigger" to read from the message buffer, so we don't risk
			 * leaving unread data in the message buffer.
			 * OTOH not using xClearOnExit makes things easier. In fact
			 * FreeRTOS doesn't allow to clear *only some of the bits we are
			 * waiting for* on exit, which means that we can't wait for other
			 * "shared" event bits (e.g. EVENT_STATUS_CLOSED) while we're
			 * waiting for EVENT_STATUS_NEW_FRAME_PENDING if we wanted to clear
			 * it with xClearOnExit, because also these would be cleared if set
			 * while we're waiting, which is not what we want.
			 */
			xEventGroupClearBits(this->events_.handle,
								 EVENT_STATUS_NEW_FRAME_PENDING);
			erpc_esp_freertos_critical_enter(&lock);
			if (!xMessageBufferIsEmpty(this->rx_fifo_.handle)) {
				size_t received =
					xMessageBufferReceive(this->rx_fifo_.handle, message->get(),
										  message->getLength(), 0);
				erpc_esp_freertos_critical_exit(&lock);
				assert(received != 0);
				message->setUsed(received);
			} else {
				erpc_esp_freertos_critical_exit(&lock);
			}

			event_received = true;
		}
		if (event & (EVENT_STATUS_CLOSED | EVENT_STATUS_DISCONNECTED)) {
			xMessageBufferReset(this->rx_fifo_.handle);
			/*
			 * Disconnected or closed. Why do we return kErpcStatus_Timeout
			 * instead of kErpcStatus_ConnectionClosed? Well, eRPC unblocks
			 * clients only when the error is timeout... See
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
		return status;
	}
}

bool TinyprotoTransport::hasMessage(void) {
	return !xMessageBufferIsEmpty(this->rx_fifo_.handle);
}
