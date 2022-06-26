#include "erpc_esp/utils.h"

#include "freertos/task.h"

void erpc_esp_freertos_critical_enter(
	erpc_esp_freertos_critical_section_lock *handle) {
#if CONFIG_IDF_TARGET_LINUX
	taskENTER_CRITICAL();
#else
	taskENTER_CRITICAL(&handle->lock);
#endif
}
void erpc_esp_freertos_critical_exit(
	erpc_esp_freertos_critical_section_lock *handle) {
#if CONFIG_IDF_TARGET_LINUX
	taskEXIT_CRITICAL();
#else
	taskEXIT_CRITICAL(&handle->lock);
#endif
}
