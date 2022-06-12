/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		erpc_esp_generic_transport_setup.c
 *
 * \brief		ERPC ESP-IDF generic transport setup functions
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#ifndef ERPC_ESP_GENERIC_TRANSPORT_SETUP_H_
#define ERPC_ESP_GENERIC_TRANSPORT_SETUP_H_

#include "erpc_common.h"

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
typedef erpc_status_t (*basic_write)(const uint8_t *data, uint32_t size);
typedef erpc_status_t (*basic_read)(uint8_t *data, uint32_t size);

/*!
 * @brief Create an ESP-IDF Generic transport.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_esp_transport_generic_init(basic_write write_func,
												 basic_read read_func);

#ifdef __cplusplus
}
#endif

#endif // ERPC_ESP_GENERIC_TRANSPORT_SETUP_H_
