/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: main.c
 *
 * Descripción: Archivo principal del proyecto
 *
 * Autor: Jon Varela Gonzalez
*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "cmd_system.h"
#include "bsp/esp-bsp.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"

#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "ADC.h"
#include "wifi.h"
#include "mqtt_client.h"
#include "comandos.h"
#include "dht11.h"
#include "lcd_16x2.h"
#include "general.h"
#include "inputs_outputs.h"
#include "json.h"
#include "mqtt.h"


#define CLAMP(val, min, max) ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

//****************************************************************************
//      Variables de este fichero
//****************************************************************************

static const char *TAG = "TFM";

static TaskHandle_t TaskLuzHabitacion 			= NULL;
static TaskHandle_t TaskTempHumedadHabitacion 	= NULL;
static TaskHandle_t TaskPersianas 				= NULL;
static TaskHandle_t TaskCortinas 				= NULL;
static TaskHandle_t TaskClock 					= NULL;

uint8_t Estado_sistema			= IDLE;

uint8_t Numero_Habitacion		= 1;

uint8_t Minute 					= 0;
uint8_t Hour 					= 0;
uint8_t Second					= 0;

uint8_t Actuacion_Termostato	= 0;

int estado_cortinas 			= 0;
int estado_persianas 			= 0;

//****************************************************************************
//      Variables externas inicializadas en otro fichero
//****************************************************************************
extern esp_mqtt_client_handle_t client;

extern EventGroupHandle_t FlagsEventos;

extern char topic_envio[64];

extern uint8_t Error_PC;
extern uint8_t Error_Habitacion;

//****************************************************************************
//		Funciones
//****************************************************************************
static void initialize_console(void);

static void ClockTask (void *pvparameters);

static int map_with_threshold(int value);

static void Luz_Habitacion(void *pvParameters);

static void Temp_Humedad_Habitacion(void *pvParameters);

static void Persianas_Habitacion(void *pvParameters);

static void Cortinas_Habitacion(void *pvParameters);

static void RTOS_Resources_Create(void);

// Inicializa el interprete de comandos/consola serie
static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM,ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM,ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .source_clk = UART_SCLK_DEFAULT,
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config = {
            .max_cmdline_args = 8,
            .max_cmdline_length = 256,
#if CONFIG_LOG_COLORS
            .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK( esp_console_init(&console_config) );

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

#if CONFIG_STORE_HISTORY
    /* Load command history from filesystem */
    linenoiseHistoryLoad(HISTORY_PATH);
#endif
}

// Tarea del clock para contar la hora actual
static void ClockTask (void *pvparameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1)
    {
    	vTaskDelayUntil(&xLastWakeTime,configTICK_RATE_HZ);

    	Second++;

    	if(Second >= 60)
		{
    		Second = 1;
			Minute++;

			if(Minute >= 60)
			{
				Minute = 0;
				Hour++;

				if(Hour >= 24)
				{
					Hour = 0;
				}

			}

		}

    }

}

// Función para mapear de 100, 4095 a 0, 255
static int map_with_threshold(int value){
    if (value < 100) return 0;

    return (value - 100) * 255 / (4095 - 100);
}

// Tarea de gestión de luz de la habitación
static void Luz_Habitacion(void *pvParameters)
{
	int muestra, intensidad_luz;

	while (1)
	{
		// Lee el dato del ADC
		muestra = adc_simple_read_raw();

		intensidad_luz = map_with_threshold(muestra);

		intensidad_luz = CLAMP(intensidad_luz,0,255);

		GL_setHabitacion(intensidad_luz);

		// Se espera durante 200ms
		vTaskDelay(0.2*configTICK_RATE_HZ);

	}

}

// Tarea de gestión de temperatura y humedad de la habitación
static void Temp_Humedad_Habitacion(void *pvParameters)
{
	struct dht11_reading sensor;

	char bufferlcd[17];
	char buffer[100];

	while (1)
	{
		// Leer datos del sensor de temperatura y humedad
		sensor = DHT11_read();

		if (sensor.status == DHT11_OK)
		{

			LCD_clearScreen();
			LCD_home();

			LCD_writeStr("Temp: ");
			sprintf(bufferlcd, "%dC", sensor.temperature);
			LCD_writeStr(bufferlcd);

			LCD_setCursor(0,1);
			LCD_writeStr("Humedad: ");
			sprintf(bufferlcd, "%d%%", sensor.humidity);
			LCD_writeStr(bufferlcd);

			if (Error_PC == 0 && Error_Habitacion == 0)
			{
				struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

				json_printf(&out1," {Temperatura: %d ,",sensor.temperature);
				json_printf(&out1," Humedad: %d }",sensor.humidity);

				int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
				ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/

			}

			if (Actuacion_Termostato == 1)
			{
				if (sensor.temperature > 20 || sensor.temperature < 15)
				{
					gpio_set_level(LED_Termostato, 1);
				}
				else
				{
					gpio_set_level(LED_Termostato, 0);
				}
			}
			else
			{
				gpio_set_level(LED_Termostato, 0);
			}

		}

		vTaskDelay(2.5*configTICK_RATE_HZ); // Se espera durante 2500ms porque el sensor lo necesita

	}

}

