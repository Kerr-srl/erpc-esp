#include "common.h"

#include "esp_log.h"
#define TAG "erpc"

#include <assert.h>
#include <stdio.h>

/**
 * We must implement this function in a C translation unit because the ESP_LOGI
 * macro is not compilable in C++.
 */
void erpc_esp_log_transport_send(const uint8_t *data, uint32_t size,
								 char *buffer, size_t buffer_len) {
	assert(buffer_len >= REQUIRED_BUFFER_SIZE(size));
	for (size_t i = 0; i < size; i++) {
		sprintf(buffer + (i * 2), "%02x", data[i]);
	}
	buffer[(size * 2)] = '\0';
	ESP_LOGI(TAG, "[%s]", buffer);
}
