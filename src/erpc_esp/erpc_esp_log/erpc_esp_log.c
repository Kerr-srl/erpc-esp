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

#include <assert.h>
#include <stdio.h>

void erpc_esp_log_transport_send(const uint8_t *data, uint32_t size,
								 char *buffer) {
	for (size_t i = 0; i < size; ++i) {
		sprintf(buffer + (i * 2), "%02x", data[i]);
	}
	buffer[(size * 2)] = '\0';
	ESP_LOGI(TAG, "[%s]", buffer);
}
