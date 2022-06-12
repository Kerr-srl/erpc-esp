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

#include "erpc_esp_log_transport_setup.h"

#include "erpc_esp_log_transport.hpp"

#include "erpc_manually_constructed.h"

using namespace erpc;
using namespace erpc::esp;

static ManuallyConstructed<EspLogTransport> s_transport;

erpc_transport_t erpc_esp_log_transport_init(
	const struct erpc_esp_log_transport_config *config) {
	erpc_transport_t transport;

	s_transport.construct(*config);
	transport = reinterpret_cast<erpc_transport_t>(s_transport.get());

	return transport;
}
