/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		erpc_esp_log_transport.cpp
 *
 * \brief		ERPC ESP-IDF logging transport class - implementation
 *
 * \copyright	Copyright 2022 Kerr s.r.l. - All Rights Reserved.
 */

#include "erpc_esp_log_transport.hpp"
#include "common.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"

#include <cassert>
#include <cstdio>

using namespace erpc::esp;

EspLogTransport::EspLogTransport(const erpc_esp_log_transport_config &config)
	: config_(config) {
	ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 1000, 1000,
										100, NULL, 0));
}

EspLogTransport::~EspLogTransport() {
	ESP_ERROR_CHECK(uart_driver_delete(CONFIG_ESP_CONSOLE_UART_NUM));
}

erpc_status_t EspLogTransport::underlyingSend(const uint8_t *data,
											  uint32_t size) {
	if (this->config_.tx_buffer.buffer) {
		erpc_esp_log_transport_send(data, size, this->config_.tx_buffer.buffer,
									this->config_.tx_buffer.len);
	} else {
		char buffer[REQUIRED_BUFFER_SIZE(size)];
		erpc_esp_log_transport_send(data, size, buffer, sizeof(buffer));
	}
	return kErpcStatus_Success;
}
erpc_status_t EspLogTransport::underlyingReceive(uint8_t *data, uint32_t size) {
	assert(size > 0);
	int bytes_read = 0;
	if (uart_read_bytes(CONFIG_ESP_CONSOLE_UART_NUM, data, 1, portMAX_DELAY) ==
		1) {
		bytes_read = 1;
		/*
		 * received first byte, the remaining bytes should be received within
		 * the configured timeout
		 */
		if (size > 1) {
			bytes_read += uart_read_bytes(
				CONFIG_ESP_CONSOLE_UART_NUM, data + 1, size - 1,
				this->config_.rx_timeout_ms / portTICK_PERIOD_MS);
		}
	}

	return (size != bytes_read) ? kErpcStatus_ReceiveFailed
								: kErpcStatus_Success;
}
