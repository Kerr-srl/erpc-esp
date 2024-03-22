#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"

#include "erpc_esp_uart_transport_setup.h"

#include "gen/c_multi_interface_host_client.h"
#include "gen/c_multi_interface_target_server.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define TAG "main"

#include "sdkconfig.h"

#include <string.h>

void server_task(void *params) {
	erpc_server_t server = (erpc_server_t)params;

	ESP_LOGI(TAG, "Starting server");
	erpc_status_t status = erpc_server_run(server);
	ESP_LOGW(TAG, "Server terminated with status: %d", status);
	vTaskDelete(NULL);
}

void client_task(void *params) {
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(5000));

		{
			ESP_LOGI(TAG, "Calling host");
			char *response = say_hello_to_host("ESP32");
			ESP_LOGI(TAG, "Host response: \"%s\"", response);
			erpc_free(response);
		}

		vTaskDelay(pdMS_TO_TICKS(1000));

		{
			ESP_LOGI(TAG, "Calling host");
			char *response = say_bye_to_host("ESP32");
			ESP_LOGI(TAG, "Host response: \"%s\"", response);
			erpc_free(response);
		}
	}
	vTaskDelete(NULL);
}

void app_main() {
	ESP_LOGD(TAG, "Initializing UART transport");
	erpc_transport_t transport =
		erpc_esp_transport_uart_init(CONFIG_ESP_CONSOLE_UART_NUM);
	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();

	erpc_transport_t arbitrator;
	erpc_client_t client = erpc_arbitrated_client_init(
		transport, message_buffer_factory, &arbitrator);
	initgreet_host_client(client);
	initfarewell_host_client(client);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_t server = erpc_server_init(arbitrator, message_buffer_factory);
	erpc_add_service_to_server(server, create_greet_target_service());
	erpc_add_service_to_server(server, create_farewell_target_service());

	xTaskCreate(server_task, "server", 0x1000, server, 2, NULL);
	xTaskCreate(client_task, "client", 0x1000, NULL, 2, NULL);
}
