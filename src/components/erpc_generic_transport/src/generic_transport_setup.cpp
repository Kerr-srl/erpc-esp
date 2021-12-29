/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		tinyproto_transport_setup.c
 *
 * \brief		ERPC Tinyproto transport setup functions
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#include "erpc_generic_transport_setup.h"

#include "generic_transport.hpp"

#include "erpc_manually_constructed.h"

using namespace erpc;
using namespace erpc::esp;

static ManuallyConstructed<GenericTransport> s_transport;

erpc_transport_t erpc_esp_transport_generic_init(basic_write write_func,
												 basic_read read_func) {
	erpc_transport_t transport;

	s_transport.construct(write_func, read_func);
	transport = reinterpret_cast<erpc_transport_t>(s_transport.get());

	return transport;
}
