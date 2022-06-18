#include "erpc_esp_log.h"

#include "erpc_esp_tinyproto_transport_setup.h"

#include "gen/hello_world_with_esp_log_host.h"
#include "gen/hello_world_with_esp_log_target_server.h"

#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_log.h"
#define TAG "main"

#include "sdkconfig.h"

#include <string.h>

#define TX_TASK_PRIORITY 1
#define RX_TASK_PRIORITY 2

#define CLIENT_TASK_PRIORITY 2
#define SERVER_TASK_PRIORITY 3

#define CONNECTED_BIT BIT0

void say_hello_to_target(const char *name, uint32_t ith) {
	ESP_LOGI(TAG, "Received [%u]th call from host.", ith);
}

static EventGroupHandle_t g_event_flag;

void server_task(void *params) {
	ESP_LOGI(TAG, "Starting server");
	while (xEventGroupWaitBits(g_event_flag, CONNECTED_BIT, pdFALSE, pdTRUE,
							   portMAX_DELAY)) {
		erpc_status_t status = erpc_server_run();
		ESP_LOGW(TAG, "Server terminated with status: %d. Re-starting server",
				 status);
	}
	vTaskDelete(NULL);
}

void client_task(void *params) {
	uint32_t i = 0;
	while (xEventGroupWaitBits(g_event_flag, CONNECTED_BIT, pdFALSE, pdTRUE,
							   portMAX_DELAY)) {
		ESP_LOGI(TAG, "Calling host the [%u]th time", i);
		say_hello_to_host("ESP32 target", i);
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

static void on_tinyproto_connect_status_change(bool connected) {
	if (connected) {
		ESP_LOGI(TAG, "Tinyproto connected");
		xEventGroupSetBits(g_event_flag, CONNECTED_BIT);
	} else {
		ESP_LOGW(TAG, "Tinyproto disconnected");
		xEventGroupClearBits(g_event_flag, CONNECTED_BIT);
	}
}

static int tinyproto_write_fn(void *pdata, const void *buffer, int size) {
	static char esp_log_send_buffer[1024];
	assert(sizeof(esp_log_send_buffer) >=
		   ERPC_ESP_LOG_REQUIRED_BUFFER_SIZE(size));
	erpc_esp_log_transport_send(buffer, size, esp_log_send_buffer);
	return size;
}
static int tinyproto_read_fn(void *pdata, void *buffer, int size) {
	return uart_read_bytes(CONFIG_ESP_CONSOLE_UART_NUM, buffer, size,
						   pdMS_TO_TICKS(5));
}

static struct erpc_esp_transport_tinyproto_config tinyproto_config =
	ERPC_ESP_TRANSPORT_TINYPROTO_CONFIG_DEFAULT();

static uint8_t g_tinyproto_rx_buffer[1024];

void app_main() {
	ESP_LOGI(TAG, "Target started");

	g_event_flag = xEventGroupCreate();
	assert(g_event_flag);

	ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 1000, 1000,
										100, NULL, 0));

	tinyproto_config.rx_task_priority = RX_TASK_PRIORITY;
	tinyproto_config.tx_task_priority = TX_TASK_PRIORITY;
	tinyproto_config.send_timeout = pdMS_TO_TICKS(5000);
	tinyproto_config.receive_timeout = pdMS_TO_TICKS(5000);
	tinyproto_config.on_connect_status_change_cb =
		on_tinyproto_connect_status_change;
	erpc_transport_t transport = erpc_esp_transport_tinyproto_init(
		g_tinyproto_rx_buffer, sizeof(g_tinyproto_rx_buffer),
		tinyproto_write_fn, tinyproto_read_fn, &tinyproto_config);

	erpc_esp_transport_tinyproto_connect(portMAX_DELAY);
	ESP_LOGI(TAG, "Connection established");

	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();
	erpc_transport_t arbitrator =
		erpc_arbitrated_client_init(transport, message_buffer_factory);
	erpc_arbitrated_client_set_error_handler(client_error);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_init(arbitrator, message_buffer_factory);
	erpc_add_service_to_server(create_hello_world_target_service());

	xTaskCreate(server_task, "server", 0x1000, NULL, SERVER_TASK_PRIORITY,
				NULL);
	xTaskCreate(client_task, "client", 0x1000, NULL, CLIENT_TASK_PRIORITY,
				NULL);
}
