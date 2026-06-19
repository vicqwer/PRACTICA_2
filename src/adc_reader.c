/**
 * @file    adc_reader.c
 * @brief   Implementacion de lectura ADC oneshot para tres potenciometros.
 *
 * Utiliza la API adc_oneshot de ESP-IDF 5.x/6.x (nueva API; no usa
 * adc1_get_raw() de la API legacy). El handle del ADC se guarda como
 * variable estatica para ser reutilizado por las tareas.
 *
 * Plataforma : ESP32 / ESP-IDF 5.x - 6.x
 * Estandar   : C99
 */

#include "adc_reader.h"
#include "esp_log.h"

/* ------------------------------------------------------------------ */
/*  Constantes internas                                                */
/* ------------------------------------------------------------------ */

// Etiqueta para los mensajes de depuración (logs) de este módulo
static const char *TAG = "ADC_READER";

/* ------------------------------------------------------------------ */
/*  Estado interno                                                     */
/* ------------------------------------------------------------------ */

/** 
 * Handle del modulo ADC1 en modo oneshot.
 * Se declara como 'static' para que solo sea visible dentro de este archivo.
 * Es compartido entre los tres canales (potenciómetros), ya que todos usan
 * la misma unidad ADC1.
 */
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

/** 
 * Tabla de mapeo: Índice lógico (0, 1, 2) -> Canal físico del ADC1.
 * 
 * Esta tabla centraliza la asignación de pines, facilitando el mantenimiento
 * y evitando números mágicos esparcidos por todo el código.
 * 
 * En el ESP32 DevKit V1:
 * - ADC1_CH0 = GPIO 36 (Pot 0)
 * - ADC1_CH3 = GPIO 39 (Pot 1)
 * - ADC1_CH6 = GPIO 34 (Pot 2)
 */
static const adc_channel_t adc_channel_map[ADC_CHANNEL_COUNT] = {
    ADC_CH_POT0,   /* Potenciometro 0 -> GPIO 36 */
    ADC_CH_POT1,   /* Potenciometro 1 -> GPIO 39 */
    ADC_CH_POT2,   /* Potenciometro 2 -> GPIO 34 */
};

/* ------------------------------------------------------------------ */
/*  Implementacion                                                     */
/* ------------------------------------------------------------------ */

/**
 * @brief Inicializa el hardware del ADC1 en modo "One-Shot".
 * 
 * Configura la unidad ADC1, la atenuación (para permitir lecturas de hasta 3.3V)
 * y la resolución de 12 bits. Esta función debe llamarse UNA SOLA VEZ antes
 * de cualquier lectura.
 */
void adc_reader_init(void)
{
    /* 1. Crear la instancia de la unidad ADC1 en modo oneshot */
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id  = ADC_UNIT_1,           // Usamos el ADC1 (no el ADC2 para evitar conflictos con WiFi/BT)
        .ulp_mode = ADC_ULP_MODE_DISABLE, // No usamos ULP (Ultra Low Power) en esta práctica
    };
    
    // Se pasa la dirección de s_adc_handle para que el driver lo inicialice
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &s_adc_handle));

    /* 2. Configurar la atenuación y la resolución comunes para todos los canales */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,       /* Atenuación de 12 dB: Permite medir voltajes de 0V a ~3.3V.
                                               Sin esta atenuación, el ADC solo mediría hasta ~1.1V. */
        .bitwidth = ADC_BITWIDTH_DEFAULT,  /* En ESP32, BITWIDTH_DEFAULT equivale a 12 bits.
                                               Esto da un rango de valores de 0 a 4095. */
    };

    /* 3. Aplicar la configuración a cada uno de los canales (potenciómetros) */
    for (uint8_t i = 0; i < ADC_CHANNEL_COUNT; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(
            s_adc_handle,           // Handle de la unidad
            adc_channel_map[i],     // El canal físico a configurar (obtenido de la tabla)
            &chan_cfg));            // La configuración común de atenuación y resolución
    }

    ESP_LOGI(TAG, "ADC inicializado: %u canales, 12 bits, atten 12 dB",
             ADC_CHANNEL_COUNT);
}

/**
 * @brief Realiza una lectura del ADC en el canal especificado.
 * 
 * @param channel Índice lógico del potenciómetro (0, 1 o 2).
 * @return uint16_t Valor de la lectura cruda de 12 bits (0 a 4095).
 *                 Retorna 0 en caso de error (canal inválido o ADC no inicializado).
 */
uint16_t adc_reader_get_raw(uint8_t channel)
{
    // Validación de seguridad: Verificar que el canal es válido y que el ADC fue inicializado
    if (channel >= ADC_CHANNEL_COUNT || s_adc_handle == NULL) {
        return 0U;
    }

    int raw = 0;
    
    /* Realizar la lectura del ADC en modo oneshot.
     * adc_oneshot_read toma una muestra en el momento exacto de la llamada.
     * El resultado se almacena en la variable 'raw' pasada por referencia.
     */
    if (adc_oneshot_read(s_adc_handle, adc_channel_map[channel], &raw) == ESP_OK) {
        return (uint16_t)raw; // Cast seguro a uint16_t (el valor máximo es 4095)
    }

    // Si la lectura falló, retornar 0 como valor seguro
    return 0U;
}