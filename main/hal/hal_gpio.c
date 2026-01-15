#include "hal_gpio.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define TAG "HAL_GPIO"

/* =========================================================
 * Internal ESP-IDF GPIO context
 * ========================================================= */
typedef struct
{
    gpio_num_t pin;
    hal_callback_t callback;
    void *context;
} esp_gpio_ctx_t;

/* =========================================================
 * GPIO ISR handler (ESP-IDF)
 * ========================================================= */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    esp_gpio_ctx_t *ctx = (esp_gpio_ctx_t *)arg;

    if (!ctx || !ctx->callback)
        return;

    static uint8_t level; // ISR-safe (no allocation)
    level = gpio_get_level(ctx->pin);

    ctx->callback(
        ctx,               /* bus_handle */
        HAL_EVENT_RX_DATA, /* event type */
        &level,            /* data -> GPIO level */
        sizeof(level),     /* data_len */
        ctx->context       /* user context */
    );
}

/* =========================================================
 * HAL API IMPLEMENTATIONS
 * ========================================================= */

int hal_gpio_initialize(hal_gpio_handle_t *handle)
{
    if (!handle || handle->config.pin < 0)
        return GCD_HAL_GPIO_ERR_INVALID_ARG;

    esp_gpio_ctx_t *ctx = calloc(1, sizeof(esp_gpio_ctx_t));
    if (!ctx)
        return GCD_HAL_GPIO_ERR_HW_FAILURE;

    ctx->pin = (gpio_num_t)handle->config.pin;
    ctx->callback = NULL;
    ctx->context = handle->config.context;

    gpio_config_t cfg = {0};

    cfg.pin_bit_mask = (1ULL << ctx->pin);
    cfg.mode = (handle->config.direction == HAL_GPIO_DIR_OUTPUT)
                   ? GPIO_MODE_OUTPUT
                   : GPIO_MODE_INPUT;

    cfg.pull_up_en = (handle->config.pull == HAL_GPIO_PULL_UP);
    cfg.pull_down_en = (handle->config.pull == HAL_GPIO_PULL_DOWN);

    cfg.intr_type = GPIO_INTR_DISABLE;

    esp_err_t ret = gpio_config(&cfg);
    if (ret != ESP_OK)
    {
        free(ctx);
        return GCD_HAL_GPIO_ERR_HW_FAILURE;
    }

    if (handle->config.direction == HAL_GPIO_DIR_OUTPUT)
    {
        gpio_set_level(ctx->pin, handle->config.init_level ? 1 : 0);
    }

    handle->gpio_handle = ctx;
    return GCD_HAL_GPIO_OK;
}

int hal_gpio_shutdown(hal_gpio_handle_t *handle)
{
    if (!handle || !handle->gpio_handle)
        return GCD_HAL_GPIO_ERR_NOT_INIT;

    esp_gpio_ctx_t *ctx = (esp_gpio_ctx_t *)handle->gpio_handle;

    gpio_reset_pin(ctx->pin);
    free(ctx);
    handle->gpio_handle = NULL;

    return GCD_HAL_GPIO_OK;
}

int hal_gpio_write(hal_gpio_handle_t *handle, bool level)
{
    if (!handle || !handle->gpio_handle)
        return GCD_HAL_GPIO_ERR_NOT_INIT;

    esp_gpio_ctx_t *ctx = (esp_gpio_ctx_t *)handle->gpio_handle;
    gpio_set_level(ctx->pin, level ? 1 : 0);

    return GCD_HAL_GPIO_OK;
}

int hal_gpio_read(hal_gpio_handle_t *handle, bool *level)
{
    if (!handle || !handle->gpio_handle || !level)
        return GCD_HAL_GPIO_ERR_INVALID_ARG;

    esp_gpio_ctx_t *ctx = (esp_gpio_ctx_t *)handle->gpio_handle;
    *level = gpio_get_level(ctx->pin) ? true : false;

    return GCD_HAL_GPIO_OK;
}

int hal_gpio_toggle(hal_gpio_handle_t *handle)
{
    if (!handle || !handle->gpio_handle)
        return GCD_HAL_GPIO_ERR_NOT_INIT;

    bool level;
    hal_gpio_read(handle, &level);
    hal_gpio_write(handle, !level);

    return GCD_HAL_GPIO_OK;
}

int hal_gpio_register_callback(hal_gpio_handle_t *handle,
                               hal_callback_t callback,
                               void *context)
{
    if (!handle || !handle->gpio_handle)
        return GCD_HAL_GPIO_ERR_NOT_INIT;

    esp_gpio_ctx_t *ctx = (esp_gpio_ctx_t *)handle->gpio_handle;
    ctx->callback = callback;
    ctx->context = context;

    gpio_int_type_t intr;

    switch (handle->config.irq_trigger)
    {
    case HAL_GPIO_IRQ_RISING:
        intr = GPIO_INTR_POSEDGE;
        break;
    case HAL_GPIO_IRQ_FALLING:
        intr = GPIO_INTR_NEGEDGE;
        break;
    case HAL_GPIO_IRQ_BOTH:
        intr = GPIO_INTR_ANYEDGE;
        break;
    default:
        intr = GPIO_INTR_DISABLE;
        break;
    }

    gpio_set_intr_type(ctx->pin, intr);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(ctx->pin, gpio_isr_handler, ctx);

    return GCD_HAL_GPIO_OK;
}