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
/*  Se declaran static const para garantizar que su vida util supera   */
/*  la del stack de tasks_create_all(). Las tareas leen estos datos    */
/*  durante toda su ejecucion. Si fueran locales, al salir de la       */
/*  funcion de creacion, los punteros quedarian colgando (dangling).   */
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

/**
 * @brief Función genérica de tarea para controlar un LED por potenciómetro.
 * 
 * Esta función se ejecuta en 3 instancias diferentes. Cada instancia recibe
 * una estructura de configuración a través de pvParameters que indica qué
 * canal ADC debe leer y qué canal LED debe controlar.
 * 
 * @param pvParameters Puntero a una estructura task_params_t (configuración).
 */
void vTaskPotLED(void *pvParameters)
{
    /* --- 1. Obtener configuracion del canal via pvParameters --- */
    const task_params_t *cfg = (const task_params_t *)pvParameters;

    uint32_t cycle_count = 0U;   /* Contador de ciclos para log periodico */

    ESP_LOGI(TAG, "[%s] Tarea iniciada (ADC ch%u -> LED ch%u)",
             cfg->name, cfg->adc_channel, cfg->led_channel);

    /* --- 2. Bucle infinito de la tarea --- */
    for (;;)
    {
        /* 2.1 Leer potenciometro: valor raw de 12 bits [0, 4095] */
        uint16_t raw = adc_reader_get_raw(cfg->adc_channel);

        /* 2.2 Escalar a duty PWM de 8 bits [0, 255] 
           (La función adc_reader_to_duty hace la conversión matemática) */
        uint8_t duty = adc_reader_to_duty(raw);

        /* 2.3 Aplicar brillo al LED correspondiente */
        leds_set_duty((led_channel_t)cfg->led_channel, duty);

        /* 2.4 Imprimir estado periodicamente (cada TASK_LOG_EVERY_N ciclos)
         *     para no saturar el puerto UART y permitir depuración. */
        cycle_count++;
        if (cycle_count % TASK_LOG_EVERY_N == 0)
        {
            // Obtener la cantidad de heap (RAM dinámica) libre en el sistema
            uint32_t heap = esp_get_free_heap_size();
            
            // Obtener el "Watermark" de pila de la tarea actual.
            // Indica cuánta pila ha sido realmente utilizada (cuanto menor el valor, más pila usó).
            UBaseType_t wm = uxTaskGetStackHighWaterMark(NULL);

            ESP_LOGI(TAG, "[%s] raw=%4u duty=%3u heap=%u wm=%u",
                     cfg->name,
                     (unsigned int)raw,
                     (unsigned int)duty,
                     (unsigned int)heap,
                     (unsigned int)wm);
        }

        /* 2.5 Bloquear la tarea durante el periodo configurado.
         *     Durante este tiempo el planificador puede ejecutar
         *     otras tareas (multitarea cooperativa-apropiativa).
         *     Esto es clave para que el CPU no se quede atascado en un bucle ocupado.
         */
        vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD_MS));
    }

    /* Nunca se llega aqui, pero se deja por buena practica de programación :) */
    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
/*  Creacion de las tres instancias                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Crea y lanza las tres tareas de control de LEDs.
 * 
 * Cada tarea recibe un parámetro de configuración diferente (params_ch0, ch1, ch2),
 * pero ejecuta la misma función vTaskPotLED. Esto demuestra la flexibilidad de pvParameters.
 */
void tasks_create_all(void)
{
    BaseType_t ret;

    /* Crear Tarea Canal 0 - Prioridad 1 (menor) */
    ret = xTaskCreate(vTaskPotLED,               // Función de la tarea
                      "PotLED_0",                // Nombre amigable
                      TASK_STACK_SIZE_WORDS,     // Tamaño de la pila en palabras (4 bytes)
                      (void *)&params_ch0,       // pvParameters (Configuración del canal 0)
                      1,                        // Prioridad (1 es la más baja de las 3)
                      &s_task_handles[0]);       // TaskHandle_t donde guardar el identificador
    configASSERT(ret == pdPASS); // Verificar que la tarea se creó correctamente

    /* Crear Tarea Canal 1 - Prioridad 2 (media) */
    ret = xTaskCreate(vTaskPotLED,
                      "PotLED_1",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch1,
                      2,                         // Prioridad 2 (media)
                      &s_task_handles[1]);
    configASSERT(ret == pdPASS);

    /* Crear Tarea Canal 2 - Prioridad 3 (mayor) */
    ret = xTaskCreate(vTaskPotLED,
                      "PotLED_2",
                      TASK_STACK_SIZE_WORDS,
                      (void *)&params_ch2,
                      3,                         // Prioridad 3 (la más alta)
                      &s_task_handles[2]);
    configASSERT(ret == pdPASS);

    ESP_LOGI("TASKS", "Tres tareas creadas correctamente");
}