#include "../esp_log_private.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

static SemaphoreHandle_t mutex;

void esp_log_impl_lock(void) {
	if (!mutex) {
		mutex = xSemaphoreCreateMutex();
	}
	xSemaphoreTake(mutex, portMAX_DELAY);
}

bool esp_log_impl_lock_timeout(void) {
	esp_log_impl_lock();
	return true;
}

void esp_log_impl_unlock(void) {
	xSemaphoreGive(mutex);
}

uint32_t esp_log_timestamp(void) {
	struct timespec current_time;
	int result = clock_gettime(CLOCK_MONOTONIC, &current_time);
	assert(result == 0);
	uint32_t milliseconds =
		current_time.tv_sec * 1000 + current_time.tv_nsec / 1000000;
	return milliseconds;
}
