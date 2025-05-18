/*
 * TFM: Control Habitación Hospital
 *
 * Máster Universitario en Sistemas Electrónicos para Entornos Inteligentes
 *
 * Archivo: inputs_outputs.c
 *
 * Descripción: Driver para las entradas y salidas (GPIO) y para los LEDs tanto lógicos como de PWM
 *
 * Autor: Jon Varela Gonzalez
*/

#include "inputs_outputs.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"

#include "driver/uart.h"

#include "driver/adc.h"
#include "driver/ledc.h"
#include "driver/dac.h"
#include "esp_event.h"
#include "mqtt_client.h"

#include "bsp/esp-bsp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include <stdio.h>

#include "general.h"
#include "json.h"

//****************************************************************************
//      Variables de este fichero
//****************************************************************************

static const char *TAG = "TFM";

static QueueHandle_t gpio_evt_queue = NULL;
static TickType_t last_press_times[40];

EventGroupHandle_t FlagsEventos = NULL;

uint8_t Actuacion_Persianas		= 0;
uint8_t Actuacion_Ruido			= 0;

//****************************************************************************
//      Variables externas inicializadas en otro fichero
//****************************************************************************

extern esp_mqtt_client_handle_t client;

extern char topic_recepcion[64];
extern char topic_envio[64];

extern uint8_t Estado_sistema;

extern uint8_t Numero_Habitacion;

extern uint8_t Hour;

extern uint8_t Error_PC;
extern uint8_t Error_Habitacion;


//****************************************************************************
//		Funciones
//****************************************************************************
void IRAM_ATTR gpio_isr_handler(void* arg);

static void gpio_task_handler(void* arg);

void GL_initGPIO(void);

void GL_initLEDC();

void GL_setPersianas(uint8_t Level);

void GL_setCortinas(uint8_t Level);

void GL_setHabitacion(uint8_t Level);


// Interrupción para los botones externos como GPIOs
void IRAM_ATTR gpio_isr_handler(void* arg)
{
	uint32_t gpio_num = (uint32_t) arg;
	BaseType_t xHigherPriorityTaskWoken=pdFALSE;

	xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);

	if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();

}

