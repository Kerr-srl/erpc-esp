#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"

#include "erpc_esp_tinyproto_transport_setup.h"

#if HOST
#	include "gen/hello_world_with_connection_host_server.h"
#	include "gen/hello_world_with_connection_target.h"
#else
#	include "gen/hello_world_with_connection_host.h"
#	include "gen/hello_world_with_connection_target_server.h"
#endif

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#define TAG "main"

#include "sdkconfig.h"

#include <string.h>

#define RPC_UART_PORT ((CONFIG_ESP_CONSOLE_UART_NUM + 1) % UART_NUM_MAX)
// Change these pins as needed to make them work on your hardware
#define RPC_UART_TX GPIO_NUM_19
#define RPC_UART_RX GPIO_NUM_20

#define TX_TASK_PRIORITY 1
#define RX_TASK_PRIORITY 2

#define CLIENT_TASK_PRIORITY 2
#define SERVER_TASK_PRIORITY 3

#define CONNECTED_BIT BIT0

#if HOST
char *say_hello_to_host(const char *name, uint32_t ith) {
	size_t len = strlen("Hello ") + strlen(name) + 1;
	char *response = erpc_malloc(len);
	assert(response);
	memset(response, 0, len);
	strcat(response, "Hello ");
	strcat(response, name);
	ESP_LOGI(TAG, "Received call from target. Responding with \"%s\"",
			 response);
	return response;
}
#else
char *say_hello_to_target(const char *name, uint32_t ith) {
	size_t len = strlen("Hello ") + strlen(name) + 1;
	char *response = erpc_malloc(len);
	assert(response);
	memset(response, 0, len);
	strcat(response, "Hello ");
	strcat(response, name);
	ESP_LOGI(TAG, "Received call from host. Responding with \"%s\"", response);
	return response;
}
#endif

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

static uint8_t buffer[1000];

static int tinyproto_write_fn(void *pdata, const void *buffer, int size) {
	return uart_write_bytes(RPC_UART_PORT, buffer, size);
}
static int tinyproto_read_fn(void *pdata, void *buffer, int size) {
	return uart_read_bytes(RPC_UART_PORT, buffer, size, pdMS_TO_TICKS(10));
}

static struct erpc_esp_transport_tinyproto_config tinyproto_config =
	ERPC_ESP_TRANSPORT_TINYPROTO_CONFIG_DEFAULT();

void app_main() {
	ESP_LOGD(TAG, "Initializing Tinyproto transport");

	ESP_ERROR_CHECK(
		uart_driver_install(RPC_UART_PORT, 2048, 2048, 100, NULL, 0));
	ESP_ERROR_CHECK(uart_set_pin(RPC_UART_PORT, RPC_UART_TX, RPC_UART_RX,
								 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	g_event_flag = xEventGroupCreate();
	assert(g_event_flag);

	tinyproto_config.rx_task_priority = RX_TASK_PRIORITY;
	tinyproto_config.tx_task_priority = TX_TASK_PRIORITY;
	tinyproto_config.send_timeout = pdMS_TO_TICKS(5000);
	tinyproto_config.receive_timeout = pdMS_TO_TICKS(5000);
	tinyproto_config.on_connect_status_change_cb =
		on_tinyproto_connect_status_change;
	erpc_transport_t transport = erpc_esp_transport_tinyproto_init(
		buffer, sizeof(buffer), tinyproto_write_fn, tinyproto_read_fn,
		&tinyproto_config);

	bool res = erpc_esp_transport_tinyproto_connect(portMAX_DELAY);
	assert(res);
	ESP_LOGI(TAG, "Connection established");

	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();
	erpc_transport_t arbitrator =
		erpc_arbitrated_client_init(transport, message_buffer_factory);
	erpc_arbitrated_client_set_error_handler(client_error);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_init(arbitrator, message_buffer_factory);
#if HOST
	erpc_add_service_to_server(create_hello_world_host_service());
#else
	erpc_add_service_to_server(create_hello_world_target_service());
#endif

	xTaskCreate(server_task, "server", 0x1000, NULL, SERVER_TASK_PRIORITY,
				NULL);
	xTaskCreate(client_task, "client", 0x1000, NULL, CLIENT_TASK_PRIORITY,
				NULL);
}
