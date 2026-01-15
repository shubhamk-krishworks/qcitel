#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file oal_queue.h
 * @brief OS Abstraction Layer - Queue API
 *
 * This module provides a platform-independent interface for creating,
 * managing, and interacting with queues across ISR and task contexts.
 */

/**
 * @defgroup OAL_Queue_Config Queue Configuration Macros
 * @brief Default queue parameters
 * @{
 */

/// Default maximum queue length
#define DEFAULT_OAL_QUEUE_LENGTH (16U)

/// Default item size in bytes
#define DEFAULT_OAL_QUEUE_ITEM_SIZE (4U)

/** @} */

/**
 * @defgroup OAL_Queue_Types Queue Data Types
 * @brief Structures and handles used in the queue module
 * @{
 */

/// Opaque handle to an OS-specific queue object
typedef void *OAL_QueueHandle_t;

/**
 * @brief Queue object descriptor
 */
typedef struct
{
    uint32_t length;              /**< Maximum number of items the queue can hold */
    uint32_t item_size;           /**< Size of each item in bytes */
    bool isr_enabled;             /**< Indicates whether the queue is used in an ISR context */
    OAL_QueueHandle_t handle;     /**< OS-specific queue handle */
    volatile uint32_t fill_count; /**< Current number of items in the queue (optional tracking) */
} OAL_Queue_t;

/** @} */

/**
 * @defgroup OAL_Queue_API Queue Public API
 * @brief Functions for managing and accessing queues
 * @{
 */

/**
 * @brief Create and initialize a queue with default settings.
 *
 * @param[out] queue Pointer to queue object to initialize
 *
 * @return 0 on success, or negative error code on failure
 */
int32_t OAL_Queue_CreateDefault(OAL_Queue_t *queue);

/**
 * @brief Initialize a queue object with custom length, item size, and ISR usage flag.
 *
 * @param[out] queue        Pointer to queue object
 * @param[in]  item_size    Size of each item in bytes
 * @param[in]  length       Maximum number of items
 * @param[in]  isr_enabled  true to enable ISR-safe behavior, false otherwise
 *
 * @return 0 on success, or negative error code on failure
 */
int32_t OAL_Queue_Create(OAL_Queue_t *queue, uint32_t item_size, uint32_t length, bool isr_enabled);

/**
 * @brief Destroy the queue and release all associated resources.
 *
 * @param[in,out] queue Pointer to queue object
 */
void OAL_Queue_Destroy(OAL_Queue_t *queue);

/**
 * @brief Push an item to the queue (non-blocking).
 *
 * @param[in] queue Pointer to queue object
 * @param[in] item  Pointer to item data
 *
 * @return 0 on success, or -1 on failure
 */
int32_t OAL_Queue_Push(const OAL_Queue_t *queue, const void *item);

/**
 * @brief Pop an item from the queue (non-blocking).
 *
 * @param[in]  queue     Pointer to queue object
 * @param[out] out_item  Pointer to receive dequeued item
 *
 * @return Size of item popped on success, -1 if queue is empty
 */
int32_t OAL_Queue_Pop(const OAL_Queue_t *queue, void *out_item);

/**
 * @brief Wait and pop item from the queue (blocks up to internal timeout).
 *
 * This function waits for data to become available in the queue. It blocks the calling task
 * for a fixed timeout period defined internally (e.g., 5000 ms). Not valid for ISR context.
 *
 * @param[in]  queue     Pointer to queue object
 * @param[out] out_item  Pointer to receive dequeued item
 *
 * @return Size of item popped on success, -1 on timeout or failure
 */
int32_t OAL_Queue_WaitPop(const OAL_Queue_t *queue, void *out_item);

/**
 * @brief Peek at the front item without removing it (non-blocking).
 *
 * @param[in]  queue     Pointer to queue object
 * @param[out] out_item  Pointer to receive item data
 *
 * @return Size of item peeked on success, -1 if queue is empty
 */
int32_t OAL_Queue_Peek(const OAL_Queue_t *queue, void *out_item);

/**
 * @brief Get the number of items currently in the queue.
 *
 * @param[in] queue Pointer to queue object
 *
 * @return Number of items currently in the queue
 */
uint32_t OAL_Queue_GetSize(const OAL_Queue_t *queue);

/**
 * @brief Check if the queue is full.
 *
 * @param[in] queue Pointer to queue object
 *
 * @return true if full, false otherwise
 */
bool OAL_Queue_IsFull(const OAL_Queue_t *queue);

/**
 * @brief Check if the queue is empty.
 *
 * @param[in] queue Pointer to queue object
 *
 * @return true if empty, false otherwise
 */
bool OAL_Queue_IsEmpty(const OAL_Queue_t *queue);

/** @} */ // End of OAL_Queue_API