// Tarea para hacer el antirrebote
static void gpio_task_handler(void* arg)
{
    uint32_t io_num;
    int estado;

    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            TickType_t now = xTaskGetTickCount();
            if (now - last_press_times[io_num] > pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
                last_press_times[io_num] = now;

                switch (io_num) {
                    case Boton_Llamada_Paciente:
                    	estado = gpio_get_level(Boton_Llamada_Paciente);

                    	ESP_LOGI(TAG, "Boton Llamada paciente: %d\n", estado);

                    	if (Error_PC == 0 && Error_Habitacion == 0)
                    	{
							if (estado == 0)
							{
								if (Estado_sistema == IDLE)
								{
									char buffer[50];

									struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

									gpio_set_level(LED_Llamada_Paciente, 1);
									Estado_sistema = LLAMADA;

									json_printf(&out1," {LlamadaPaciente: %B }",1);

									int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
									ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/

								}
								else if (Estado_sistema == LLAMADA)
								{
									char buffer[50];

									struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

									gpio_set_level(LED_Llamada_Paciente, 0);
									Estado_sistema = IDLE;

									json_printf(&out1," {LlamadaPaciente: %B }",0);

									int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
									ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/

								}
							}
                    	}

                        break;
                    case Boton_Presencia_Asistencia:
                    	estado = gpio_get_level(Boton_Presencia_Asistencia);

                    	ESP_LOGI(TAG, "Boton Presencia asistencia: %d\n", estado);

                    	if (Error_PC == 0 && Error_Habitacion == 0)
                    	{
							if (estado == 0)
							{
								if (Estado_sistema == IDLE || Estado_sistema == LLAMADA)
								{
									char buffer[100];

									struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

									gpio_set_level(LED_Presencia_Asistencia, 1);
									gpio_set_level(LED_Llamada_Paciente, 0);
									Estado_sistema = PRESENCIA;

									json_printf(&out1," {Presencia: %B ,",1);
									json_printf(&out1," LlamadaPaciente: %B ,",0);
									json_printf(&out1," LlamadaPacienteRuido: %B }",0);

									int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
									ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/

								}
								else if (Estado_sistema == PRESENCIA)
								{
									char buffer[50];

									struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

									gpio_set_level(LED_Presencia_Asistencia, 0);
									gpio_set_level(LED_Llamada_Paciente, 0);
									Estado_sistema = IDLE;

									json_printf(&out1," {Presencia: %B }",0);

									int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
									ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/
								}
							}
                    	}

                        break;
                    case Boton_Subir_Persianas:
                    	estado = gpio_get_level(Boton_Subir_Persianas);

                    	ESP_LOGI(TAG, "Boton Subir persianas: %d\n", estado);

                        if (estado == 0)
                        {
                        	xEventGroupSetBits(FlagsEventos,SUBIR_PERSIANAS_FLAG);
                        }
                        else
						{
                        	xEventGroupClearBits(FlagsEventos,SUBIR_PERSIANAS_FLAG);
						}

                        break;
                    case Boton_Bajar_Persianas:
                    	estado = gpio_get_level(Boton_Bajar_Persianas);

                    	ESP_LOGI(TAG, "Boton Bajar persianas: %d\n", estado);

                        if (estado == 0)
						{
                        	xEventGroupSetBits(FlagsEventos,BAJAR_PERSIANAS_FLAG);
						}
						else
						{
							xEventGroupClearBits(FlagsEventos,BAJAR_PERSIANAS_FLAG);
						}

                        break;
                    case Boton_Subir_Cortinas:
                    	estado = gpio_get_level(Boton_Subir_Cortinas);

                    	ESP_LOGI(TAG, "Boton Subir cortinas: %d\n", estado);

                        if (estado == 0)
						{
                        	xEventGroupSetBits(FlagsEventos,SUBIR_CORTINAS_FLAG);
						}
						else
						{
							xEventGroupClearBits(FlagsEventos,SUBIR_CORTINAS_FLAG);
						}

                        break;
                    case Boton_Bajar_Cortinas:
                    	estado = gpio_get_level(Boton_Bajar_Cortinas);

                    	ESP_LOGI(TAG, "Boton Bajar cortinas: %d\n", estado);

                        if (estado == 0)
						{
                        	xEventGroupSetBits(FlagsEventos,BAJAR_CORTINAS_FLAG);
						}
						else
						{
							xEventGroupClearBits(FlagsEventos,BAJAR_CORTINAS_FLAG);
						}

                        break;
                    case Input_Sensor_Ruido:
                    	estado = gpio_get_level(Input_Sensor_Ruido);

                    	ESP_LOGI(TAG, "Sensor de ruido: %d\n", estado);

                        if (Error_PC == 0 && Error_Habitacion == 0)
                        {
							if (Actuacion_Ruido == 1)
							{
								if (estado == 1)
								{
									if (Estado_sistema == IDLE)
									{
										char buffer[50];

										struct json_out out1 = JSON_OUT_BUF(buffer, sizeof(buffer));

										gpio_set_level(LED_Llamada_Paciente, 1);
										Estado_sistema = LLAMADA;

										json_printf(&out1," {LlamadaPacienteRuido: %B }",1);

										int msg_id = esp_mqtt_client_publish(client, topic_envio, buffer, 0, 0, 0);

#ifdef DEBUG
										ESP_LOGI(TAG, "sent successful, msg_id=%d: %s", msg_id, buffer);
#endif /*DEBUG*/

									}
								}
							}
                        }

                        break;
                    case Input_Sensor_Luz:
                    	estado = gpio_get_level(Input_Sensor_Luz);

                    	ESP_LOGI(TAG, "Sensor de luz: %d\n", estado);

                    	if (Actuacion_Persianas == 1 && estado == 1)
                    	{
                    		if (Hour < 8 || Hour > 22)
							{
								xEventGroupSetBits(FlagsEventos,BAJAR_PERSIANAS_FLAG);
							}

                    	}

                        break;

                    case Input_switch:
						estado = gpio_get_level(Input_switch);

						ESP_LOGI(TAG, "Input Switch: %d\n", estado);

						int Numero_Habitacion_Anterior;

						Numero_Habitacion_Anterior = Numero_Habitacion;

						Numero_Habitacion = estado + 1;

						if (Numero_Habitacion != Numero_Habitacion_Anterior)
						{

							if (Error_PC == 0 && Error_Habitacion == 0)
							{
								char buffer1[50], buffer2[50];

								struct json_out out1 = JSON_OUT_BUF(buffer1, sizeof(buffer1));

								struct json_out out2 = JSON_OUT_BUF(buffer2, sizeof(buffer2));

								json_printf(&out1,"{Habitacion:desconectado}\0");
								esp_mqtt_client_publish(client,topic_envio, buffer1, 0, 0, 1);

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
						else
						{
							Numero_Habitacion = Numero_Habitacion_Anterior;
						}

						break;
                    default:
                    	ESP_LOGI(TAG, "GPIO: %lu\n", io_num);
                        break;
                }
            }
        }
    }
}

