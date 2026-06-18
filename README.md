 Práctica 2 - Control de brillo de LEDs mediante potenciómetros y tareas FreeRTOS

Integrantes

1. Víctor Hugo Barrera García
2. Sergio García Hernández

Descripción de la práctica

La práctica consistió en implementar un sistema multitarea utilizando FreeRTOS sobre una tarjeta ESP32.  
El sistema permite leer tres potenciómetros mediante entradas ADC y controlar el brillo de tres LEDs mediante 
salidas PWM utilizando el periférico LEDC.

Cada potenciómetro se encuentra asociado a un LED. Cuando el valor del potenciómetro cambia, la lectura analógica 
del ADC también cambia y se convierte a un valor de duty cycle PWM. De esta manera, el brillo del LED correspondiente 
aumenta o disminuye de forma proporcional.

Para evitar duplicar código, se implementó una única función de tarea llamada `vTaskPotLED()`. 
Esta función es reutilizada por las tres tareas mediante el uso de `pvParameters`, permitiendo que cada tarea 
reciba una configuración distinta: canal ADC, canal LED y nombre de identificación para el monitoreo por UART.

Funcionamiento

Cada tarea realiza las siguientes acciones:

1. Lee el valor del ADC del potenciómetro asignado.
2. Convierte la lectura de 12 bits, en el rango de `0` a `4095`, a un duty cycle PWM de 8 bits, en el rango de `0` a `255`.
3. Actualiza el brillo del LED correspondiente mediante LEDC.
4. Imprime información de monitoreo por UART, incluyendo `raw`, `duty`, `heap` y `wm`.
5. Utiliza `vTaskDelay()` para liberar el CPU y permitir la ejecución de las demás tareas.

Tabla de pines

| Canal | Potenciómetro | GPIO ADC | LED | GPIO PWM |
|------|---------------|----------|-----|----------|
| 0 | POT0 | GPIO36 | LED0 rojo | GPIO25 |
| 1 | POT1 | GPIO39 | LED1 verde | GPIO26 |
| 2 | POT2 | GPIO34 | LED2 amarillo | GPIO27 |

---

Estructura del proyecto

| Archivo | Descripción |
|--------|-------------|
| `main.c` | Punto de entrada del programa. Inicializa ADC, LEDC y crea las tareas FreeRTOS. |
| `adc_reader.h` / `adc_reader.c` | Configuración y lectura de los canales ADC. También contiene la conversión de lectura ADC a duty PWM. |
| `leds.h` / `leds.c` | Configuración del periférico LEDC y actualización del duty cycle de cada LED. |
| `tasks.h` / `tasks.c` | Creación de tareas FreeRTOS y uso de `pvParameters` para reutilizar una sola función de tarea. |
| `platformio.ini` | Configuración del proyecto para PlatformIO con framework ESP-IDF. |

---

Salida esperada en monitor serial

Durante la ejecución, el monitor serial muestra información periódica de cada tarea:
```text
I TASKS: [POT0-LED0] raw=2048 duty=127 heap=189432 wm=1512
I TASKS: [POT1-LED1] raw=1024 duty= 63 heap=189432 wm=1488
I TASKS: [POT2-LED2] raw=3210 duty=199 heap=189432 wm=1456
```

Donde:

* `raw` representa la lectura directa del ADC.
* `duty` representa el valor PWM aplicado al LED.
* `heap` muestra la memoria libre disponible.
* `wm` muestra el stack watermark de la tarea.
