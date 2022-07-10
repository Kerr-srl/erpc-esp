#include "erpc_esp/host/posix_io.h"
#include "erpc_esp_tinyproto_transport_setup.h"

#include "gen/hello_world_host.h"
#include "gen/hello_world_target_server.h"

#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

#include "esp_log.h"
#define TAG "main"

#include "sdkconfig.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TX_TASK_PRIORITY 3
#define RX_TASK_PRIORITY 3

#define CLIENT_TASK_PRIORITY 2
#define SERVER_TASK_PRIORITY 2

#define CONNECTED_BIT (1)

void say_hello_to_target(const char *name, uint32_t ith) {
	ESP_LOGI(TAG, "Received [%u]th call from host.", ith);
}

static EventGroupHandle_t g_event_flag;

void server_task(void *params) {
	erpc_esp_transport_tinyproto_wait_connected(portMAX_DELAY);
	ESP_LOGI(TAG, "Connected. Starting server.");
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
	erpc_esp_transport_tinyproto_wait_connected(portMAX_DELAY);
	ESP_LOGI(TAG, "Connected. Starting client.");
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

static erpc_esp_host_posix_io g_posix_io_stderr;
static int tinyproto_write_fn(void *pdata, const void *buffer, int size) {
	ESP_LOGD(TAG, "Sending %d bytes", size);
	return erpc_esp_host_posix_write(&g_posix_io_stderr, buffer, size);
}

static erpc_esp_host_posix_io g_posix_io_stdin;
static int tinyproto_read_fn(void *pdata, void *buffer, int size) {
	ESP_LOGD(TAG, "Reading %d bytes", size);
	return erpc_esp_host_posix_read(&g_posix_io_stdin, buffer, size);
}

static struct erpc_esp_transport_tinyproto_config tinyproto_config =
	ERPC_ESP_TRANSPORT_TINYPROTO_CONFIG_DEFAULT();

static uint8_t g_tinyproto_rx_buffer[1024];

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
								   StackType_t **ppxIdleTaskStackBuffer,
								   uint32_t *pulIdleTaskStackSize) {
	static StaticTask_t task_handle;
	static StackType_t task_stack_buffer[4096];
	*ppxIdleTaskTCBBuffer = &task_handle;
	*ppxIdleTaskStackBuffer = task_stack_buffer;
	*pulIdleTaskStackSize =
		sizeof(task_stack_buffer) / sizeof(task_stack_buffer[0]);
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
									StackType_t **ppxTimerTaskStackBuffer,
									uint32_t *pulTimerTaskStackSize) {
	static StaticTask_t task_handle;
	static StackType_t task_stack_buffer[4096];
	*ppxTimerTaskTCBBuffer = &task_handle;
	*ppxTimerTaskStackBuffer = task_stack_buffer;
	*pulTimerTaskStackSize =
		sizeof(task_stack_buffer) / sizeof(task_stack_buffer[0]);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
	ESP_LOGE(TAG, "Stack overflow of TASK \"%s\"", pcTaskName);
	exit(0);
}

void vApplicationIdleHook(void) {
	/*
	 * Why invoke nanosleep in idle hook?
	 * See https://www.freertos.org/FreeRTOS-simulator-for-Linux.html
	 * "Known Issues"
	 */
	struct timespec sleep = {.tv_sec = 1};
	nanosleep(&sleep, NULL);
}

static SemaphoreHandle_t printf_mutex;
static int vprint_with_freertos_mutex(const char *str, va_list va) {
	xSemaphoreTake(printf_mutex, portMAX_DELAY);
	int ret = vprintf(str, va);
	xSemaphoreGive(printf_mutex);
	return ret;
}

int main() {
	erpc_esp_host_posix_io_init(&g_posix_io_stderr, STDERR_FILENO, true);
	erpc_esp_host_posix_io_init(&g_posix_io_stdin, STDIN_FILENO, false);

	printf_mutex = xSemaphoreCreateMutex();
	assert(printf_mutex);
	esp_log_set_vprintf(vprint_with_freertos_mutex);

	// Increase this level to debug
	esp_log_level_set("*", ESP_LOG_INFO);

	ESP_LOGI(TAG, "Target started");

	g_event_flag = xEventGroupCreate();
	assert(g_event_flag);

	tinyproto_config.rx_task_priority = RX_TASK_PRIORITY;
	tinyproto_config.tx_task_priority = TX_TASK_PRIORITY;
	tinyproto_config.send_timeout = pdMS_TO_TICKS(5000);
	tinyproto_config.receive_timeout = portMAX_DELAY;
	tinyproto_config.on_connect_status_change_cb =
		on_tinyproto_connect_status_change;
	erpc_transport_t transport = erpc_esp_transport_tinyproto_init(
		g_tinyproto_rx_buffer, sizeof(g_tinyproto_rx_buffer),
		tinyproto_write_fn, tinyproto_read_fn, &tinyproto_config);

	erpc_esp_transport_tinyproto_open();

	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();
	erpc_transport_t arbitrator =
		erpc_arbitrated_client_init(transport, message_buffer_factory);
	erpc_arbitrated_client_set_error_handler(client_error);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_init(arbitrator, message_buffer_factory);
	erpc_add_service_to_server(create_hello_world_target_service());

	/*
	 * 4096 as stack size seems to be not enough. It causes application to hang.
	 */
	BaseType_t created = xTaskCreate(server_task, "server", 1024, NULL,
									 SERVER_TASK_PRIORITY, NULL);
	assert(created == pdPASS);
	created = xTaskCreate(client_task, "client", 1024, NULL,
						  CLIENT_TASK_PRIORITY, NULL);
	assert(created == pdPASS);

	vTaskStartScheduler();
	return 0;
}
