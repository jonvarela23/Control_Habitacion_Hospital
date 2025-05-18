/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: comandos.c
 *
 * Descripción: Archivo para hacer los test desde el terminal de puerto serie
 *
 * Autor: Jon Varela Gonzalez
*/

#include "comandos.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_sleep.h"
#include "esp_spi_flash.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "argtable3/argtable3.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "wifi.h"
#include "mqtt.h"
#include "ADC.h"
#include "dht11.h"
#include "lcd_16x2.h"
#include "mqtt_client.h"
#include "general.h"
#include "inputs_outputs.h"
#include "json.h"

//****************************************************************************
//      Variables de este fichero
//****************************************************************************
//TAG para los mensajes de consola
static const char *TAG = "COMANDOS";

//****************************************************************************
//      Variables externas inicializadas en otro fichero
//****************************************************************************
extern uint8_t Numero_Habitacion;

extern esp_mqtt_client_handle_t client;

extern char topic_recepcion[64];
extern char topic_envio[64];

extern uint8_t Error_PC;
extern uint8_t Error_Habitacion;

//****************************************************************************
//		Funciones
//****************************************************************************

static int Cmd_led(int argc, char **argv);
static void register_Cmd_led(void);

static int Cmd_adcRead(int argc, char **argv);
static void register_Cmd_adcRead(void);

static int Cmd_pwm(int argc, char **argv);
static void register_Cmd_pwm(void);

static int Cmd_temp_humedad(int argc, char **argv);
static void register_Cmd_temp_humedad(void);

static int Cmd_print(int argc, char **argv);
static void register_Cmd_print_lcd(void);

static int Cmd_get_switch(int argc, char **argv);
static void register_Cmd_get_switch(void);

static int Cmd_habitacion(int argc, char **argv);
static void register_Cmd_habitacion(void);

static int Cmd_restart(int argc, char **argv);
static void register_Cmd_restart(void);


// Comando led para cambiar el estado del Led
static int Cmd_led(int argc, char **argv)
{
	if (argc != 3)
	{
		//Si los parámetros no son suficientes, muestro la ayuda
		ESP_LOGI(TAG, "led x on|off\r\n");
	}
	else
	{

		if (0==strncmp(argv[1], "llamada",7))
		{
			if (0==strncmp(argv[2], "on",2))
			{
				ESP_LOGI(TAG, "Enciendo el led de llamada\r\n");
				gpio_set_level(LED_Llamada_Paciente, 1);
			}
			else if (0==strncmp(argv[2], "off",3))
			{
				ESP_LOGI(TAG, "Apago el led de llamada\r\n");
				gpio_set_level(LED_Llamada_Paciente, 0);
			}
			else
			{
				//Si el parámetro no es correcto, muestro la ayuda
				ESP_LOGI(TAG, "led llamada on|off\r\n");
			}
		}
		else if (0==strncmp(argv[1], "presencia",9))
		{
			if (0==strncmp(argv[2], "on",2))
			{
				ESP_LOGI(TAG, "Enciendo el led de presencia\r\n");
				gpio_set_level(LED_Presencia_Asistencia, 1);
			}
			else if (0==strncmp(argv[2], "off",3))
			{
				ESP_LOGI(TAG, "Apago el led de presencia\r\n");
				gpio_set_level(LED_Presencia_Asistencia, 0);
			}
			else
			{
				//Si el parámetro no es correcto, muestro la ayuda
				ESP_LOGI(TAG, "led presencia on|off\r\n");
			}
		}
		else if (0==strncmp(argv[1], "termostato",10))
		{
			if (0==strncmp(argv[2], "on",2))
			{
				ESP_LOGI(TAG, "Enciendo el led de termostato\r\n");
				gpio_set_level(LED_Termostato, 1);
			}
			else if (0==strncmp(argv[2], "off",3))
			{
				ESP_LOGI(TAG, "Apago el led de termostato\r\n");
				gpio_set_level(LED_Termostato, 0);
			}
			else
			{
				//Si el parámetro no es correcto, muestro la ayuda
				ESP_LOGI(TAG, "led termostato on|off\r\n");
			}
		}
		else
		{
			//Si el parámetro no es correcto, muestro la ayuda
			ESP_LOGI(TAG, "led x (llamada, presencia o termostato) on|off\r\n");
		}
	}
    return 0;

}

