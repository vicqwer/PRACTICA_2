/**
 * @file    main.c
 * @brief   Punto de entrada del sistema de control de LEDs por potenciometros.
 *
 * =============================================================================
 * CONCLUSION DEL EQUIPO
 *
 * Integrantes: Victor Hugo Barrera Garcia, Sergio Garcia Hernandez
 *
 * En esta practica se implemento un sistema multitarea en FreeRTOS para leer tres
 * potenciometros mediante el ADC de la ESP32 y controlar el brillo de tres LEDs
 * usando PWM. Se comprobo que cada tarea puede trabajar de forma independiente,
 * manteniendo un periodo de ejecucion constante gracias al uso de vTaskDelay(),
 * lo cual permite que el planificador libere el CPU y ejecute otras tareas, a
 * diferencia de un retardo por ciclos for que mantendria ocupado al procesador.
 *
 * El uso de pvParameters permitio reutilizar una sola funcion de tarea para los
 * tres canales, evitando variables globales innecesarias y la duplicacion de
 * funciones casi iguales. Al pasar una estructura con el canal ADC, el canal LED
 * y el nombre de la tarea, cada instancia pudo comportarse de manera diferente
 * sin modificar la firma fija de vTaskPotLED().
 *
 * Los bloques task_params_t se declararon como static const para asegurar que
 * permanezcan validos durante toda la ejecucion del programa. Si se declararan
 * como variables locales dentro de tasks_create_all(), al terminar esa funcion
 * la memoria podria reutilizarse y las tareas recibirian datos incorrectos; esto
 * se detectaria experimentalmente al observar lecturas, nombres o canales
 * erroneos en la salida UART.
 *
 * Aunque las tres tareas tienen el mismo periodo de 50 ms, sus prioridades pueden
 * notarse cuando varias quedan listas al mismo tiempo o cuando existe carga en el
 * sistema. En esos casos, FreeRTOS atiende primero a la tarea con mayor prioridad,
 * por lo que el orden de los mensajes en la terminal puede cambiar.
 *
 * El stack watermark permite observar cuanta pila libre conserva cada tarea. Se
 * esperarian valores parecidos porque ejecutan la misma funcion, aunque pueden
 * diferir ligeramente por el orden de ejecucion, llamadas a impresion por UART o
 * consumo temporal de pila. Si se agregara un cuarto potenciometro y LED, el
 * diseno modular solo requeriria agregar el nuevo canal en los arreglos de ADC y
 * LEDC, crear otro bloque task_params_t y una cuarta llamada a xTaskCreate(),
 * sin cambiar las firmas publicas de las funciones.
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