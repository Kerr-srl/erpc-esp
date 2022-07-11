#include "erpc_esp/host/posix_io.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/**
 * FreeRTOS already uses SIGUSR1
 */
#define SIG_RX_READY SIGUSR2

/**
 * Structure shared between signal handler and all the posix reader threads.
 * Access synchronization happens via the `sem` semaphore .
 *
 * Who it works:
 *
 * * The semaphore is initially 1.
 * * A reader thread waits for the semaphore then modifies g_rx
 * * The same reader then raises a signal
 * * Until the signal handler posts the signal, all the readers are blocked and
 * can't access g_rx.
 */
static struct {
	pthread_once_t setup_once;
	sem_t sem;
	erpc_esp_host_posix_io *pending_rx;
} g_rx = {
	.setup_once = PTHREAD_ONCE_INIT,
};

/**
 * Think of it as an IRQ handler that "talks" with the FreeRTOS world
 *
 * Only pthreads that are managed by FreeRTOS accept signals, so we are
 * sure that this signal handler is executed "in the FreeRTOS world".
 */
static void rx_ready_signal_handler(int sig) {
	assert(sig == SIG_RX_READY);
	assert(g_rx.pending_rx != NULL);

	BaseType_t high_priority_task_woken;
	xSemaphoreGiveFromISR(g_rx.pending_rx->io.rx.sem.handle,
						  &high_priority_task_woken);

	g_rx.pending_rx = NULL;
	// Finished accessing g_rx, unlock it.
	int ret = sem_post(&g_rx.sem);
	assert(ret == 0);
}

static void raise_process_signal(int signo) {
	/*
	 * Why not `raise()`?  Apparently, `raise()` sends the signal to the current
	 * pthread, while we want to signal to be delivered to the current active
	 * FreeRTOS task, which is the only pthread that has an empty sigmask.
	 */
	kill(getpid(), signo);
}

static void setup_rx_signal_handler() {
	{
		// initialized an unamed semaphore that is not shared among processes
		int ret = sem_init(&g_rx.sem, 0, 1);
		assert(ret == 0);
	}

	struct sigaction sigtick;
	sigtick.sa_flags = 0;
	sigtick.sa_handler = rx_ready_signal_handler;
	sigfillset(&sigtick.sa_mask);
	int ret = sigaction(SIG_RX_READY, &sigtick, NULL);
	assert(ret == 0);
}

static void *writer(void *arg) {
	erpc_esp_host_posix_io *handle = arg;
	uint8_t buffer[256];

	while (1) {
		// TODO can we use portYIELD_FROM_ISR? I don't think so.
		BaseType_t woken;

		pthread_mutex_lock(&handle->mutex);
		size_t n_read = xStreamBufferReceiveFromISR(
			handle->io.tx.fifo.handle, buffer, sizeof(buffer), &woken);
		while (n_read == 0) {
			pthread_cond_wait(&handle->cond, &handle->mutex);
			n_read = xStreamBufferReceiveFromISR(
				handle->io.tx.fifo.handle, buffer, sizeof(buffer), &woken);
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

	while (1) {
		pthread_mutex_lock(&handle->mutex);
		while (handle->io.rx.pending.buffer == NULL) {
			pthread_cond_wait(&handle->cond, &handle->mutex);
		}

		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(handle->fd, &read_set);
		int ret = select(handle->fd + 1, &read_set, NULL, NULL, NULL);
		if (ret < 0) {
			printf("Error [%d]: %s", ret, strerror(errno));
			assert(0);
		}
		ret = read(handle->fd, handle->io.rx.pending.buffer,
				   handle->io.rx.pending.buffer_max_size);
		assert(ret > 0);

		handle->io.rx.pending.actually_read_size = ret;
		// clear pending status
		handle->io.rx.pending.buffer = NULL;
		handle->io.rx.pending.buffer_max_size = 0;
		pthread_mutex_unlock(&handle->mutex);

		// lock g_rx
		ret = sem_wait(&g_rx.sem);
		assert(ret == 0);

		g_rx.pending_rx = handle;
		/*
		 * Raise signal. What happens next is that the signal handler
		 * will access g_rx and call sem_post to unlock it.
		 * We don't need to wait until signal handler finishes.
		 * We can just wait for the next read request, which will be made
		 * only if the signal handler has finished.
		 */
		raise_process_signal(SIG_RX_READY);
	}
	return NULL;
}

void erpc_esp_host_posix_io_init(erpc_esp_host_posix_io *handle, int fd,
								 bool read_or_write) {
	{
		int ret = pthread_once(&g_rx.setup_once, setup_rx_signal_handler);
		assert(ret == 0);
	}

	handle->fd = fd;
	int ret = pthread_mutex_init(&handle->mutex, NULL);
	assert(ret == 0);
	ret = pthread_cond_init(&handle->cond, NULL);
	assert(ret == 0);

	/**
	 * Initialize FreeRTOS objects before masking signals and creating pthread.
	 * FreeRTOS objects' initialization functions may enter and exit in critical
	 * section, which on the POSIX port basically means: mask signals and unmask
	 * them.
	 * So, if we did something like: 1. mask signals; 2. init freertos
	 * object; 3. create pthread.
	 * At step 3 we may have signals unmasked, which is not what we want.
	 */
	if (read_or_write) {
		handle->io.tx.fifo.handle = xStreamBufferCreateStatic(
			sizeof(handle->io.tx.fifo.data_buffer), 1,
			handle->io.tx.fifo.data_buffer, &handle->io.tx.fifo.buf);
		assert(handle->io.tx.fifo.handle);
	} else {
		// set as non-blocking
		fcntl(handle->fd, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
		handle->io.rx.sem.handle =
			xSemaphoreCreateBinaryStatic(&handle->io.rx.sem.buf);
		assert(handle->io.rx.sem.handle);
	}

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
	handle->io.rx.pending.buffer_max_size = size;
	handle->io.rx.pending.buffer = data;
	pthread_mutex_unlock(&handle->mutex);

	// awake reader thread
	pthread_cond_signal(&handle->cond);
	BaseType_t res = xSemaphoreTake(handle->io.rx.sem.handle, portMAX_DELAY);
	assert(res == pdTRUE);

	pthread_mutex_lock(&handle->mutex);
	assert(handle->io.rx.pending.buffer == NULL);
	assert(handle->io.rx.pending.buffer_max_size == 0);
	size_t read_size = handle->io.rx.pending.actually_read_size;
	pthread_mutex_unlock(&handle->mutex);
	return read_size;
}

int erpc_esp_host_posix_write(erpc_esp_host_posix_io *handle, const void *data,
							  size_t size) {
	int written = 0;
	int remaining = size;

	assert(remaining > 0);
	while (remaining) {
		pthread_mutex_lock(&handle->mutex);
		size_t sent = xStreamBufferSend(handle->io.tx.fifo.handle,
										data + written, remaining, 0);
		pthread_mutex_unlock(&handle->mutex);
		pthread_cond_signal(&handle->cond);
		assert(remaining >= sent);
		remaining -= sent;
		written += sent;
	}

	return size;
}