static void register_Cmd_led(void)
{
    const esp_console_cmd_t cmd = {
        .command = "led",
        .help = "Enciende y apaga el led x (llamada, presencia o termostato)",
        .hint = NULL,
        .func = &Cmd_led,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

// Comando adc para leer el valor del adc del potenciometro
static int Cmd_adcRead(int argc, char **argv)
{
	int muestra;
	float voltaje;
	muestra=adc_simple_read_raw();
	voltaje=(float)adc_simple_read_mv()/1000.0;
	ESP_LOGI(TAG, "ADC RAW: %d Volt: %f \r\n",muestra,voltaje);

	return 0;
}

static void register_Cmd_adcRead(void)
{
    const esp_console_cmd_t cmd = {
        .command = "adc",
        .help = "Lee valor del ADC del potenciometro",
        .hint = NULL,
        .func = &Cmd_adcRead,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

// Comando pwm para cambiar la intensidad de los leds
static int Cmd_pwm(int argc, char **argv)
{
	if (argc !=3)
	{
		//Si los parámetros no son suficientes, muestro la ayuda
		ESP_LOGI(TAG, "pwm x 'valor'\r\n");
	}
	else
	{

		if (0==strncmp(argv[1], "persianas",9))
		{
			ESP_LOGI(TAG, "Establezco valor a Led persianas\r\n");
			GL_setPersianas((uint8_t)strtoul(argv[2],NULL,10));
		}
		else if (0==strncmp(argv[1], "cortinas",8))
		{
			ESP_LOGI(TAG, "Establezco valor a Led cortinas\r\n");
			GL_setCortinas((uint8_t)strtoul(argv[2],NULL,10));
		}
		else if (0==strncmp(argv[1], "habitacion",10))
		{
			ESP_LOGI(TAG, "Establezco valor a Led habitacion\r\n");
			GL_setHabitacion((uint8_t)strtoul(argv[2],NULL,10));
		}
		else
		{
			//Si el parámetro no es correcto, muestro la ayuda
			ESP_LOGI(TAG, "pwm x (persianas, cortinas o habitacion) 'valor'\r\n");
		}
	}
    return 0;

}

static void register_Cmd_pwm(void)
{
    const esp_console_cmd_t cmd = {
        .command = "pwm",
        .help = "Establece el nivel pwm del led x (persianas, cortinas o habitacion)",
        .hint = NULL,
        .func = &Cmd_pwm,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

// Comando lectura Temperatura y Humedad
static int Cmd_temp_humedad(int argc, char **argv)
{
	ESP_LOGI(TAG, "Temperatura: %d\r\n", DHT11_read().temperature);
	ESP_LOGI(TAG, "Humedad: %d\r\n", DHT11_read().humidity);
	ESP_LOGI(TAG, "Codigo de estado: %d\r\n", DHT11_read().status);

    return 0;

}

static void register_Cmd_temp_humedad(void)
{
    const esp_console_cmd_t cmd = {
        .command = "temp",
        .help = "Leer el valor de temperatura y humedad",
        .hint = NULL,
        .func = &Cmd_temp_humedad,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

// Comando para printear en LCD
static int Cmd_print(int argc, char **argv)
{
	if (argc !=2)
	{
		//Si los parámetros no son suficientes, muestro la ayuda
		ESP_LOGI(TAG, "print x (una palabra)\r\n");
	}
	else
	{
		ESP_LOGI(TAG, "Escribe %s en LCD\r\n", argv[1]);

		LCD_clearScreen();
		LCD_home();
		LCD_writeStr(argv[1]);

	}

    return 0;

}

static void register_Cmd_print_lcd(void)
{
    const esp_console_cmd_t cmd = {
        .command = "print",
        .help = "Escribir mensaje en LCD",
        .hint = NULL,
        .func = &Cmd_print,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

// Comando para obtener el valor del switch
static int Cmd_get_switch(int argc, char **argv)
{

	ESP_LOGI(TAG, "El switch tiene valor %d\r\n", gpio_get_level(Input_switch));

    return 0;

}

static void register_Cmd_get_switch(void)
{
    const esp_console_cmd_t cmd = {
        .command = "get",
        .help = "Leer valor del switch",
        .hint = NULL,
        .func = &Cmd_get_switch,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

// Comando para cambiar el número de la habitación
static int Cmd_habitacion(int argc, char **argv)
{
	uint8_t numero;

	if (argc !=2)
	{
		//Si los parámetros no son suficientes, muestro la ayuda
		ESP_LOGI(TAG, "habitacion x 'numero'\r\n");
	}
	else
	{
		numero = (uint8_t)strtoul(argv[1],NULL,10);

		if (numero > 1 && numero < 17)
		{

			int Numero_Habitacion_Anterior = Numero_Habitacion;

			Numero_Habitacion = numero;

			if (Numero_Habitacion != Numero_Habitacion_Anterior)
			{

				ESP_LOGI(TAG, "Establezco el numero de la habitacion: %d\r\n", numero);

				if (Error_PC == 0 && Error_Habitacion == 0)
				{
					char buffer1[50], buffer2[50];

					struct json_out out1 = JSON_OUT_BUF(buffer1, sizeof(buffer1));
					struct json_out out2 = JSON_OUT_BUF(buffer2, sizeof(buffer2));

					json_printf(&out1,"{Habitacion:desconectado}\0");
					esp_mqtt_client_publish(client,topic_envio, buffer1, 0, 0, 1);

					Numero_Habitacion = numero;

					sprintf(topic_envio, "/TFM/jvg/ESP/Habitacion%d", Numero_Habitacion);

					json_printf(&out2,"{Habitacion:conectado}\0");
					esp_mqtt_client_publish(client,topic_envio, buffer2, 0, 0, 1);
				}
				else
				{
					sprintf(topic_envio, "/TFM/jvg/ESP/Habitacion%d", Numero_Habitacion);
				}

				sprintf(topic_recepcion, "/TFM/jvg/PC/Habitacion%d", Numero_Habitacion);

			}

		}
		else
		{
			//Si el parámetro no es correcto, muestro la ayuda
			ESP_LOGI(TAG, "numero entre 1 y 16\r\n");
		}
	}

    return 0;

}

static void register_Cmd_habitacion(void)
{
    const esp_console_cmd_t cmd = {
        .command = "habitacion",
        .help = "Cambiar número habitación",
        .hint = NULL,
        .func = &Cmd_habitacion,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

// Comando para reiniciar el micro
static int Cmd_restart(int argc, char **argv)
{

	esp_restart();

    return 0;

}

static void register_Cmd_restart(void)
{
    const esp_console_cmd_t cmd = {
        .command = "restart",
        .help = "Reiniciar el micro",
        .hint = NULL,
        .func = &Cmd_restart,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void init_MisComandos(void)
{
	register_Cmd_led();
	register_Cmd_adcRead();
	register_Cmd_pwm();
	register_Cmd_temp_humedad();
	register_Cmd_print_lcd();
	register_Cmd_get_switch();
	register_Cmd_habitacion();
	register_Cmd_restart();
}
