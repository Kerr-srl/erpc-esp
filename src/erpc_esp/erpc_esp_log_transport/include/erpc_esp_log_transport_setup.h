/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		erpc_esp_log_transport_setup.c
 *
 * \brief		ERPC ESP-IDF log transport setup functions
 *
 * \copyright	Copyright 2022 Kerr s.r.l. - All Rights Reserved.
 */

#ifndef ERPC_ESP_LOG_TRANSPORT_SETUP_H_
#define ERPC_ESP_LOG_TRANSPORT_SETUP_H_

#include "erpc_common.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Opaque transport object type.
 */
typedef struct ErpcTransport *erpc_transport_t;

struct erpc_esp_log_transport_config {
	/**
	 * ERPC reception timeout in milliseconds
	 */
	uint32_t rx_timeout_ms;
	struct {
		/**
		 * May be NULL, in which case VLA is used.
		 */
		char *buffer;
		/**
		 * Length of the buffer
		 */
		size_t len;
	} tx_buffer;
};

/*!
 * @brief Create an ESP-IDF log transport.
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t
erpc_esp_log_transport_init(const struct erpc_esp_log_transport_config *config);

#ifdef __cplusplus
}
#endif

#endif // ERPC_ESP_LOG_TRANSPORT_SETUP_H_
