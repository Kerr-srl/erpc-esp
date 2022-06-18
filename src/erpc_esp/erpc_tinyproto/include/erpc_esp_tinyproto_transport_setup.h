/**
 * \file		erpc_esp_tinyproto_transport_setup.c
 *
 * \brief		ERPC ESP-IDF TINYPROTO transport setup functions
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#ifndef ERPC_ESP_TINYPROTO_TRANSPORT_SETUP_H_
#define ERPC_ESP_TINYPROTO_TRANSPORT_SETUP_H_

#include "hal/tiny_types.h"

#include "freertos/FreeRTOS.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Opaque transport object type.
 */
typedef struct ErpcTransport *erpc_transport_t;

/**
 * TinyProto transport configuration.
 */
struct erpc_esp_transport_tinyproto_config {
	/**
	 * Underlying send timeout
	 */
	TickType_t send_timeout;
	/**
	 * Underlying receive timeout
	 */
	TickType_t receive_timeout;
	/**
	 * Connection status change callback
	 *
	 * \param [in] connected whether now the link is connected
	 *
	 * May be NULL.
	 */
	void (*on_connect_status_change_cb)(bool connected);
	/**
	 * Rx task priority
	 */
	UBaseType_t rx_task_priority;
	/**
	 * Tx task priority
	 */
	UBaseType_t tx_task_priority;
};

#define ERPC_ESP_TRANSPORT_TINYPROTO_CONFIG_DEFAULT()                          \
	{                                                                          \
		.send_timeout = pdMS_TO_TICKS(500),                                    \
		.receive_timeout = pdMS_TO_TICKS(500), .tx_task_priority = 10,         \
		.rx_task_priority = 11,                                                \
	}

/*!
 * @brief Create an ESP-IDF Tinyproto transport.
 *
 * @param [in] buffer Tinyproto reception buffer
 * @param [in] buffer_size size of the buffer
 * @param [in] write_func low level write function
 * @param [in] read_func low level read function
 * @param [in] config other misc tinyproto configuration
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_esp_transport_tinyproto_init(
	void *buffer, size_t buffer_size, write_block_cb_t write_func,
	read_block_cb_t read_func,
	const struct erpc_esp_transport_tinyproto_config *config);

/**
 * Block and wait until connection is established
 *
 * \param [in] timeout
 *
 * \retval true connection established
 * \retval false timeout
 */
bool erpc_esp_transport_tinyproto_connect(TickType_t timeout);

#ifdef __cplusplus
}
#endif

#endif // ERPC_ESP_TINYPROTO_TRANSPORT_SETUP_H_
