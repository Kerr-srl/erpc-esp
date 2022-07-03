#ifndef ERPC_ESP_HOST_POSIX_IO_H_
#define ERPC_ESP_HOST_POSIX_IO_H_

#include "freertos/FreeRTOS.h"
#include "freertos/stream_buffer.h"

#include <pthread.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * erpc_esp_host_posix_io handle
 *
 * Use as opaque type
 */
typedef struct {
	int fd;
	StaticMessageBuffer_t buf;
	StreamBufferHandle_t handle;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	uint8_t buffer[256];
} erpc_esp_host_posix_io;

/**
 * \param [in] handle:
 * \param [in] fd open file descriptor
 * \param [in] read_or_write false for read, true for write
 */
void erpc_esp_host_posix_io_init(erpc_esp_host_posix_io *handle, int fd,
								 bool read_or_write);

int erpc_esp_host_posix_read(erpc_esp_host_posix_io *handle, void *data,
							 size_t size);

int erpc_esp_host_posix_write(erpc_esp_host_posix_io *handle, const void *data,
							  size_t size);

#ifdef __cplusplus
}
#endif

#endif /* ifndef ERPC_ESP_HOST_POSIX_IO_H_ */
