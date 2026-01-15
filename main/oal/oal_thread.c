#include "oal_thread.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

/*-----------------------------------------------------------
 * Internal thread trampoline wrapper
 *----------------------------------------------------------*/
static void OAL_Thread_Trampoline(void *param)
{
    OAL_Thread_t *thread = (OAL_Thread_t *)param;

    if (!thread || !thread->function)
    {
        if (thread)
            thread->status = OAL_THREAD_FAILED;

        vTaskDelete(NULL);
        return;
    }

    thread->status = OAL_THREAD_RUNNING;

    /* Call user task */
    thread->function(thread->args);

    /* Mark completed */
    thread->status = OAL_THREAD_COMPLETED;

    /* Self delete */
    vTaskDelete(NULL);
}

/*-----------------------------------------------------------
 * Create default thread object
 *----------------------------------------------------------*/
OAL_Thread_t OAL_Thread_CreateDefault(const char *name,
                                      OAL_ThreadFunction_t function,
                                      void *args)
{
    OAL_Thread_t thread;

    thread.name = name;
    thread.stack_size = DEFAULT_OAL_THREAD_STACK_SIZE;
    thread.priority = DEFAULT_OAL_THREAD_PRIORITY;
    thread.function = function;
    thread.args = args;
    thread.handle = NULL;
    thread.status = OAL_THREAD_NOT_STARTED;

    return thread;
}

/*-----------------------------------------------------------
 * Initialize thread manually
 *----------------------------------------------------------*/
void OAL_Thread_Init(OAL_Thread_t *thread,
                     const char *name,
                     uint32_t stack_size,
                     uint8_t priority)
{
    if (!thread)
        return;

    thread->name = name;
    thread->stack_size = stack_size;
    thread->priority = priority;
    thread->function = NULL;
    thread->args = NULL;
    thread->handle = NULL;
    thread->status = OAL_THREAD_NOT_STARTED;
}

/*-----------------------------------------------------------
 * Set thread function and argument
 *----------------------------------------------------------*/
void OAL_Thread_SetTask(OAL_Thread_t *thread,
                        OAL_ThreadFunction_t func,
                        void *args)
{
    if (!thread)
        return;

    thread->function = func;
    thread->args = args;
}

/*-----------------------------------------------------------
 * Start thread
 *----------------------------------------------------------*/
bool OAL_Thread_Start(OAL_Thread_t *thread)
{
    if (!thread || !thread->function)
        return false;

    if (thread->handle != NULL)
        return false; /* already started */

    BaseType_t result = xTaskCreate(
        OAL_Thread_Trampoline,
        thread->name ? thread->name : "oal_thread",
        thread->stack_size / sizeof(StackType_t), /* stack in words */
        thread,
        thread->priority,
        (TaskHandle_t *)&thread->handle);

    if (result != pdPASS)
    {
        thread->status = OAL_THREAD_FAILED;
        return false;
    }

    return true;
}

/*-----------------------------------------------------------
 * Destroy thread
 *----------------------------------------------------------*/
void OAL_Thread_Destroy(OAL_Thread_t *thread)
{
    if (!thread || !thread->handle)
        return;

    vTaskDelete((TaskHandle_t)thread->handle);
    thread->handle = NULL;
    thread->status = OAL_THREAD_NOT_STARTED;
}

/*-----------------------------------------------------------
 * Is thread running
 *----------------------------------------------------------*/
bool OAL_Thread_IsRunning(const OAL_Thread_t *thread)
{
    if (!thread || !thread->handle)
        return false;

    eTaskState state = eTaskGetState((TaskHandle_t)thread->handle);
    return (state == eRunning || state == eReady || state == eBlocked);
}

/*-----------------------------------------------------------
 * Get status
 *----------------------------------------------------------*/
OAL_ThreadStatus_t OAL_Thread_GetStatus(const OAL_Thread_t *thread)
{
    if (!thread)
        return OAL_THREAD_FAILED;

    return thread->status;
}

/*-----------------------------------------------------------
 * Get handle
 *----------------------------------------------------------*/
OAL_ThreadHandle_t OAL_Thread_GetHandle(const OAL_Thread_t *thread)
{
    if (!thread)
        return NULL;

    return thread->handle;
}

/*-----------------------------------------------------------
 * Get name
 *----------------------------------------------------------*/
const char *OAL_Thread_GetName(const OAL_Thread_t *thread)
{
    if (!thread)
        return NULL;

    return thread->name;
}

/*-----------------------------------------------------------
 * Set priority at runtime
 *----------------------------------------------------------*/
void OAL_Thread_SetPriority(OAL_Thread_t *thread, uint8_t priority)
{
    if (!thread)
        return;

    thread->priority = priority;

    if (thread->handle)
    {
        vTaskPrioritySet((TaskHandle_t)thread->handle, priority);
    }
}

/*-----------------------------------------------------------
 * Get runtime priority
 *----------------------------------------------------------*/
uint8_t OAL_Thread_GetPriority(const OAL_Thread_t *thread)
{
    if (!thread)
        return 0;

    if (thread->handle)
    {
        return uxTaskPriorityGet((TaskHandle_t)thread->handle);
    }

    return thread->priority;
}