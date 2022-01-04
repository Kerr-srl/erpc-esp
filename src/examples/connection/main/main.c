#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"

#include "erpc_esp_tinyproto_transport_setup.h"

#include "gen/hello_world_with_connection_host.h"
#include "gen/hello_world_with_connection_target_server.h"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define TAG "main"

#include "sdkconfig.h"

#include <string.h>

char *say_hello_to_target(const char *name, uint32_t ith) {
	size_t len = strlen("Hello ") + strlen(name) + 1;
	char *response = (char *)(erpc_malloc(len));
	memset(response, 0, len);
	strcat(response, "Hello ");
	strcat(response, name);
	ESP_LOGI(TAG, "Received call (%u) from host. Responding with \"%s\"",
			 (unsigned)ith, response);
	return response;
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
		ESP_LOGI(TAG, "Calling host");
		char *response = say_hello_to_host("ESP32", i);
		ESP_LOGI(TAG, "Host response: \"%s\"", response);
		++i;
	}
	vTaskDelete(NULL);
}

static void client_error(erpc_status_t err, uint32_t functionID) {
	if (err != kErpcStatus_Success) {
		ESP_LOGE(TAG, "Client error %d, functionID: %u", err,
				 (unsigned)functionID);
	}
}

static uint8_t buffer[1000];

static int tinyproto_write_fn(void *pdata, const void *buffer, int size) {
	return uart_write_bytes(CONFIG_ESP_CONSOLE_UART_NUM, buffer, size);
}
static int tinyproto_read_fn(void *pdata, void *buffer, int size) {
	return uart_read_bytes(CONFIG_ESP_CONSOLE_UART_NUM, buffer, size,
						   pdMS_TO_TICKS(5));
}

static struct erpc_esp_transport_tinyproto_config tinyproto_config =
	ERPC_ESP_TRANSPORT_TINYPROTO_CONFIG_DEFAULT();

void app_main() {
	ESP_LOGD(TAG, "Initializing UART transport");

	ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 1000, 1000,
										100, NULL, 0));

	tinyproto_config.send_timeout = pdMS_TO_TICKS(10000);
	tinyproto_config.receive_timeout = pdMS_TO_TICKS(10000);
	erpc_transport_t transport = erpc_esp_transport_tinyproto_init(
		buffer, sizeof(buffer), tinyproto_write_fn, tinyproto_read_fn,
		&tinyproto_config);

	ESP_LOGI(TAG, "Connection established");
	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();
	erpc_transport_t arbitrator =
		erpc_arbitrated_client_init(transport, message_buffer_factory);
	erpc_arbitrated_client_set_error_handler(client_error);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_init(arbitrator, message_buffer_factory);
	erpc_add_service_to_server(create_hello_world_target_service());

	xTaskCreate(server_task, "server", 0x1000, NULL, 1, NULL);
	xTaskCreate(client_task, "client", 0x1000, NULL, 1, NULL);
}
