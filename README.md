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

  Compilación y carga

Para compilar el proyecto:

```bash
pio run
```

Para cargar el programa en la ESP32:

```bash
pio run --target upload
```

Para abrir el monitor serial:

```bash
pio device monitor -b 115200
```

---
Preguntas guía

1. ¿Por qué es preferible pasar la configuración de canal mediante `pvParameters` en lugar de usar variables globales o funciones de tarea distintas para cada canal?

Es preferible usar `pvParameters` porque permite reutilizar una sola función de tarea para varios canales. En esta práctica, la función `vTaskPotLED()`
puede controlar diferentes pares de potenciómetro y LED dependiendo de la estructura `task_params_t` que recibe al crearse la tarea.

Esto evita duplicar código creando una función diferente para cada canal. También reduce el uso innecesario de variables globales y hace que el programa sea
más ordenado, modular y fácil de modificar.

---

2. ¿Qué sucedería si los bloques `task_params_t` se declararan como variables locales en `tasks_create_all()` en lugar de `static`? ¿Cómo lo detectarías experimentalmente?

Si los bloques `task_params_t` se declararan como variables locales dentro de `tasks_create_all()`, podrían dejar de ser válidos 
cuando esa función termine. Esto ocurre porque las variables locales viven en el stack de la función, y al salir de ella esa memoria puede reutilizarse para otra cosa.

Como las tareas siguen ejecutándose después de que `tasks_create_all()` termina, podrían leer datos corruptos o incorrectos. 
Experimentalmente se detectaría viendo comportamientos extraños, por ejemplo nombres incorrectos en el monitor serial, canales ADC 
equivocados, LEDs que no corresponden al potenciómetro o incluso reinicios inesperados.

Por eso los parámetros se declaran como `static const`, para asegurar que existan durante toda la ejecución del programa.

---

3. Las tres tareas tienen el mismo periodo de 50 ms pero prioridades distintas. ¿En qué situación se hace visible la diferencia de prioridad en la salida del terminal?

La diferencia de prioridad se hace visible cuando varias tareas quedan listas para ejecutarse al mismo tiempo, por ejemplo cuando las
tres terminan su `vTaskDelay()` en el mismo tick o casi al mismo tiempo.

En ese caso, FreeRTOS ejecuta primero la tarea con mayor prioridad. Por eso, aunque las tres tareas tengan el mismo periodo de 50 ms, 
el orden de los mensajes en la terminal puede mostrar primero la tarea de prioridad más alta y después las de menor prioridad.

---

4. ¿Por qué se usa `vTaskDelay(pdMS_TO_TICKS(50))` en lugar de un `for` con retardo de software? ¿Qué diferencia implica para el planificador?

Se usa `vTaskDelay(pdMS_TO_TICKS(50))` porque esta función bloquea temporalmente la tarea y libera el CPU para que el planificador 
pueda ejecutar otras tareas.

En cambio, un retardo hecho con un ciclo `for` mantiene ocupado al procesador sin hacer trabajo útil. Eso impide que FreeRTOS administre 
correctamente el tiempo de CPU y puede afectar la respuesta de otras tareas.

Con `vTaskDelay()`, la tarea pasa al estado `BLOCKED` durante el tiempo indicado y después vuelve a estar lista para ejecutarse.

---

5. ¿Qué valor esperarías en el `stack watermark` de la tarea con menor prioridad comparado con el de mayor prioridad? ¿Por qué podrían diferir?

Se esperaría que el `stack watermark` de las tres tareas sea parecido, porque todas ejecutan la misma función `vTaskPotLED()` y realizan 
operaciones similares: leer ADC, convertir a duty, actualizar PWM e imprimir por UART.

Sin embargo, podrían existir pequeñas diferencias debido al momento en que cada tarea imprime mensajes, al uso temporal de funciones internas, 
a la prioridad de ejecución o al orden en que el planificador atiende cada tarea. El `stack watermark` permite observar cuánta pila libre queda
disponible, por lo que valores muy bajos podrían indicar que el tamaño de stack asignado es insuficiente.

---

6. Si se añadiera un cuarto potenciómetro y LED, ¿qué cambios mínimos requeriría el código dado el diseño modular actual y la restricción de no modificar las firmas existentes?

Gracias al diseño modular, no sería necesario cambiar las firmas de las funciones públicas. Los cambios mínimos serían agregar el nuevo canal ADC,
el nuevo canal LED y crear una nueva estructura de parámetros `task_params_t` para el cuarto canal.

También se tendría que agregar una cuarta llamada a `xTaskCreate()` para crear otra instancia de `vTaskPotLED()`, además de actualizar los
arreglos o definiciones internas donde se configuran los canales ADC y LEDC.

En resumen, se agregaría la configuración del nuevo hardware y una nueva instancia de tarea, pero se seguiría usando la misma función `vTaskPotLED()` y las mismas interfaces públicas.


Estado

Práctica validada en ESP32.
El contador BCD muestra valores de 0 a 9, permite cambiar dirección, alternar velocidad y pausar o reanudar el sistema desde el último valor mostrado.
