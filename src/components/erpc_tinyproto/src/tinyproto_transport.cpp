/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		uart_transport.cpp
 *
 * \brief		ERPC ESP-IDF Tinyproto transport class - implementation
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#include "tinyproto_transport.hpp"

#include <cassert>

using namespace erpc::esp;

erpc_status_t TinyprotoTransport::connect() {
	erpc_status_t status = kErpcStatus_Success;

	this->tinyproto_.setReceiveCallback(TinyprotoTransport::receive_cb);
	this->tinyproto_.setUserData(this);
	this->tinyproto_.setSendTimeout(this->config_.send_timeout);

	// This is the default used by the Python binding
	this->tinyproto_.enableCrc16();

	this->tinyproto_.begin();
	xTaskCreateStatic(this->rx_task, "TinyprotoRx",
					  sizeof(this->rx_task_.stack), this, this->config_.rx_task_priority,
					  this->rx_task_.stack, &this->rx_task_.buffer);
	xTaskCreateStatic(this->tx_task, "TinyprotoTx",
					  sizeof(this->tx_task_.stack), this, this->config_.tx_task_priority,
					  this->tx_task_.stack, &this->tx_task_.buffer);

	TickType_t timeout_ticks = this->config_.connect_timeout;
	TickType_t polling_rate = pdMS_TO_TICKS(10);
	while (tiny_fd_get_status(this->tinyproto_.getHandle()) != 0) {
		vTaskDelay(polling_rate);
		if (timeout_ticks <= polling_rate) {
			status = kErpcStatus_Timeout;
			break;
		}
		timeout_ticks -= polling_rate;
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

void TinyprotoTransport::receive_cb(void *user_data, tinyproto::IPacket &pkt) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	size_t sent =
		xMessageBufferSend(pthis->rx_fifo.handle, pkt.data(), pkt.size(), 0);

	assert(sent == pkt.size());
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
	size_t received =
		xMessageBufferReceive(this->rx_fifo.handle, message->get(),
							  message->getLength(), this->config_.receive_timeout);
	if (received != 0) {
		message->setUsed(received);
		return kErpcStatus_Success;
	}
	return kErpcStatus_Timeout;
}

bool TinyprotoTransport::hasMessage(void) {
	return !xMessageBufferIsEmpty(this->rx_fifo.handle);
}
