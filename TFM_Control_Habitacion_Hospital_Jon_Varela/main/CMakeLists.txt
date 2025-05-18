idf_component_register(SRCS "main.c" "comandos.c" "wifi.c" "inputs_outputs.c" "ADC.c" "json.c" "mqtt.c" "dht11.c" "lcd_16x2.c"					
     INCLUDE_DIRS "."                    	
)

# IMPORTANTE: CREA la imagen.... cuidado con el nombre que debe ser el de la particion.                    
# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'.
# partspiffs es el nombre que tiene en la tabla de particiones y debe coincidir con el c�digo que lo monta. 