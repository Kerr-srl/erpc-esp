/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		erpc_esp_tinyproto_transport_setup.c
 *
 * \brief		ERPC ESP-IDF TINYPROTO transport setup functions
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#ifndef ERPC_ESP_TINYPROTO_TRANSPORT_SETUP_H_
#define ERPC_ESP_TINYPROTO_TRANSPORT_SETUP_H_

#include "TinyProtocolFd.h"

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

/*!
 * @brief Create an ESP-IDF Tinyproto transport.
 *
 * @param [in] port Tinyproto port
 * @param [in] baud_rate baud rate
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_esp_transport_tinyproto_init(
	void *buffer, size_t buffer_size, write_block_cb_t write_func,
	read_block_cb_t read_func, TickType_t send_timeout = pdMS_TO_TICKS(500),
	TickType_t receive_timeout = pdMS_TO_TICKS(500),
	TickType_t connect_timeout = portMAX_DELAY);

#ifdef __cplusplus
}
#endif

#endif // ERPC_ESP_TINYPROTO_TRANSPORT_SETUP_H_
