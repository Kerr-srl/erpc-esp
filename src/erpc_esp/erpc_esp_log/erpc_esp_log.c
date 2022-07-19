/**
 * \file		erpc_esp_log.c
 *
 * \brief		eRPC ESP-IDF log utilities - implementation
 *
 * \copyright	Copyright 2022 Kerr s.r.l. - All Rights Reserved.
 */
#include "erpc_esp_log.h"

/**
 * Side step build time logging level configuration. We always want eRPC output
 * to be logged.
 */
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define TAG "erpc"

#include <assert.h>
#include <stdio.h>

void erpc_esp_log_transport_init(void) {
	esp_log_level_set(TAG, LOG_LOCAL_LEVEL);
}

void erpc_esp_log_transport_send(const uint8_t *data, uint32_t size,
								 char *buffer) {
	for (size_t i = 0; i < size; ++i) {
		sprintf(buffer + (i * 2), "%02x", data[i]);
	}
	buffer[(size * 2)] = '\0';

	/*
	 * We already sidestepped the build time logging level configuration.
	 * But `esp_log_level_set` could still disable the log.
	 * We mitigate the problem by using the highest logging level.
	 */
	ESP_LOGE(TAG, "[%s]", buffer);
}
