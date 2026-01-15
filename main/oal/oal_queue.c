#include "oal_queue.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define OAL_QUEUE_WAIT_TIMEOUT_MS (5000U)

#define OAL_QUEUE_MAX_DELAY portMAX_DELAY

/* Convert ms to ticks */
#define OAL_MS_TO_TICKS(ms) pdMS_TO_TICKS(ms)

/* Internal error codes */
#define OAL_OK (0)
#define OAL_ERROR (-1)

/*-----------------------------------------------------------
 * Create queue with default parameters
 *----------------------------------------------------------*/
int32_t OAL_Queue_CreateDefault(OAL_Queue_t *queue)
{
    if (!queue)
        return OAL_ERROR;

    return OAL_Queue_Create(queue,
                            DEFAULT_OAL_QUEUE_ITEM_SIZE,
                            DEFAULT_OAL_QUEUE_LENGTH,
                            false);
}

/*-----------------------------------------------------------
 * Create queue with custom parameters
 *----------------------------------------------------------*/
int32_t OAL_Queue_Create(OAL_Queue_t *queue, uint32_t item_size, uint32_t length, bool isr_enabled)
{
    if (!queue || item_size == 0 || length == 0)
        return OAL_ERROR;

    QueueHandle_t q = xQueueCreate(length, item_size);
    if (!q)
        return OAL_ERROR;

    queue->length = length;
    queue->item_size = item_size;
    queue->isr_enabled = isr_enabled;
    queue->handle = (OAL_QueueHandle_t)q;
    queue->fill_count = 0;

    return OAL_OK;
}

/*-----------------------------------------------------------
 * Destroy queue
 *----------------------------------------------------------*/
void OAL_Queue_Destroy(OAL_Queue_t *queue)
{
    if (!queue || !queue->handle)
        return;

    vQueueDelete((QueueHandle_t)queue->handle);
    queue->handle = NULL;
    queue->fill_count = 0;
}

/*-----------------------------------------------------------
 * Push item (non-blocking)
 *----------------------------------------------------------*/
int32_t OAL_Queue_Push(const OAL_Queue_t *queue, const void *item)
{
    if (!queue || !queue->handle || !item)
        return OAL_ERROR;

    BaseType_t result;
    QueueHandle_t q = (QueueHandle_t)queue->handle;

    if (queue->isr_enabled && xPortInIsrContext())
    {
        BaseType_t higher_prio_task_woken = pdFALSE;
        result = xQueueSendFromISR(q, item, &higher_prio_task_woken);

        if (higher_prio_task_woken)
            portYIELD_FROM_ISR();
    }
    else
    {
        result = xQueueSend(q, item, 0); // non-blocking
    }

    if (result == pdPASS)
    {
        ((OAL_Queue_t *)queue)->fill_count++;
        return OAL_OK;
    }

    return OAL_ERROR;
}

/*-----------------------------------------------------------
 * Pop item (non-blocking)
 *----------------------------------------------------------*/
int32_t OAL_Queue_Pop(const OAL_Queue_t *queue, void *out_item)
{
    if (!queue || !queue->handle || !out_item)
        return OAL_ERROR;

    BaseType_t result;
    QueueHandle_t q = (QueueHandle_t)queue->handle;

    if (queue->isr_enabled && xPortInIsrContext())
    {
        BaseType_t higher_prio_task_woken = pdFALSE;
        result = xQueueReceiveFromISR(q, out_item, &higher_prio_task_woken);

        if (higher_prio_task_woken)
            portYIELD_FROM_ISR();
    }
    else
    {
        result = xQueueReceive(q, out_item, 0); // non-blocking
    }

    if (result == pdPASS)
    {
        ((OAL_Queue_t *)queue)->fill_count--;
        return queue->item_size;
    }

    return OAL_ERROR;
}

/*-----------------------------------------------------------
 * Wait and pop item (blocking, task only)
 *----------------------------------------------------------*/
int32_t OAL_Queue_WaitPop(const OAL_Queue_t *queue, void *out_item)
{
    if (!queue || !queue->handle || !out_item)
        return OAL_ERROR;

    /* Not allowed in ISR */
    if (xPortInIsrContext())
        return OAL_ERROR;

    QueueHandle_t q = (QueueHandle_t)queue->handle;

    BaseType_t result = xQueueReceive(q, out_item, OAL_QUEUE_MAX_DELAY);

    if (result == pdPASS)
    {
        ((OAL_Queue_t *)queue)->fill_count--;
        return queue->item_size;
    }

    return OAL_ERROR;
}

/*-----------------------------------------------------------
 * Peek item (non-blocking)
 *----------------------------------------------------------*/
int32_t OAL_Queue_Peek(const OAL_Queue_t *queue, void *out_item)
{
    if (!queue || !queue->handle || !out_item)
        return OAL_ERROR;

    QueueHandle_t q = (QueueHandle_t)queue->handle;

    BaseType_t result = xQueuePeek(q, out_item, 0);

    if (result == pdPASS)
        return queue->item_size;

    return OAL_ERROR;
}

/*-----------------------------------------------------------
 * Get queue size
 *----------------------------------------------------------*/
uint32_t OAL_Queue_GetSize(const OAL_Queue_t *queue)
{
    if (!queue || !queue->handle)
        return 0;

    QueueHandle_t q = (QueueHandle_t)queue->handle;
    return uxQueueMessagesWaiting(q);
}

/*-----------------------------------------------------------
 * Is full
 *----------------------------------------------------------*/
bool OAL_Queue_IsFull(const OAL_Queue_t *queue)
{
    if (!queue || !queue->handle)
        return false;

    QueueHandle_t q = (QueueHandle_t)queue->handle;
    return (uxQueueSpacesAvailable(q) == 0);
}

/*-----------------------------------------------------------
 * Is empty
 *----------------------------------------------------------*/
bool OAL_Queue_IsEmpty(const OAL_Queue_t *queue)
{
    if (!queue || !queue->handle)
        return true;

    QueueHandle_t q = (QueueHandle_t)queue->handle;
    return (uxQueueMessagesWaiting(q) == 0);
}