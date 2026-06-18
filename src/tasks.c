/**
 * @file    tasks.c
 * @brief   Implementacion de las tareas FreeRTOS para control PWM por ADC.
 *
 * Una sola funcion de tarea (vTaskPotLED) sirve a los tres canales.
 * La diferenciacion se logra exclusivamente a traves del puntero
 * pvParameters, que apunta a un task_params_t estatico distinto por canal.
 *
 * Ciclo de cada tarea:
 *   - Lee el ADC del canal asignado.
 *   - Escala la lectura (12 bits) al ciclo de trabajo PWM (8 bits).
 *   - Actualiza el LED del canal asignado.
 *   - Cada TASK_LOG_EVERY_N iteraciones imprime estado por UART.
 *   - Se bloquea TASK_PERIOD_MS milisegundos (libera el procesador).
 *
 * Plataforma : ESP32 / FreeRTOS (ESP-IDF 5.x - 6.x)
 * Estandar   : C99
 */

#include "tasks.h"
#include "adc_reader.h"
#include "leds.h"
#include "esp_log.h"

/* ------------------------------------------------------------------ */
/*  Etiqueta de log para este modulo                                   */
/* ------------------------------------------------------------------ */

static const char *TAG = "TASKS";

/* ------------------------------------------------------------------ */
/*  Bloques de parametros estaticos (uno por canal)                    */
/*                                                                     */
/*  Se declaran static para garantizar que su vida util supera la del  */
/*  stack de tasks_create_all(). Las tareas leen estos datos durante   */
/*  toda su ejecucion.                                                 */
/* ------------------------------------------------------------------ */

static const task_params_t params_ch0 = {
    .adc_channel = 0,
    .led_channel = 0,
    .name        = "POT0-LED0",
};

static const task_params_t params_ch1 = {
    .adc_channel = 1,
    .led_channel = 1,
    .name        = "POT1-LED1",
};

static const task_params_t params_ch2 = {
    .adc_channel = 2,
    .led_channel = 2,
    .name        = "POT2-LED2",
};

/* ------------------------------------------------------------------ */
/*  Handles de tarea (necesarios para uxTaskGetStackHighWaterMark)     */
/* ------------------------------------------------------------------ */

static TaskHandle_t s_task_handles[3] = { NULL, NULL, NULL };

/* ------------------------------------------------------------------ */
/*  Funcion de tarea                                                   */
/* ------------------------------------------------------------------ */

void vTaskPotLED(void *pvParameters)
{
    /* --- Obtener configuracion del canal via pvParameters --- */
    const task_params_t *cfg = (const task_params_t *)pvParameters;

    uint32_t cycle_count = 0U;   /* Contador de ciclos para log periodico */

    ESP_LOGI(TAG, "[%s] Tarea iniciada (ADC ch%u -> LED ch%u)",
             cfg->name, cfg->adc_channel, cfg->led_channel);

    for (;;)
     {
        /* Leer potenciometro: valor raw de 12 bits [0, 4095] */
        uint16_t raw =adc_reader_get_raw(cfg->adc_channel);

        /* Escalar a duty PWM de 8 bits [0, 255] */
        uint8_t duty= adc_reader_to_duty(raw);


        /* Aplicar brillo al LED correspondiente */
        leds_set_duty((led_channel_t)cfg->led_channel, duty);

        /* 4. Imprimir estado periodicamente (cada TASK_LOG_EVERY_N ciclos),raw, duty, name
         *    para no saturar el puerto UART */
        cycle_count++;
        if (cycle_count % TASK_LOG_EVERY_N == 0)
        {
uint32_t heap = esp_get_free_heap_size();
UBaseType_t wm = uxTaskGetStackHighWaterMark(NULL);

ESP_LOGI(TAG, "[%s] raw=%4u duty=%3u heap=%u wm=%u",
         cfg->name,
         (unsigned int)raw,
         (unsigned int)duty,
         (unsigned int)heap,
         (unsigned int)wm);
        }
        

        /*  Bloquear la tarea durante el periodo configurado.
         *    Durante este tiempo el planificador puede ejecutar
         *    otras tareas (multitarea cooperativa-apropiativa). */
           vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
    }

    /* Nunca se llega aqui, pero se deja por buena practica de programación :) */
    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
/*  Creacion de las tres instancias                                    */
/* ------------------------------------------------------------------ */

void tasks_create_all(void)
{
    BaseType_t ret;

    /* Task Create Canal 0 - Prioridad 1 (menor) */
    /*
        Example:
    
        ret = xTaskCreate(vTaskPotLED,
                      "PotLED_0",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch0,    //pvParameters -> params_ch0 
                      1,
                      &s_task_handles[0]);
        configASSERT(ret == pdPASS);
    */
        ret = xTaskCreate(vTaskPotLED,
                      "PotLED_0",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch0,    //pvParameters -> params_ch0 
                      1,
                      &s_task_handles[0]);
        configASSERT(ret == pdPASS);

   /* TODO  Task Create Canal 1 - Prioridad 2 (menor) */
         ret = xTaskCreate(vTaskPotLED,
                      "PotLED_1",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch1,    //pvParameters -> params_ch0 
                      2,
                      &s_task_handles[1]);
        configASSERT(ret == pdPASS);

    /* TODO Task Create Canal 2 - Prioridad 3 (mayor) */
        ret = xTaskCreate(vTaskPotLED,
                      "PotLED_2",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch2,    //pvParameters -> params_ch0 
                      3,
                      &s_task_handles[2]);
        configASSERT(ret == pdPASS);

    ESP_LOGI("TASKS", "Tres tareas creadas correctamente");
}