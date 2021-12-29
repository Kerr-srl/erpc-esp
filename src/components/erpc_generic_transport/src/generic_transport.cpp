/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		uart_transport.cpp
 *
 * \brief		ERPC ESP-IDF Generic transport class - implementation
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */

#include "generic_transport.hpp"

#include <cassert>

using namespace erpc::esp;

erpc_status_t GenericTransport::underlyingSend(const uint8_t *data,
											   uint32_t size) {
	return this->basic_write_fn_(data, size);
}
erpc_status_t GenericTransport::underlyingReceive(uint8_t *data,
												  uint32_t size) {

	return this->basic_read_fn_(data, size);
}
