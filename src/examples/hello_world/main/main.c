#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"

#include "erpc_esp_uart_transport_setup.h"

#if HOST
#include "gen/c_hello_world_host_server.h"
#include "gen/c_hello_world_target_client.h"
#else
#include "gen/c_hello_world_host_client.h"
#include "gen/c_hello_world_target_server.h"
#endif

#include "esp_log.h"
#define TAG "main"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include <inttypes.h>
#include <string.h>

#if HOST
char *say_hello_to_host(const char *name, uint32_t ith) {
	size_t len = strlen("Hello ") + strlen(name) + 1;
	char *response = erpc_malloc(len);
	memset(response, 0, len);
	strcat(response, "Hello ");
	strcat(response, name);
	ESP_LOGI(TAG,
			 "Received [%" PRIu32 "]call from target. Responding with \"%s\"",
			 ith, response);
	return response;
}
#else
char *say_hello_to_target(const char *name, uint32_t ith) {
	size_t len = strlen("Hello ") + strlen(name) + 1;
	char *response = erpc_malloc(len);
	memset(response, 0, len);
	strcat(response, "Hello ");
	strcat(response, name);
	ESP_LOGI(TAG,
			 "Received [%" PRIu32 "]th call from host. Responding with \"%s\"",
			 ith, response);
	return response;
}
#endif

void server_task(void *params) {
	erpc_server_t server = (erpc_server_t)params;
	ESP_LOGI(TAG, "Starting server");

	erpc_status_t status = erpc_server_run(server);
	ESP_LOGW(TAG, "Server terminated with status: %d", status);
	vTaskDelete(NULL);
}

void client_task(void *params) {
	uint32_t i = 0;
	vTaskDelay(pdMS_TO_TICKS(1000));
	while (1) {
#if HOST
		ESP_LOGI(TAG, "Calling target");
		char *response = say_hello_to_target("ESP32 host", i);
		ESP_LOGI(TAG, "Target response: \"%s\"", response);
#else
		ESP_LOGI(TAG, "Calling host");
		char *response = say_hello_to_host("ESP32 target", i);
		ESP_LOGI(TAG, "Host response: \"%s\"", response);
#endif
		erpc_free(response);
		++i;
	}
	vTaskDelete(NULL);
}

#define RPC_UART_PORT ((CONFIG_ESP_CONSOLE_UART_NUM + 1) % UART_NUM_MAX)
// Change these pins as needed to make them work on your hardware
#define RPC_UART_TX GPIO_NUM_19
#define RPC_UART_RX GPIO_NUM_20

void app_main() {
	ESP_LOGD(TAG, "Initializing UART transport");
	erpc_transport_t transport = erpc_esp_transport_uart_init(RPC_UART_PORT);
	ESP_ERROR_CHECK(uart_set_pin(RPC_UART_PORT, RPC_UART_TX, RPC_UART_RX,
								 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();
	erpc_transport_t arbitrator;
	erpc_client_t client = erpc_arbitrated_client_init(
		transport, message_buffer_factory, &arbitrator);
	inithello_world_host_client(client);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_t server = erpc_server_init(arbitrator, message_buffer_factory);
#if HOST
	erpc_add_service_to_server(server, create_hello_world_host_service());
#else
	erpc_add_service_to_server(server, create_hello_world_target_service());
#endif

	xTaskCreate(server_task, "server", 0x1000, server, 3, NULL);
	xTaskCreate(client_task, "client", 0x1000, NULL, 2, NULL);

	while (1) {
		vTaskDelay(portMAX_DELAY);
	}
}
