/**
 * \verbatim
 *                              _  __
 *                             | |/ /
 *                             | ' / ___ _ __ _ __
 *                             |  < / _ \ '__| '__|
 *                             | . \  __/ |  | |
 *                             |_|\_\___|_|  |_|
 * \endverbatim
 * \file		esp_log_transport.hpp
 *
 * \brief		ERPC ESP Log transport class - interface
 *
 * \copyright	Copyright 2021 Kerr s.r.l. - All Rights Reserved.
 */
#ifndef ERPC_ESP_LOG_TRANSPORT_HPP_
#define ERPC_ESP_LOG_TRANSPORT_HPP_

#include "erpc_esp_log_transport_setup.h"

#include "erpc_framed_transport.h"

#include <string>

namespace erpc {
namespace esp {

/*!
 * @brief EspLog transport layer
 */
class EspLogTransport : public FramedTransport {
  public:
	/*!
	 * @brief Constructor.
	 *
	 * \param [in] config transport config
	 */
	EspLogTransport(const erpc_esp_log_transport_config &config);
	~EspLogTransport();

  private:
	virtual erpc_status_t underlyingSend(const uint8_t *data,
										 uint32_t size) override;
	virtual erpc_status_t underlyingReceive(uint8_t *data,
											uint32_t size) override;
	erpc_esp_log_transport_config config_;
};
} // namespace esp
} // namespace erpc

#endif /* ifndef ERPC_ESP_LOG_TRANSPORT_HPP_ */
