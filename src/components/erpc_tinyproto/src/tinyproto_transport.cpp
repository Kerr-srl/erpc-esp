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

erpc_status_t TinyprotoTransport::connect(TickType_t timeout_ticks) {
	erpc_status_t status = kErpcStatus_Success;

	this->tinyproto_.setReceiveCallback(TinyprotoTransport::receive_cb);
	this->tinyproto_.setSendCallback(TinyprotoTransport::sent_cb);
	this->tinyproto_.setUserData(this);

	// This is the default used by the Python binding
	this->tinyproto_.enableCrc16();

	this->tinyproto_.begin();
	xTaskCreateStatic(this->rx_task, "TinyprotoRx",
					  sizeof(this->rx_task_.stack), this, 1,
					  this->rx_task_.stack, &this->rx_task_.buffer);
	xTaskCreateStatic(this->tx_task, "TinyprotoTx",
					  sizeof(this->tx_task_.stack), this, 1,
					  this->tx_task_.stack, &this->tx_task_.buffer);

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
		// to avoid starving other tasks
		vTaskDelay(1);
	}
	vTaskDelete(NULL);
}

void TinyprotoTransport::tx_task(void *user_data) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	while (1) {
		pthis->tinyproto_.run_tx(pthis->write_func_);
		// to avoid starving other tasks
		vTaskDelay(1);
	}
	vTaskDelete(NULL);
}

void TinyprotoTransport::receive_cb(void *user_data, tinyproto::IPacket &pkt) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	size_t sent =
		xStreamBufferSend(pthis->rx_fifo.handle, pkt.data(), pkt.size(), 0);

	assert(sent == pkt.size());
}

void TinyprotoTransport::sent_cb(void *user_data, tinyproto::IPacket &pkt) {
	TinyprotoTransport *pthis = static_cast<TinyprotoTransport *>(user_data);
	xSemaphoreGive(pthis->sent_semaphore.handle);
}

erpc_status_t TinyprotoTransport::underlyingSend(const uint8_t *data,
												 uint32_t size) {
	int ret = this->tinyproto_.write((const char *)data, size);
	if (ret < 0) {
		return kErpcStatus_SendFailed;
	}
	if (xSemaphoreTake(this->sent_semaphore.handle, this->send_timeout_) !=
		pdTRUE) {
		return kErpcStatus_Timeout;
	}
	return kErpcStatus_Success;
}
erpc_status_t TinyprotoTransport::underlyingReceive(uint8_t *data,
													uint32_t size) {
	xStreamBufferSetTriggerLevel(this->rx_fifo.handle, size);
	size_t received = xStreamBufferReceive(this->rx_fifo.handle, data, size,
										   this->receive_timeout_);
	if (received == size) {
		return kErpcStatus_Success;
	}
	return kErpcStatus_Timeout;
}
