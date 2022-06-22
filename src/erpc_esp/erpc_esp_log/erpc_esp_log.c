/**
 * \file		erpc_esp_log.c
 *
 * \brief		eRPC ESP-IDF log utilities - implementation
 *
 * \copyright	Copyright 2022 Kerr s.r.l. - All Rights Reserved.
 */
#include "erpc_esp_log.h"

#include "esp_log.h"
#define TAG "erpc"

#include "driver/uart.h"
#include "sdkconfig.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void erpc_esp_log_transport_send(const uint8_t *data, uint32_t size,
								 char *buffer) {
	size_t i = 0;
	strcat(buffer, ERPC_ESP_LOG_PREFIX_);
	i += ERPC_ESP_STRLEN_(ERPC_ESP_LOG_PREFIX_);
	for (size_t j = 0; j < size; ++j) {
		sprintf(buffer + i + (j * 2), "%02x", data[j]);
	}
	i += size * 2;
	strcat(buffer + i, ERPC_ESP_LOG_POSTFIX_);
	i += ERPC_ESP_STRLEN_(ERPC_ESP_LOG_POSTFIX_);
	buffer[i] = '\0';
	++i;

	uart_write_bytes(CONFIG_ESP_CONSOLE_UART_NUM, buffer, i);
}
