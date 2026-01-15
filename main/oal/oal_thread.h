#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * @file oal_thread.h
 * @brief OS Abstraction Layer - Thread Management API
 *
 * This module provides platform-independent thread creation, management,
 * and status querying interfaces.
 */

/**
 * @defgroup OAL_Thread_Config Thread Configuration Macros
 * @brief Default configuration values
 * @{
 */

/// Default thread stack size in bytes
#define DEFAULT_OAL_THREAD_STACK_SIZE (2 * 1024U)

/// Default thread priority
#define DEFAULT_OAL_THREAD_PRIORITY (5U)

/** @} */

/**
 * @defgroup OAL_Thread_Types Thread Types
 * @brief Data types for thread abstraction
 * @{
 */

/// Opaque thread handle (OS-specific)
typedef void *OAL_ThreadHandle_t;

/// Thread function signature
typedef void (*OAL_ThreadFunction_t)(void *args);

/**
 * @brief Enumeration for thread execution state
 */
typedef enum
{
    OAL_THREAD_NOT_STARTED = 0, /**< Thread has not started */
    OAL_THREAD_RUNNING,         /**< Thread is currently running */
    OAL_THREAD_COMPLETED,       /**< Thread finished execution */
    OAL_THREAD_FAILED           /**< Thread terminated with error */
} OAL_ThreadStatus_t;

/**
 * @brief Thread descriptor object
 */
typedef struct
{
    const char *name;              /**< Name of the thread */
    uint32_t stack_size;           /**< Stack size in bytes */
    uint8_t priority;              /**< Thread priority */
    OAL_ThreadFunction_t function; /**< Thread entry function */
    void *args;                    /**< User argument to the thread function */
    OAL_ThreadHandle_t handle;     /**< OS-specific thread handle */
    OAL_ThreadStatus_t status;     /**< Current thread execution status */
} OAL_Thread_t;

/** @} */ // End of OAL_Thread_Types

/**
 * @defgroup OAL_Thread_API Thread API
 * @brief Public API for thread lifecycle management
 * @{
 */

/**
 * @brief Create a default-initialized thread object.
 *
 * @param[in]  name     Thread name
 * @param[in]  function Pointer to thread function
 * @param[in]  args     Argument passed to thread function
 *
 * @return Initialized OAL_Thread_t object
 */
OAL_Thread_t OAL_Thread_CreateDefault(const char *name,
                                      OAL_ThreadFunction_t function,
                                      void *args);

/**
 * @brief Initialize a thread object manually.
 *
 * @param[out] thread     Pointer to thread object
 * @param[in]  name       Thread name
 * @param[in]  stack_size Stack size in bytes
 * @param[in]  priority   Thread priority
 */
void OAL_Thread_Init(OAL_Thread_t *thread,
                     const char *name,
                     uint32_t stack_size,
                     uint8_t priority);

/**
 * @brief Set the entry function and context argument for a thread.
 *
 * @param[in,out] thread Pointer to thread object
 * @param[in]     func   Thread function
 * @param[in]     args   Argument to pass to the function
 */
void OAL_Thread_SetTask(OAL_Thread_t *thread,
                        OAL_ThreadFunction_t func,
                        void *args);

/**
 * @brief Start the thread execution.
 *
 * @param[in,out] thread Pointer to thread object
 *
 * @return true on success, false on failure
 */
bool OAL_Thread_Start(OAL_Thread_t *thread);

/**
 * @brief Destroy the thread and release resources.
 *
 * @param[in,out] thread Pointer to thread object
 */
void OAL_Thread_Destroy(OAL_Thread_t *thread);

/**
 * @brief Check if the thread is currently running.
 *
 * @param[in] thread Pointer to thread object
 *
 * @return true if running, false otherwise
 */
bool OAL_Thread_IsRunning(const OAL_Thread_t *thread);

/**
 * @brief Get the thread execution status.
 *
 * @param[in] thread Pointer to thread object
 *
 * @return Current execution status
 */
OAL_ThreadStatus_t OAL_Thread_GetStatus(const OAL_Thread_t *thread);

/**
 * @brief Get the underlying thread handle.
 *
 * @param[in] thread Pointer to thread object
 *
 * @return Thread handle (opaque pointer)
 */
OAL_ThreadHandle_t OAL_Thread_GetHandle(const OAL_Thread_t *thread);

/**
 * @brief Get the thread name.
 *
 * @param[in] thread Pointer to thread object
 *
 * @return Pointer to the thread name string
 */
const char *OAL_Thread_GetName(const OAL_Thread_t *thread);

/**
 * @brief Set the thread's runtime priority.
 *
 * @param[in,out] thread   Pointer to thread object
 * @param[in]     priority New priority value
 */
void OAL_Thread_SetPriority(OAL_Thread_t *thread, uint8_t priority);

/**
 * @brief Get the thread’s runtime priority.
 *
 * @param[in] thread Pointer to thread object
 *
 * @return Current priority value
 */
uint8_t OAL_Thread_GetPriority(const OAL_Thread_t *thread);

/** @} */ // End of OAL_Thread_API