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

#include "erpc_esp_tinyproto_transport_setup.h"

#include "erpc_framed_transport.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "TinyProtocolFd.h"

#include <string>

namespace erpc {
namespace esp {

/*!
 * @brief Tinyproto transport layer
 *
 * Tinyproto (which is based on HDLC) is already a framed protocol, so we
 * inherit from Transport instead of FramedTransport
 */
class TinyprotoTransport : public Transport {
  public:
	/*!
	 * @brief Constructor.
	 *
	 * @param [in] buffer Tinyproto port
	 * @param [in] buffer_size
	 * @param [in] write_func low level write function
	 * @param [in] read_func low level read function
	 * @param [in] config other misc tinyproto configuration
	 */
	TinyprotoTransport(void *buffer, size_t buffer_size,
					   write_block_cb_t write_func, read_block_cb_t read_func,
					   const erpc_esp_transport_tinyproto_config &config);

	/*!
	 * @brief This function will establish connection with the other side
	 *
	 * \param [in] timeout connection timeout
	 *
	 * @retval #kErpcStatus_Success When creating host was successful or client
	 * connected successfully.
	 * @retval #kErpcStatus_Timeout Connection timeout
	 */
	erpc_status_t connect(TickType_t timeout);

  private:
	/*!
	 * @brief Write data to the Tinyproto connection.
	 *
	 * @param[in] message Message to send.
	 *
	 * @retval kErpcStatus_ReceiveFailed Serial failed to receive data.
	 * @retval kErpcStatus_Success Successfully received all data.
	 */
	virtual erpc_status_t send(MessageBuffer *message) override;

	/*!
	 * @brief Read data from the Tinyproto connection.
	 *
	 * @param[in] message Message to receive.
	 *
	 * @retval kErpcStatus_ReceiveFailed Serial failed to receive data.
	 * @retval kErpcStatus_Success Successfully received all data.
	 */
	virtual erpc_status_t receive(MessageBuffer *message) override;
	virtual bool hasMessage(void) override;

	static void rx_task(void *user_data);
	static void tx_task(void *user_data);
	/**
	 * Tinyproto callback used when application data has been received
	 */
	static void receive_cb(void *user_data, uint8_t addr,
						   tinyproto::IPacket &pkt);
	/**
	 * Tinyproto callback used when connection status has changed
	 */
	static void connect_cb(void *user_data, uint8_t addr, bool connected);

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
		/*
		 * Usually 2K is enough, but sometimes (typically due to logging)
		 * we hit stack overflow. So let's add another 1K.
		 */
		StackType_t stack[2048 + 1024];
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
	 * Configuration
	 */
	erpc_esp_transport_tinyproto_config config_;
	/**
	 * FIFO that contains data that received via receive_cb (called by
	 * Tinyproto) and that receive waits.
	 */
	struct {
		StaticMessageBuffer_t buf;
		MessageBufferHandle_t handle;
		uint8_t buffer[256];
	} rx_fifo;
	struct {
		StaticEventGroup_t buf;
		EventGroupHandle_t handle;
	} events;
};
} // namespace esp
} // namespace erpc

#endif /* ifndef ERPC_TINYPROTO_TRANSPORT_HPP_HPP_ */
