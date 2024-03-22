#include "erpc_arbitrated_client_setup.h"
#include "erpc_mbf_setup.h"
#include "erpc_port.h"
#include "erpc_server_setup.h"
#include "erpc_transport_setup.h"

#include "erpc_generic_transport_setup.h"

#include "gen/c_hello_world_with_socket_host_client.h"
#include "gen/c_hello_world_with_socket_target_server.h"

#include "esp_log.h"
#define TAG "main"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "sdkconfig.h"

#include <string.h>
#include <assert.h>

#define CONFIG_EXAMPLE_WIFI_SSID "myssid"
#define CONFIG_EXAMPLE_WIFI_PASSWORD "mypassword"
#define CONFIG_EXAMPLE_HOST_IP_ADDRESS "hostip"
#define CONFIG_EXAMPLE_HOST_IP_PORT (28080)

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
	erpc_server_t server = (erpc_server_t)params;

	ESP_LOGI(TAG, "Starting server");
	erpc_status_t status = erpc_server_run(server);
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

static SemaphoreHandle_t g_semph_got_ip_addr;

static void event_handler(void *arg, esp_event_base_t event_base,
						  int32_t event_id, void *event_data) {
	assert(event_base == WIFI_EVENT);
	if (event_id == WIFI_EVENT_STA_START) {
		ESP_LOGI(TAG, "Wifi started. Connecting...");
		ESP_ERROR_CHECK(esp_wifi_connect());
	} else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
		ESP_LOGI(TAG, "Disconnected. Reconnecting...");
		ESP_ERROR_CHECK(esp_wifi_connect());
	} else {
		ESP_LOGW(TAG, "Unhandled wifi event %d", (int)event_id);
	}
}

static void on_got_ip(void *arg, esp_event_base_t event_base, int32_t event_id,
					  void *event_data) {
	ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
	ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR,
			 esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
	xSemaphoreGive(g_semph_got_ip_addr);
}

static void wifi_start(void) {
	g_semph_got_ip_addr = xSemaphoreCreateBinary();
	assert(g_semph_got_ip_addr != NULL);

	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
											   &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
											   &on_got_ip, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	wifi_config_t wifi_config = {
		.sta =
			{
				.ssid = CONFIG_EXAMPLE_WIFI_SSID,
				.password = CONFIG_EXAMPLE_WIFI_PASSWORD,
				.pmf_cfg =
					{
						.capable = true,
						.required = false,
					},
			},
	};

	ESP_LOGI(TAG, "SSID: %s, %zu", wifi_config.sta.ssid,
			 strlen((const char *)wifi_config.sta.ssid));
	ESP_LOGI(TAG, "Password: %s, %zu", wifi_config.sta.password,
			 strlen((const char *)wifi_config.sta.password));

	ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

int g_socket_fd = -1;

void init_socket() {
	char host_ip[] = CONFIG_EXAMPLE_HOST_IP_ADDRESS;
	int addr_family = 0;
	int ip_protocol = 0;

	struct sockaddr_in dest_addr;
	dest_addr.sin_addr.s_addr = inet_addr(host_ip);
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(CONFIG_EXAMPLE_HOST_IP_PORT);
	addr_family = AF_INET;
	ip_protocol = IPPROTO_IP;

	g_socket_fd = socket(addr_family, SOCK_STREAM, ip_protocol);
	if (g_socket_fd < 0) {
		ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
		assert(0);
	}
	ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip,
			 CONFIG_EXAMPLE_HOST_IP_PORT);

	int err = connect(g_socket_fd, (struct sockaddr *)&dest_addr,
					  sizeof(struct sockaddr_in6));
	if (err != 0) {
		ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
		assert(0);
	}
	ESP_LOGI(TAG, "Successfully connected");
}

erpc_status_t basic_write_fn(const uint8_t *data, uint32_t size) {
	return send(g_socket_fd, data, size, 0) == size ? kErpcStatus_Success
													: kErpcStatus_Fail;
}
erpc_status_t basic_read_fn(uint8_t *data, uint32_t size) {
	ssize_t length;
	erpc_status_t status = kErpcStatus_Success;

	// Loop until all requested data is received.
	while (size > 0U) {
		length = read(g_socket_fd, data, size);

		// Length will be zero if the connection is closed.
		if (length > 0) {
			size -= length;
			data += length;
		} else {
			if (length == 0) {
				// close socket, not server
				close(false);
				status = kErpcStatus_ConnectionClosed;
			} else {
				status = kErpcStatus_ReceiveFailed;
			}
			break;
		}
	}

	return status;
}

void app_main() {
	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_start();

	xSemaphoreTake(g_semph_got_ip_addr, portMAX_DELAY);

	init_socket();

	erpc_transport_t transport =
		erpc_esp_transport_generic_init(basic_write_fn, basic_read_fn);
	ESP_LOGI(TAG, "TCP transport created");

	erpc_mbf_t message_buffer_factory = erpc_mbf_static_init();
	erpc_transport_t arbitrator;
	erpc_client_t client = erpc_arbitrated_client_init(
		transport, message_buffer_factory, &arbitrator);
	inithello_world_host_client(client);
	erpc_arbitrated_client_set_error_handler(client, client_error);

	ESP_LOGD(TAG, "Initializing server");
	erpc_server_t server = erpc_server_init(arbitrator, message_buffer_factory);
	erpc_add_service_to_server(server, create_hello_world_target_service());

	xTaskCreate(server_task, "server", 0x1000, server, 1, NULL);
	xTaskCreate(client_task, "client", 0x1000, NULL, 1, NULL);

	while (1) {
		vTaskDelay(portMAX_DELAY);
	}
}
