/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		uart_transport_setup.c
 *
 * \brief		ERPC ESP-IDF UART transport setup functions
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#include "erpc_esp_uart_transport_setup.h"

#include "uart_transport.hpp"

#include "erpc_manually_constructed.hpp"

using namespace erpc;
using namespace erpc::esp;

static ManuallyConstructed<UARTTransport> s_transport;

erpc_transport_t erpc_esp_transport_uart_init(uart_port_t port) {
	erpc_transport_t transport;

	s_transport.construct(port);
	if (s_transport->init() == kErpcStatus_Success) {
		transport = reinterpret_cast<erpc_transport_t>(s_transport.get());
	} else {
		transport = NULL;
	}

	return transport;
}
