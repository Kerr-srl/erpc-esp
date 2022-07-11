# Host ESP-IDF app

Example of building an ESP-IDF app for the host platform.

## Usage

Check the documentation of the [erpc](../../erpc_esp/erpc/README.md) component.
Check the documentation of the [erpc_tinyproto](../../erpc_esp/erpc_tinyproto/README.md) component.

This example works as follows:

* The ESP-IDF application uses:
    * stdout as channel to exchange eRPC data with the Python script.
    * stderr to print logs.
* The Python script interacts with the ESP-IDF application by:
    * sending eRPC data to stdin of the ESP-IDF application (via PIPE);
    * receiving eRPC data from stdout of the ESP-IDF application (via PIPE);
    * using stdout to print logs of the Python script itself;
    * redirecting stderr of the ESP-IDF application to its own stderr, so that when necessary logs from the Python script and logs from the ESP-IDF application can be monitored separately.
* Note that choosing stderr on the ESP-IDF application to print logs is intentional. There are two reasons:
    * `assert()` abortions in C are communicated via stderr. We want to see those too.
    * If we want to monitor separately the ESP-IDF application's logs and the ones from the Python script, the easiest way is to redirect the stderr of the Python script (to which the stderr of the ESP-IDF application is redirected) to a file, as shown below. However, if we used stdout for ESP-IDF application's logs, then redirecting it to a file (i.e. redirecting the Python script's stderr, to which the stdout of the ESP-IDF application would be redirected, to a file) would result in seeing buffered logs, but most of the times we want to see logs in realtime.

```bash
# Build the project
$ idf.py build
# Communicate with the built firmware using Python
$ python main/main.py build/host.elf
# Communicate with the built firmware using Python, but look at the logs
# separately
$ python main/main.py build/host.elf 2> firmware.log
# In another shell
$ tail -f firmware.log
```

## Application notes

### What this example offers

* This example provides in its [components](./components/) directory several components that are useful when trying to build an ESP-IDF app for the host platform:
    * freertos: overrides the default freertos from ESP-IDF with the FreeRTOS POSIX/Linux simulator provided in [FreeRTOS-Kernel](https://github.com/FreeRTOS/FreeRTOS-Kernel).
        * Note that ESP-IDF provides an non-standard FreeRTOS. See [ESP-IDF FreeRTOS (SMP)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/freertos-smp.html) and [FreeRTOS Supplemental Features](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_additions.html). You may need to write your own adapter layer for the incompatibilities.
    * tinyproto: overrides the default tinyproto component. It almost identical, but it forces to use of ESP32 HAL port, instead of using the Linux HAL port (which makes use of pthread synchronization primitives).
    * esp_timer: overrides the default esp_timer component. Implements only `esp_timer_get_time()`, which is used by tinyproto.
    * log: overrides the default `log` component from ESP-IDF with a version that works with the FreeRTOS Linux simulator.
    * posix_io: provides utility code to work with blocking I/O when using FreeRTOS Linux simulator.

### FreeRTOS Linux simulator pitfalls

* Check out the implementation details of the [FreeRTOS Linux simulator](https://www.freertos.org/FreeRTOS-simulator-for-Linux.html).
* One gist of this documentation article is that in a program that uses FreeRTOS Linux simulator we sort of have two separate concurrency worlds: the underlying pthreads created via FreeRTOS (`xTaskCreate`), whose scheduling is managed by FreeRTOS and normal pthreads, which FreeRTOS is not aware of and can be used to simulate interrupts.
    * While normal pthreads can be used to simulate interrupts, we need to consider the fact that these two worlds execute concurrently, contrary to what is common on many single core MCUs, where, if the ISR is executing, we can be sure that the tasks are not executing.
    * Furthermore, we need to know that FreeRTOS POSIX simulator's critical section is implemented as simply masking all the signals. This is enough as a critical section mechanism *in the FreeRTOS world*. But e.g. entering in a critical section in a FreeRTOS task can't prevent a normal pthread from running.
    * Note that running normal FreeRTOS (not the Linux simulator) on a SMP processor actually poses the same problems.
* Another thing that we can derive from this article is that pthread synchronization primitives are not "recognized" by FreeRTOS. E.g. blocking on a `pthread_mutex` in a FreeRTOS task does not lead FreeRTOS to context switch to another task.
    * This could lead to deadlocks.
        * E.g. Task A has priority 1 and Task B priority 2. Task A takes a `pthread_mutex`, but then Task B wakes up for some reason and, having higher priority than Task A, FreeRTOS context switches to Task B. Task B also tries to take the `pthread_mutex`, but it is taken by Task A. Since FreeRTOS doesn't "recognized" `pthread_mutex` as a concurrency primitive, Task B just keeps executing, i.e. Task B just keeps being blocked waiting for `pthread_mutex` (NOTE: FreeRTOS doesn't know that Task B is blocked). Task A won't have chance to run because Task B has higher priority. Thus, we have a deadlock.
    * Therefore, anywhere pthread synchronization primitives are used, we need to wrap them with a layer of FreeRTOS synchronization primitives.
* Problem with esp_log:
    * This example uses ESP-IDF's log component.
        * Under the hood `esp_log` uses `vprintf()`. Some implementation of `vprintf()` make use of mutexes to ensure thread-safety. As we said, `pthread_mutex` doesn't play well with FreeRTOS. Therefore we need to replace using `esp_log_set_vprintf` the default `vprintf` function with a `vprintf` function wrapped around FreeRTOS mutexes.
        * `esp_log` also needs another lock, which for the Linux port is implemented using `pthread_mutex`. Again the same problem. To solve this, we override the `log` component with our slightly modified version (see [log](./components/log)), which implements the locks using FreeRTOS mutex.
* Problem with blocking I/O:
    * This example uses stdin as channel to receive eRPC inputs. A common way to read from stdin is to use the POSIX API `read()`, which we know is by default blocking. But based on what we read about FreeRTOS Linux simulator, we know that, while `read()` is blocking, the fact that it is blocking is not "recognized" by FreeRTOS POSIX port, which means that blocking on `read()` does not lead FreeRTOS to context switch to other runnable tasks.
    * Therefore, if we want to implement blocking I/O, we need to use the primitives provided by FreeRTOS. [Stream buffer](https://www.freertos.org/RTOS-stream-buffer-example.html) is a good fit for such use case. What we do is to have a separate thread (not managed by FreeRTOS) that takes care of actually performing I/O using POSIX I/O APIs and then pass the results from the pthread world to the FreeRTOS world via stream buffer.
        * In a sense the pthread that actually performs I/O can be thought as an interrupt handler, which passes I/O data to FreeRTOS tasks via synchronization primitives such as stream buffer in this case.
        * But, as we said above, FreeRTOS critical sections are not sufficient to protect the two worlds from race conditions. Therefore, we need to use also `pthread_mutex` to protect access to data that is shared between these two worlds, i.e. the stream buffer.
    * [posix_io](./components/posix_io/) contains the solution that we describe for blocking I/O.