// Tarea de gestión de las persianas de la habitación
static void Persianas_Habitacion(void *pvParameters)
{
	EventBits_t Evento;

	while (1)
	{
		Evento = xEventGroupWaitBits(FlagsEventos,SUBIR_PERSIANAS_FLAG | BAJAR_PERSIANAS_FLAG,pdFALSE,pdFALSE,portMAX_DELAY);

		if ((Evento & SUBIR_PERSIANAS_FLAG) && !(Evento & BAJAR_PERSIANAS_FLAG))
		{
			estado_persianas++;
		}
		else if ((Evento & BAJAR_PERSIANAS_FLAG) && !(Evento & SUBIR_PERSIANAS_FLAG))
		{
			estado_persianas--;
		}

		estado_persianas = CLAMP(estado_persianas,0,255);

		if (estado_persianas == 0 || estado_persianas == 255)
		{
			xEventGroupClearBits(FlagsEventos,SUBIR_PERSIANAS_FLAG | BAJAR_PERSIANAS_FLAG);
		}

		GL_setPersianas(estado_persianas);

		vTaskDelay(0.2*configTICK_RATE_HZ); // Se espera durante 200ms

	}

}

// Tarea de gestión de las cortinas de la habitación
static void Cortinas_Habitacion(void *pvParameters)
{
	EventBits_t Evento;

	while (1)
	{
		Evento = xEventGroupWaitBits(FlagsEventos,SUBIR_CORTINAS_FLAG | BAJAR_CORTINAS_FLAG,pdFALSE,pdFALSE,portMAX_DELAY);

		if ((Evento & SUBIR_CORTINAS_FLAG) && !(Evento & BAJAR_CORTINAS_FLAG))
		{
			estado_cortinas++;
		}
		else if ((Evento & BAJAR_CORTINAS_FLAG) && !(Evento & SUBIR_CORTINAS_FLAG))
		{
			estado_cortinas--;
		}

		estado_cortinas = CLAMP(estado_cortinas,0,255);

		if (estado_cortinas == 0 || estado_cortinas == 255)
		{
			xEventGroupClearBits(FlagsEventos,SUBIR_CORTINAS_FLAG | BAJAR_CORTINAS_FLAG);
		}

		GL_setCortinas(estado_cortinas);

		vTaskDelay(0.2*configTICK_RATE_HZ); // Se espera durante 200ms

	}

}

// Función para crear los recursos RTOS
static void RTOS_Resources_Create(void)
{
	// Se crea la tarea de gestión del reloj
	xTaskCreate(ClockTask, "clock", 1024, NULL, 1, &TaskClock);

	// Se crea la tarea de gestión de luz de la habitación
	xTaskCreate(Luz_Habitacion, "Luz_Habitacion", 1024, NULL, 1, &TaskLuzHabitacion);

	// Se crea la tarea de gestión de temperatura y humedad de la habitación
	xTaskCreate(Temp_Humedad_Habitacion, "Temp_Humedad_Habitacion", 4096, NULL, 1, &TaskTempHumedadHabitacion);

	// Se crea la tarea de gestión de las persianas de la habitación
	xTaskCreate(Persianas_Habitacion, "Persianas", 1024, NULL, 1, &TaskPersianas);

	// Se crea la tarea de gestión de las cortinas de la habitación
	xTaskCreate(Cortinas_Habitacion, "Cortinas", 1024, NULL, 1, &TaskCortinas);
}

// Tarea principal
void app_main(void)
{

	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// Inicializa los pines de salida y de entrada
	GL_initGPIO();

	// Inicializa el LCD
	LCD_init(LCD_ADDR, SDA_PIN, SCL_PIN, LCD_COLS, LCD_ROWS);
	LCD_clearScreen();
	LCD_home();

	// Inicializa la parte del i2c para el sensor de temperatura y humedad
	DHT11_init(DHT11_GPIO);

	// Inicializa la parte del adc
	adc_simple_init();

    // Inicializa la biblioteca WiFi
    wifi_initlib();

    // Conecta a la Wifi
    wifi_init_sta();

    // Se conecta al cliente MQTT
	mqtt_app_start(CONFIG_EXAMPLE_MQTT_BROKER_URI);

    // Inicializa la consola
    initialize_console();

    // Register commands
    esp_console_register_help_command();
    register_system();
    init_MisComandos();

    // Se crean los recursos RTOS para el funcionamiento del sistema
	// De esta manera, hasta que no se haya inicializado todo, no se muestra nada en el LCD y queda más claro cuando se ha iniciado todo
    RTOS_Resources_Create();

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char* prompt = LOG_COLOR_I "TFM> " LOG_RESET_COLOR;

    // Figure out if the terminal supports escape sequences
    int probe_status = linenoiseProbe();
    if (probe_status) { // zero indicates success
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "TFM> ";
#endif // CONFIG_LOG_COLORS
    }

    // Main loop (ejecuta el interprete de comandos)
    while(true) {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL) { // Ignore empty lines
            continue;
        }
        // Add the command to the history
        linenoiseHistoryAdd(line);

        // Try to run the command
        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // Command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }
        // linenoise allocates line buffer on the heap, so need to free it
        linenoiseFree(line);
    }
}


