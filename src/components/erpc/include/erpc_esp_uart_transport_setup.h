/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		erpc_esp_uart_transport_setup.c
 *
 * \brief		ERPC ESP-IDF UART transport setup functions
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#ifndef ERPC_ESP_UART_TRANSPORT_SETUP_H_
#define ERPC_ESP_UART_TRANSPORT_SETUP_H_

#include "driver/uart.h"

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
 * @brief Create an ESP-IDF UART transport.
 *
 * @param [in] port UART port
 * @param [in] baud_rate baud rate
 *
 * @return Return NULL or erpc_transport_t instance pointer.
 */
erpc_transport_t erpc_esp_transport_uart_init(uart_port_t port);

#ifdef __cplusplus
}
#endif

#endif // ERPC_ESP_UART_TRANSPORT_SETUP_H_
