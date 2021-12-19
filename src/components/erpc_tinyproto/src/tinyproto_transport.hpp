/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		tinyproto_transport.hpp
 *
 * \brief		ERPC Tinyproto transport class - interface
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */
#ifndef ERPC_TINYPROTO_TRANSPORT_HPP_HPP_
#define ERPC_TINYPROTO_TRANSPORT_HPP_HPP_

#include "erpc_framed_transport.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

#include "TinyProtocolFd.h"

#include <string>

namespace erpc {
namespace esp {

/*!
 * @brief Tinyproto transport layer
 */
class TinyprotoTransport : public FramedTransport {
  public:
	/*!
	 * @brief Constructor.
	 */
	TinyprotoTransport(void *buffer, size_t buffer_size,
					   write_block_cb_t write_func, read_block_cb_t read_func,
					   TickType_t send_timeout = pdMS_TO_TICKS(500),
					   TickType_t receive_timeout = pdMS_TO_TICKS(500))
		: tinyproto_(buffer, buffer_size), write_func_(write_func),
		  read_func_(read_func), send_timeout_(send_timeout),
		  receive_timeout_(receive_timeout) {
		this->sent_semaphore.handle =
			xSemaphoreCreateBinaryStatic(&this->sent_semaphore.buf);
		this->rx_fifo.handle =
			xStreamBufferCreateStatic(sizeof(this->rx_fifo.buffer), 1,
									  this->rx_fifo.buffer, &this->rx_fifo.buf);
	}

	/*!
	 * @brief This function will establish connection with the other side
	 *
	 * \param [in] timeout_ticks connetion timeout in FreeRTOS ticks
	 *
	 * @retval #kErpcStatus_Success When creating host was successful or client
	 * connected successfully.
	 * @retval #kErpcStatus_UnknownName Host name resolution failed.
	 * @retval #kErpcStatus_ConnectionFailure Connecting to the specified host
	 * failed.
	 */
	erpc_status_t connect(TickType_t timeout_ticks = portMAX_DELAY);

	/*!
	 * @brief This function disconnects with the other side.
	 *
	 * @param[in] stopServer Specify is server shall be closed as well (stop
	 * listen())
	 * @retval #kErpcStatus_Success Always return this.
	 */
	erpc_status_t close(bool stopServer = true);

  private:
	/*!
	 * @brief Write data to the Tinyproto connection.
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
	 * @brief Read data from the Tinyproto connection.
	 *
	 * @param[inout] data Preallocated buffer for receiving data.
	 * @param[in] size Size of data to read.
	 *
	 * @retval kErpcStatus_ReceiveFailed Serial failed to receive data.
	 * @retval kErpcStatus_Success Successfully received all data.
	 */
	virtual erpc_status_t underlyingReceive(uint8_t *data,
											uint32_t size) override;
	static void rx_task(void *user_data);
	static void tx_task(void *user_data);
	/**
	 * Tinyproto callback used when application data has been received
	 */
	static void receive_cb(void *user_data, tinyproto::IPacket &pkt);
	/**
	 * Tinyproto callback used when application data has been successfully
	 * (reliably) sent
	 */
	static void sent_cb(void *user_data, tinyproto::IPacket &pkt);

	/**
	 * Full-duplex Tinyproto instance
	 */
	tinyproto::IFd tinyproto_;
	/**
	 * User provided transmission medium low-level write function
	 */
	write_block_cb_t write_func_;
	/**
	 * User provided transmission medium low-level read function
	 */
	read_block_cb_t read_func_;
	/**
	 * A static FreeRTOS task
	 */
	struct protocol_task {
		StaticTask_t buffer;
		StackType_t stack[2048];
	};
	/**
	 * Task that reads and processes incoming data
	 *
	 * See also rx_task
	 */
	protocol_task rx_task_;
	/**
	 * Task that sends tinyproto frames on the wire
	 *
	 * See also tx_task
	 */
	protocol_task tx_task_;
	/**
	 * Underlying send timeout
	 */
	TickType_t send_timeout_;
	/**
	 * Underlying receive timeout
	 */
	TickType_t receive_timeout_;
	/**
	 * Semaphore used to notify to the underlyingSend function that a Tinyproto
	 * FD frame has been sent.
	 */
	struct {
		StaticSemaphore_t buf;
		SemaphoreHandle_t handle;
	} sent_semaphore;
	/**
	 * FIFO that contains data that received via receive_cb (called by
	 * Tinyproto) and that underlyingReceive waits.
	 */
	struct {
		StaticStreamBuffer_t buf;
		StreamBufferHandle_t handle;
		uint8_t buffer[256];
	} rx_fifo;
};
} // namespace esp
} // namespace erpc

#endif /* ifndef ERPC_TINYPROTO_TRANSPORT_HPP_HPP_ */
