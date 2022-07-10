#ifndef ERPC_ESP_HOST_POSIX_IO_H_
#define ERPC_ESP_HOST_POSIX_IO_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"

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
	union {
		struct {
			struct {
				StaticMessageBuffer_t buf;
				StreamBufferHandle_t handle;
				uint8_t data_buffer[256];
			} fifo;
		} tx;
		struct {
			struct {
				/**
				 * NULL if no no pending read to be performed by reader thread
				 */
				uint8_t *buffer;
				size_t buffer_max_size;
				size_t actually_read_size;
			} pending;
			struct {
				StaticSemaphore_t buf;
				SemaphoreHandle_t handle;
			} sem;
		} rx;
	} io;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
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
