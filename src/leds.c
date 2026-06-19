/**
 * @file    leds.c
 * @brief   Implementacion del control PWM para tres LEDs via LEDC (ESP32).
 *
 * Utiliza un unico timer LEDC de alta velocidad compartido entre los tres
 * canales. La resolucion de 8 bits permite 256 niveles de brillo, suficiente
 * para representar la lectura ADC de 12 bits escalada a ese rango.
 *
 * Plataforma : ESP32 / ESP-IDF 5.x - 6.x
 * Estandar   : C99
 */

#include "leds.h"
#include "esp_log.h"

/* ------------------------------------------------------------------ */
/*  Constantes internas                                                */
/* ------------------------------------------------------------------ */

// Etiqueta para los mensajes de log de este módulo
static const char *TAG = "LEDS";

/** Timer LEDC compartido por los tres canales. */
#define LED_TIMER   LEDC_TIMER_0

/** Modo de velocidad del periferico LEDC. */
#define LED_MODE    LEDC_LOW_SPEED_MODE

/* ------------------------------------------------------------------ */
/*  Tabla de configuracion de canales                                  */
/* ------------------------------------------------------------------ */

/** 
 * Asocia cada canal lógico (0, 1, 2) con su GPIO físico y su canal LEDC interno.
 * 
 * Esta tabla centraliza la asignación de hardware, lo que facilita el mantenimiento.
 * Si necesitas cambiar el pin del LED 1, solo modificas esta tabla y el resto del
 * código sigue funcionando sin cambios.
 */
static const struct 
{
    gpio_num_t      gpio;      // El pin GPIO físico al que está conectado el LED
    ledc_channel_t  channel;   // El canal PWM del ESP32 que usará
    
} led_map[LED_CHANNEL_COUNT] = {
    { LED_PIN_0, LEDC_CHANNEL_0 },
    { LED_PIN_1, LEDC_CHANNEL_1 },
    { LED_PIN_2, LEDC_CHANNEL_2 },
};

/* ------------------------------------------------------------------ */
/*  Implementacion                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Inicializa el subsistema PWM para controlar el brillo de los LEDs.
 * 
 * Configura un único Timer LEDC y los 3 canales PWM asociados.
 * Todos los LEDs inician en 0% de brillo (apagados).
 */
void leds_init(void)
{
    /* 1. Configurar el timer LEDC compartido.
       Este timer genera la frecuencia base (PWM) para todos los canales. */
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LED_MODE,           // Velocidad baja (suficiente para LEDs)
        .timer_num       = LED_TIMER,          // Usamos el Timer 0
        .duty_resolution = LED_PWM_RESOLUTION, // 8 bits (0 a 255). Definido en leds.h
        .freq_hz         = LED_PWM_FREQ_HZ,    // Frecuencia del PWM (ej: 5000 Hz)
        .clk_cfg         = LEDC_AUTO_CLK,      // El ESP32 elige automáticamente el reloj más adecuado
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    /* 2. Configurar cada uno de los 3 canales LEDC */
    for (uint8_t i = 0; i < LED_CHANNEL_COUNT; i++) {
        ledc_channel_config_t ch_cfg = {
            .speed_mode = LED_MODE,                 // Misma velocidad que el timer
            .channel    = led_map[i].channel,       // Canal físico del LED (0, 1 o 2)
            .timer_sel  = LED_TIMER,                // Mismo timer que configuramos arriba
            .intr_type  = LEDC_INTR_DISABLE,        // No necesitamos interrupciones
            .gpio_num   = led_map[i].gpio,          // Pin GPIO del LED
            .duty       = 0,                        // Iniciar con Duty Cycle = 0 (apagado)
            .hpoint     = 0,
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ch_cfg));
    }

    // Mensaje de confirmación de inicialización
    ESP_LOGI(TAG, "Subsistema PWM inicializado (%u canales, %u Hz, 8 bits)", 
             LED_CHANNEL_COUNT, LED_PWM_FREQ_HZ);
}

/**
 * @brief Establece el brillo de un LED específico.
 * 
 * @param channel El índice del LED a controlar (LED_CHANNEL_0, LED_CHANNEL_1, LED_CHANNEL_2).
 * @param duty El valor del ciclo de trabajo (0 a 255). 0 = apagado, 255 = máximo brillo.
 */
void leds_set_duty(led_channel_t channel, uint8_t duty)
{
    // Validación de seguridad: Verificar que el canal solicitado existe.
    if (channel >= LED_CHANNEL_COUNT) {
        /* Canal fuera de rango: ignorar silenciosamente para evitar fallos en tiempo real */
        return;
    }

    /* 
       Escribir el nuevo ciclo de trabajo y actualizar el periferico.
       ledc_set_duty() actualiza un registro interno, pero el cambio no se aplica
       al hardware hasta que se llama a ledc_update_duty().
    */
    ledc_set_duty(LED_MODE, led_map[channel].channel, duty);
    ledc_update_duty(LED_MODE, led_map[channel].channel);
}