#ifndef ERPC_CONFIG_OVERRIDE_H_
#define ERPC_CONFIG_OVERRIDE_H_

/**
 * Force use of standard assert.
 * Without this erpc headers try to include FreeRTOS.h and use `configASSERT`.
 * But in ESP-IDF FreeRTOS.h must be included as "freertos/FreeRTOS.h".
 * True, we could make it work with a bit of effort, but for now we want
 * to use the standard `assert` anyway.
 */
#if !defined(erpc_assert)
#ifdef __cplusplus
#include <cassert>
#else
#include "assert.h"
#endif
#define erpc_assert(condition) assert(condition)
#endif

#endif /* ifndef ERPC_CONFIG_OVERRIDE_H_  */
