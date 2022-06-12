/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		uart_transport.hpp
 *
 * \brief		ERPC ESP-IDF UART transport class - interface
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */
#ifndef ERPC_ESP_UART_TRANSPORT_HPP_H_
#define ERPC_ESP_UART_TRANSPORT_HPP_H_

#include "erpc_framed_transport.h"

#include "driver/uart.h"

#include <string>

namespace erpc {
namespace esp {

/*!
 * @brief UART transport layer based on ESP-IDF
 */
class UARTTransport : public FramedTransport {
  public:
	/*!
	 * @brief Constructor.
	 */
	UARTTransport(uart_port_t port);

	/*!
	 * @brief Destructor.
	 */
	virtual ~UARTTransport(void);

	/*!
	 * @brief Initialize Serial peripheral.
	 *
	 * @return Status of init function.
	 */
	erpc_status_t init(void);

  private:
	/*!
	 * @brief Write data to Serial peripheral.
	 *
	 * @param[in] data Buffer to send.
	 * @param[in] size Size of data to send.
	 *
	 * @retval kErpcStatus_ReceiveFailed Serial failed to receive data.
	 * @retval kErpcStatus_Success Successfully received all data.
	 */
	virtual erpc_status_t underlyingSend(const uint8_t *data, uint32_t size);

	/*!
	 * @brief Receive data from Serial peripheral.
	 *
	 * @param[inout] data Preallocated buffer for receiving data.
	 * @param[in] size Size of data to read.
	 *
	 * @retval kErpcStatus_ReceiveFailed Serial failed to receive data.
	 * @retval kErpcStatus_Success Successfully received all data.
	 */
	virtual erpc_status_t underlyingReceive(uint8_t *data, uint32_t size);

  private:
	uart_port_t m_port;
};
} // namespace esp
} // namespace erpc

#endif /* ifndef ERPC_ESP_UART_TRANSPORT_HPP_H_ */
