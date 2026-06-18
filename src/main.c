/**
 * @file    main.c
 * @brief   Punto de entrada del sistema de control de LEDs por potenciometros.
 *
 * =============================================================================
 * CONCLUSION DEL EQUIPO
 * Integrantes:  * Integrantes: Victor Hugo Barrera Garcia, Sergio Garcia Hernandez 
 * Fecha: 18/06/2026
* En esta practica se implemento un sistema multitarea en FreeRTOS para leer tres potenciometros
* mediante el ADC de la ESP32 y controlar el brillo de tres LEDs usando PWM. Se comprobo que cada
* tarea puede trabajar de forma independiente, manteniendo un periodo de ejecucion constante gracias
* al uso de vTaskDelay(), lo cual permite que el planificador administre correctamente el tiempo del CPU.
*
* El uso de pvParameters permitio reutilizar una sola funcion de tarea para los tres canales, evitando
* duplicar codigo y haciendo que el programa sea mas ordenado y facil de ampliar. Al pasar una estructura
* con el canal ADC, el canal LED y el nombre de la tarea, cada instancia pudo comportarse de manera diferente
* sin necesidad de crear funciones separadas.
*
* Tambien se observo que las prioridades pueden influir en el orden en que aparecen los mensajes en la UART,
* especialmente cuando varias tareas estan listas al mismo tiempo. En general, la practica permitio comprender
* mejor la modularizacion del codigo, el manejo de tareas en FreeRTOS y la relacion entre lectura analogica,
* escalamiento de datos y control PWM.
*/
/*
 * =============================================================================
 *
 * Descripcion:
 *   Inicializa los perifericos (ADC y LEDC) y arranca tres tareas FreeRTOS.
 *   Cada tarea lee un potenciometro y ajusta el brillo del LED correspondiente.
 *
 *   La logica de negocio esta completamente encapsulada en los modulos:
 *     - adc_reader : lectura de potenciometros via ADC oneshot
 *     - leds       : control de brillo via LEDC (PWM)
 *     - tasks      : tareas FreeRTOS y sus parametros
 *
 * Plataforma : ESP32 / ESP-IDF 5.x - 6.x  / FreeRTOS
 * IDE        : VS Code + PlatformIO (framework: espidf)
 * Estandar   : C99
 */

#include "adc_reader.h"
#include "leds.h"
#include "tasks.h"

/* ------------------------------------------------------------------ */
/*  Punto de entrada                                                   */
/* ------------------------------------------------------------------ */

void app_main(void)
{
    /*Inicializar subsistema ADC (tres potenciometros) */
    adc_reader_init();
    /*Inicializar subsistema LED PWM (tres canales LEDC) */
    leds_init();
    /*Crear las tres tareas FreeRTOS e iniciar el scheduler */
    tasks_create_all();
    /*app_main retorna; FreeRTOS continua ejecutando las tareas */
}