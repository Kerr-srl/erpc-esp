#ifndef ERPC_ESP_LOG_TRANSRPOT_COMMON_H_
#define ERPC_ESP_LOG_TRANSRPOT_COMMON_H_

#include "erpc_esp_log_transport_setup.h"

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define REQUIRED_BUFFER_SIZE(input) (((size_t)input * 2u) + 1u)

/**
 * Send using esp log
 */
void erpc_esp_log_transport_send(const uint8_t *data, uint32_t size,
								 char *buffer, size_t buffer_len);

#ifdef __cplusplus
}
#endif

#endif /* ifndef ERPC_ESP_LOG_TRANSRPOT_COMMON_H_ */
