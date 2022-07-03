#include "esp_timer.h"

#include <time.h>

int64_t esp_timer_get_time(void) {
	struct timespec current_time;
	int result = clock_gettime(CLOCK_MONOTONIC, &current_time);
	assert(result == 0);
	int64_t us = current_time.tv_sec * 1000000L + current_time.tv_nsec / 1000L;
	return us;
}