void GL_initGPIO(void)
{
	int estado;

	// Se crean los recursos de RTOS para hacer el antirebote de las inputs
	gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
	xTaskCreate(gpio_task_handler, "gpio_task_handler", 8192, NULL, 1, NULL);

	// Crea el grupo de eventos
	FlagsEventos = xEventGroupCreate();
	if( FlagsEventos == NULL )
	{
			while(1);
	}

	// Se inicializan los outputs
	gpio_reset_pin(LED_Llamada_Paciente);
	gpio_reset_pin(LED_Presencia_Asistencia);
	gpio_reset_pin(LED_Termostato);

	gpio_set_direction(LED_Llamada_Paciente, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_Presencia_Asistencia, GPIO_MODE_OUTPUT);
	gpio_set_direction(LED_Termostato, GPIO_MODE_OUTPUT);

	GL_initLEDC();

	// Se inicializan las inputs
	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_ANYEDGE;

	io_conf.pin_bit_mask = GPIO_INPUT_PIN_Llamada_Paciente | GPIO_INPUT_PIN_Presencia_Asistencia | GPIO_INPUT_PIN_Subir_Persianas | GPIO_INPUT_PIN_Bajar_Persianas;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	gpio_config(&io_conf);

	io_conf.pin_bit_mask = GPIO_INPUT_PIN_Subir_Cortinas | GPIO_INPUT_PIN_Bajar_Cortinas | GPIO_INPUT_PIN_Sensor_Ruido | GPIO_INPUT_PIN_Sensor_Luz | GPIO_INPUT_PIN_Switch;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);

	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

	gpio_isr_handler_add(Boton_Llamada_Paciente, gpio_isr_handler, (void*) Boton_Llamada_Paciente);
	gpio_isr_handler_add(Boton_Presencia_Asistencia, gpio_isr_handler, (void*) Boton_Presencia_Asistencia);
	gpio_isr_handler_add(Boton_Subir_Persianas, gpio_isr_handler, (void*) Boton_Subir_Persianas);
	gpio_isr_handler_add(Boton_Bajar_Persianas, gpio_isr_handler, (void*) Boton_Bajar_Persianas);
	gpio_isr_handler_add(Boton_Subir_Cortinas, gpio_isr_handler, (void*) Boton_Subir_Cortinas);
	gpio_isr_handler_add(Boton_Bajar_Cortinas, gpio_isr_handler, (void*) Boton_Bajar_Cortinas);

	gpio_isr_handler_add(Input_Sensor_Ruido, gpio_isr_handler, (void*) Input_Sensor_Ruido);
	gpio_isr_handler_add(Input_Sensor_Luz, gpio_isr_handler, (void*) Input_Sensor_Luz);

	gpio_isr_handler_add(Input_switch, gpio_isr_handler, (void*) Input_switch);


	// Se establece el numero de esta habitación inicialmente en función del switch
	estado = gpio_get_level(Input_switch);

	if (estado == 1)
	{
		Numero_Habitacion = 2;
	}
	else if (estado == 0)
	{
		Numero_Habitacion = 1;
	}

}

void GL_initLEDC()
{
	int ch;

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ledc_timer_config(&ledc_timer);


    ledc_channel_config_t ledc_channel[3] = {
            {
                .channel    = LEDC_CHANNEL_0,
                .duty       = 0,
                .gpio_num   = LED_Persianas,
                .speed_mode = LEDC_HIGH_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_1
            },
            {
                .channel    = LEDC_CHANNEL_1,
                .duty       = 0,
                .gpio_num   = LED_Cortinas,
                .speed_mode = LEDC_HIGH_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_1
            },
            {
                .channel    = LEDC_CHANNEL_2,
                .duty       = 0,
                .gpio_num   = LED_Luz_Habitacion,
                .speed_mode = LEDC_HIGH_SPEED_MODE,
                .hpoint     = 0,
                .timer_sel  = LEDC_TIMER_1
            }
        };

    	ledc_fade_func_install(0);

        for (ch = 0; ch < 3; ch++) {
            ledc_channel_config(&ledc_channel[ch]);
        }

        GL_setPersianas(0);
        GL_setCortinas(0);
		GL_setHabitacion(0);

}

void GL_setPersianas(uint8_t Level)
{
	ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_0,Level,0);
}

void GL_setCortinas(uint8_t Level)
{
	ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_1,Level,0);
}

void GL_setHabitacion(uint8_t Level)
{
	ledc_set_duty_and_update(LEDC_HIGH_SPEED_MODE,LEDC_CHANNEL_2,Level,0);
}

