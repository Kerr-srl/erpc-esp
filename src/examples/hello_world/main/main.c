#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"

#include "erpc_esp_uart_transport_setup.h"

#include "gen/hello_world_host.h"
#include "gen/hello_world_target_server.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define TAG "main"

#include "sdkconfig.h"

#include <string.h>

char *say_hello_to_target(const char *name) {
	size_t len = strlen("Hello ") + strlen(name) + 1;
	char *response = erpc_malloc(len);
	memset(response, 0, len);
	strcat(response, "Hello ");
	strcat(response, name);
	ESP_LOGI(TAG, "Received call from host. Responding with \"%s\"", response);
	return response;
}

void server_task(void *params) {
	ESP_LOGI(TAG, "Starting server");
	erpc_status_t status = erpc_server_run();
	ESP_LOGW(TAG, "Server terminated with status: %d", status);
	vTaskDelete(NULL);
}

void client_task(void *params) {
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(5000));
		ESP_LOGI(TAG, "Calling host");
		char *response = say_hello_to_host("ESP32");
		ESP_LOGI(TAG, "Host response: \"%s\"", response);
	}
	vTaskDelete(NULL);
}

void app_main() {
	ESP_LOGD(TAG, "Initializing UART transport");
	erpc_transport_t transport =
		erpc_esp_transport_uart_init(CONFIG_ESP_CONSOLE_UART_NUM);
	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();
	erpc_transport_t arbitrator =
		erpc_arbitrated_client_init(transport, message_buffer_factory);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_init(arbitrator, message_buffer_factory);
	erpc_add_service_to_server(create_hello_world_target_service());

	xTaskCreate(server_task, "server", 0x1000, NULL, 2, NULL);
	xTaskCreate(client_task, "client", 0x1000, NULL, 2, NULL);
}
