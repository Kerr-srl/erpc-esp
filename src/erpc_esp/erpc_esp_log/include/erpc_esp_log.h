/**
 * \file		erpc_esp_log.c
 *
 * \brief		eRPC ESP-IDF log utilities -interface
 *
 * \copyright	Copyright 2022 Kerr s.r.l. - All Rights Reserved.
 */
#ifndef ERPC_ESP_LOG_H_
#define ERPC_ESP_LOG_H_

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Size of the char buffer that is required to send  \p _input_len_ bytes
 */
#define ERPC_ESP_LOG_REQUIRED_BUFFER_SIZE(_input_len_)                         \
	(((size_t)_input_len_ * 2u) + 1u)

/**
 * Call this function before using #erpc_esp_log_transport_send
 */
void erpc_esp_log_transport_init(void);

/**
 * Send using esp log
 *
 * \param [in] data bytes to be sent
 * \param [in] size number of bytes to be sent
 * \param [in] buffer buffer used to hold the encoded payload. It must be large
 * at least ERPC_ESP_LOG_REQUIRED_BUFFER_SIZE.
 */
void erpc_esp_log_transport_send(const uint8_t *data, uint32_t size,
								 char *buffer);

#ifdef __cplusplus
}
#endif

#endif /* ifndef ERPC_ESP_LOG_H_ */
