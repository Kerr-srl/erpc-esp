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

#include "erpc_esp_tinyproto_transport_setup.h"

#include "tinyproto_transport.hpp"

#include "erpc_manually_constructed.h"

using namespace erpc;
using namespace erpc::esp;

static ManuallyConstructed<TinyprotoTransport> s_transport;

erpc_transport_t erpc_esp_transport_tinyproto_init(
	void *buffer, size_t buffer_size, write_block_cb_t write_func,
	read_block_cb_t read_func,
	const struct erpc_esp_transport_tinyproto_config *config) {

	s_transport.construct(buffer, buffer_size, write_func, read_func, *config);
	return reinterpret_cast<erpc_transport_t>(s_transport.get());
}

bool erpc_esp_transport_tinyproto_connect(TickType_t timeout) {
	return s_transport->connect(timeout) == kErpcStatus_Success;
}
