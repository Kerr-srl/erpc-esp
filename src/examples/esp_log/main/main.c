#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"

#include "erpc_esp_log_transport_setup.h"

#include "gen/hello_world_host.h"
#include "gen/hello_world_target_server.h"

#include "esp_log.h"
#define TAG "main"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include <string.h>

char *say_hello_to_target(const char *name, uint32_t ith) {
	size_t len = strlen("Hello ") + strlen(name) + 1;
	char *response = erpc_malloc(len);
	memset(response, 0, len);
	strcat(response, "Hello ");
	strcat(response, name);
	ESP_LOGI(TAG, "Received call from host. Responding with \"%s\"", response);
	return response;
}

static xSemaphoreHandle g_connected_sem;

void connect_ack(void) {
	ESP_LOGI(TAG, "Received connect ACK");
	xSemaphoreGive(g_connected_sem);
}

void server_task(void *params) {
	ESP_LOGI(TAG, "Starting server");
	erpc_status_t status = erpc_server_run();
	ESP_LOGW(TAG, "Server terminated with status: %d", status);
	vTaskDelete(NULL);
}

void client_task(void *params) {
	uint32_t i = 0;
	while (1) {
		ESP_LOGI(TAG, "Trying to connect");
		connect();
		BaseType_t taken = xSemaphoreTake(g_connected_sem, pdMS_TO_TICKS(100));
		if (taken) {
			break;
		}
	}
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(1000));
		ESP_LOGI(TAG, "Calling host");
		char *response = say_hello_to_host("ESP32 target", i);
		ESP_LOGI(TAG, "Host response: \"%s\"", response);
		++i;
	}
	vTaskDelete(NULL);
}

void app_main() {
	g_connected_sem = xSemaphoreCreateBinary();
	assert(g_connected_sem);

	ESP_LOGD(TAG, "Initializing esp log transport");
	struct erpc_esp_log_transport_config config = {
		.rx_timeout_ms = 100,
	};
	erpc_transport_t transport = erpc_esp_log_transport_init(&config);
	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();
	erpc_transport_t arbitrator =
		erpc_arbitrated_client_init(transport, message_buffer_factory);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_init(arbitrator, message_buffer_factory);
	erpc_add_service_to_server(create_hello_world_target_service());

	xTaskCreate(server_task, "server", 0x1000, NULL, 2, NULL);
	xTaskCreate(client_task, "client", 0x1000, NULL, 2, NULL);
}
