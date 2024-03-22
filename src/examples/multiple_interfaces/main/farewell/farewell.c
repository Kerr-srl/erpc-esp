#include "gen/c_multi_interface_target_server.h"

#include "erpc_port.h"

#include "esp_log.h"
#define TAG "greet_service"

#include <string.h>

#define BYE "Bye "

char *say_bye_to_target(const char *name) {
	size_t len = strlen(BYE) + strlen(name) + 1;
	char *response = erpc_malloc(len);
	memset(response, 0, len);
	strcat(response, BYE);
	strcat(response, name);
	ESP_LOGI(TAG, "Received call from host. Responding with \"%s\"", response);
	return response;
}
