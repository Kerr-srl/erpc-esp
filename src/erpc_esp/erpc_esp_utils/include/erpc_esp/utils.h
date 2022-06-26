#ifndef ERPC_ESP_UTILS_H_
#define ERPC_ESP_UTILS_H_

#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
#if CONFIG_IDF_TARGET_LINUX
	char dummy;
#else
	portMUX_TYPE lock;
#endif
} erpc_esp_freertos_critical_section_lock;

#if CONFIG_IDF_TARGET_LINUX
#define ERPC_ESP_FREERTOS_CRITICAL_SECTION_LOCK_INIT                           \
	{}
#else
#define ERPC_ESP_FREERTOS_CRITICAL_SECTION_LOCK_INIT                           \
	{ .lock = portMUX_INITIALIZER_UNLOCKED, }
#endif

void erpc_esp_freertos_critical_enter(
	erpc_esp_freertos_critical_section_lock *handle);
void erpc_esp_freertos_critical_exit(
	erpc_esp_freertos_critical_section_lock *handle);

#ifdef __cplusplus
}
#endif

#endif /* ifndef ERPC_ESP_UTILS_H_ */
