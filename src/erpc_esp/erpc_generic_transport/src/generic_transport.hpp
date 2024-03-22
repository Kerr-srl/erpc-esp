/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		generic_transport.hpp
 *
 * \brief		ERPC Generic transport class - interface
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */
#ifndef ERPC_GENERIC_TRANSPORT_HPP_
#define ERPC_GENERIC_TRANSPORT_HPP_

#include "erpc_generic_transport_setup.h"

#include "erpc_framed_transport.hpp"

#include <string>

namespace erpc {
namespace esp {

/*!
 * @brief Generic transport layer
 */
class GenericTransport : public FramedTransport {
  public:
	/*!
	 * @brief Constructor.
	 */
	GenericTransport(basic_write write_func, basic_read read_func)
		: basic_write_fn_(write_func), basic_read_fn_(read_func) {
	}

  private:
	/*!
	 * @brief Write data to the Generic connection.
	 *
	 * @param[in] data Buffer to send.
	 * @param[in] size Size of data to send.
	 *
	 * @retval kErpcStatus_ReceiveFailed Serial failed to receive data.
	 * @retval kErpcStatus_Success Successfully received all data.
	 */
	virtual erpc_status_t underlyingSend(const uint8_t *data,
										 uint32_t size) override;

	/*!
	 * @brief Read data from the Generic connection.
	 *
	 * @param[inout] data Preallocated buffer for receiving data.
	 * @param[in] size Size of data to read.
	 *
	 * @retval kErpcStatus_ReceiveFailed Serial failed to receive data.
	 * @retval kErpcStatus_Success Successfully received all data.
	 */
	virtual erpc_status_t underlyingReceive(uint8_t *data,
											uint32_t size) override;
	/**
	 * User provided transmission medium low-level write function
	 */
	basic_write basic_write_fn_;
	/**
	 * User provided transmission medium low-level read function
	 */
	basic_read basic_read_fn_;
};
} // namespace esp
} // namespace erpc

#endif /* ifndef ERPC_GENERIC_TRANSPORT_HPP_ */
