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
 * \brief		ERPC ESP-IDF UART transport class - implementation
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#include "uart_transport.hpp"

#define TAG "erpc_esp_uart"
#include "esp_log.h"

using namespace erpc::esp;

UARTTransport::UARTTransport(uart_port_t port) : m_port(port) {
}

UARTTransport::~UARTTransport(void) {
	ESP_ERROR_CHECK(uart_driver_delete(this->m_port));
}

erpc_status_t UARTTransport::init(void) {
	erpc_status_t status = kErpcStatus_Success;
	ESP_ERROR_CHECK(
		uart_driver_install(this->m_port, 1000, 1000, 100, NULL, 0));
	return status;
}

erpc_status_t UARTTransport::underlyingSend(const uint8_t *data,
											uint32_t size) {
	int bytes_written = uart_write_bytes(this->m_port, data, size);

	return (size != bytes_written) ? kErpcStatus_SendFailed
								   : kErpcStatus_Success;
}
erpc_status_t UARTTransport::underlyingReceive(uint8_t *data, uint32_t size) {
	int bytes_read = uart_read_bytes(this->m_port, data, size, portMAX_DELAY);

	return (size != bytes_read) ? kErpcStatus_ReceiveFailed
								: kErpcStatus_Success;
}
