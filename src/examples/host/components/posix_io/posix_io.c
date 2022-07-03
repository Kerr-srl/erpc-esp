#include "erpc_esp/host/posix_io.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void *writer(void *arg) {
	erpc_esp_host_posix_io *handle = arg;
	uint8_t buffer[256];

	while (1) {
		// TODO can we use portYIELD_FROM_ISR? I don't think so.
		BaseType_t woken;

		pthread_mutex_lock(&handle->mutex);
		size_t n_read = xStreamBufferReceiveFromISR(handle->handle, buffer,
													sizeof(buffer), &woken);
		if (n_read == 0) {
			pthread_cond_wait(&handle->cond, &handle->mutex);
			n_read = xStreamBufferReceiveFromISR(handle->handle, buffer,
												 sizeof(buffer), &woken);
		}
		pthread_mutex_unlock(&handle->mutex);

		assert(n_read > 0);
		int ret = write(handle->fd, buffer, n_read);
		if (ret != n_read) {
			printf("Error [%d]: %s", ret, strerror(errno));
		}
	}
	return NULL;
}
static void *reader(void *arg) {
	erpc_esp_host_posix_io *handle = arg;

	uint8_t buf = 0;
	while (1) {
		int ret = read(handle->fd, &buf, 1);
		if (ret == 1) {
			// TODO can we use portYIELD_FROM_ISR? I don't think so.
			BaseType_t woken;

			pthread_mutex_lock(&handle->mutex);
			xStreamBufferSendFromISR(handle->handle, &buf, 1, &woken);
			pthread_mutex_unlock(&handle->mutex);
			pthread_cond_signal(&handle->cond);
		} else {
			printf("Error [%d]: %s", ret, strerror(errno));
		}
	}
	return NULL;
}

void erpc_esp_host_posix_io_init(erpc_esp_host_posix_io *handle, int fd,
								 bool read_or_write) {
	handle->handle = xStreamBufferCreateStatic(sizeof(handle->buffer), 1,
											   handle->buffer, &handle->buf);
	assert(handle->handle);
	handle->fd = fd;
	int ret = pthread_mutex_init(&handle->mutex, NULL);
	assert(ret == 0);
	ret = pthread_cond_init(&handle->cond, NULL);
	assert(ret == 0);

	pthread_t tid;
	/*
	 * Create the independent pthreads with all the signals masked,
	 * so that FreeRTOS does not interfere with them.
	 */
	sigset_t set;
	sigfillset(&set);
	sigset_t old;
	pthread_sigmask(SIG_SETMASK, &set, &old);

	if (read_or_write) {
		pthread_create(&tid, NULL, writer, handle);
	} else {
		pthread_create(&tid, NULL, reader, handle);
	}

	/*
	 * Restore signal mask of this thread
	 */
	pthread_sigmask(SIG_SETMASK, &old, NULL);
}

int erpc_esp_host_posix_read(erpc_esp_host_posix_io *handle, void *data,
							 size_t size) {
	pthread_mutex_lock(&handle->mutex);
	size_t read = xStreamBufferReceive(handle->handle, data, size, 0);
	if (read == 0) {
		pthread_cond_wait(&handle->cond, &handle->mutex);
		read = xStreamBufferReceive(handle->handle, data, size, 0);
	}
	pthread_mutex_unlock(&handle->mutex);
	return read;
}

int erpc_esp_host_posix_write(erpc_esp_host_posix_io *handle, const void *data,
							  size_t size) {

	int written = 0;
	int remaining = size;

	assert(remaining > 0);
	while (remaining) {
		pthread_mutex_lock(&handle->mutex);
		size_t sent =
			xStreamBufferSend(handle->handle, data + written, remaining, 0);
		pthread_mutex_unlock(&handle->mutex);
		pthread_cond_signal(&handle->cond);
		assert(remaining >= sent);
		remaining -= sent;
		written += sent;
	}

	return size;
}
