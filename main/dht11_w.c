#include <stdio.h>
#include "dht.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "constants.h"
#include "menus.h"

typedef void (*ptr)();
ptr dht11_start(gpio_num_t GPIO_PIN, float freq, int test_length);
ptr dht11_serial_mode(int type, float freq, int samples, gpio_num_t GPIO_PIN);
static void print_summary(int test_length, float *temp, float *hum);
static void serial_print();

static float *temp_array = NULL;
static float *hum_array = NULL;

static int test_length_i = 0;

/*  TEMPERATURE SENSOR

    GPIO1 - ADC1

    Parameters:
    - GPIO_PIN: GPIO pin number
    - freq: sampling frequency
    - test_length: test duration

*/
ptr dht11_start(gpio_num_t GPIO_PIN, float freq, int test_length){
    test_length_i = test_length;
    CLEAR_SCREEN;
    float last_avg_temp = 0;
    float last_avg_hum = 0;
    float last_vals_temp[16] = {0};  
    float last_vals_hum[16] = {0};  
    float delta_num_temp = 0;
    float delta_perc_temp = 0.0;
    float delta_num_hum = 0;
    float delta_perc_hum = 0.0;
    int counter = 0;
    if(temp_array != NULL){
        free(temp_array);
        free(hum_array);
    }
    temp_array = malloc(test_length * sizeof(float));
    hum_array = malloc(test_length * sizeof(float));

    for(int i = 0; i < test_length/16; i++){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ MEASURING%*s │\n", DISPLAY_SETTINGS[0]-11, "Temperature/humidity");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s %*ld bytes │\n", 15, "Free memory", DISPLAY_SETTINGS[0]-24, esp_get_free_heap_size());
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-10s %-*.3f │\n",  "Sampling frequency", DISPLAY_SETTINGS[0]-21, freq);
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s ", (DISPLAY_SETTINGS[0]/2)-3, "Raw value");
        printf("│ %-*s %*.1f %*c │\n", 14, "Last temp avg", 4, last_avg_temp, (DISPLAY_SETTINGS[0]/2)-22, ' ');
        printf("│ %*s %-*s %*.1f %*c │\n", (DISPLAY_SETTINGS[0]/2)-3, " ", 18, "│ Last hum avg", 4, last_avg_hum, (DISPLAY_SETTINGS[0]/2)-22, ' ');
        HOR_SEP_TABLE_MID(DISPLAY_SETTINGS[0]);
        for(int j = 0; j < 16; j++){
            dht_read_float_data(DHT_TYPE_DHT11, GPIO_PIN, &last_vals_hum[j], &last_vals_temp[j]);
            temp_array[i*16+j] = last_vals_temp[j];
            hum_array[i*16+j] = last_vals_hum[j];
            if(last_avg_temp != 0){
                delta_num_temp = last_vals_temp[j] - last_avg_temp;
                delta_num_hum = last_vals_hum[j] - last_avg_hum;
                delta_perc_temp = (delta_num_temp/(float)last_avg_temp)*100;
                delta_perc_hum = (delta_num_hum/(float)last_avg_hum)*100;
            }    
            printf("│   Temp_rw(%*d): %*.1f", 4, counter, 4, last_vals_temp[j]); 
            printf("%*c", DISPLAY_SETTINGS[0]-(21+20), ' ');
            printf("\u0394: %*.1f/%6.2f%%   │\n", 5, delta_num_temp, delta_perc_temp);
            printf("│   Humd_rw(%*d): %*.1f", 4, counter, 4, last_vals_hum[j]); 
            printf("%*c", DISPLAY_SETTINGS[0]-(21+20), ' ');
            printf("\u0394: %*.1f/%6.2f%%   │\n", 5, delta_num_hum, delta_perc_hum);
            vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));
            
            counter++;
        }
        last_avg_temp = 0;
        last_avg_hum = 0;
        for(int j = 0; j < 16; j++){
            last_avg_temp += last_vals_temp[j];
            last_avg_hum += last_vals_hum[j];
        }
        last_avg_temp /= 16;
        last_avg_hum /= 16;
        HOR_SEP_TABLE_DW(DISPLAY_SETTINGS[0]);

        
        printf("\e[1;1H\e[2J");
        
    }
    print_summary(test_length, temp_array, hum_array);
    return &serial_print;
}

/* SERIAL MODE
    Measures the sensor in a loop for a given number of samples.
    Prints the raw value in a serial format.

    The format is:
    \start
    <raw value>
    <raw value>
    ...
    \end

    This function is intended to be used when the user wants to later process the data
    in other software, like Matlab, Python, etc.

    By using Screen utility in Linux, with the parameter -L, the user can interact 
    as the same time that all the output is stored in a file
*/
ptr dht11_serial_mode(int type, float freq, int samples, gpio_num_t GPIO_PIN){
    test_length_i = samples;
    CLEAR_SCREEN;
    print_header_in_test(type, freq, 0);
    printf("\\start");
    wait_enter(0);
    if(temp_array != NULL){
        free(temp_array);
        free(hum_array);
    }
    temp_array = malloc(samples * sizeof(float));
    hum_array = malloc(samples * sizeof(float));
    printf("Temperature,Humidity\n");
    for(int i = 0; i < samples; i++){
        dht_read_float_data(DHT_TYPE_DHT11, GPIO_PIN, &hum_array[i], &temp_array[i]);
        printf("%f,%f\n", temp_array[i], hum_array[i]);
        vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));
    }
    wait_enter(0);
    printf("\\end");
    wait_enter(0);
    return &serial_print;
}

/* PRINT SUMMARY
    Prints the summary menu and waits for user input.
*/
static void print_summary(int test_length, float *temp, float *hum){

    // CALCULATE AVERAGE AND MAX AND MIN
    float avg_temp = 0;
    float avg_hum = 0;
    float max_temp = temp[0];
    float min_temp = temp[0];
    float max_hum = hum[0];
    float min_hum = hum[0];
    for(int i = 0; i < test_length; i++){
        avg_temp += temp[i];
        avg_hum += hum[i];
        if(temp[i] > max_temp) max_temp = temp[i];
        if(temp[i] < min_temp) min_temp = temp[i];
        if(hum[i] > max_hum) max_hum = hum[i];
        if(hum[i] < min_hum) min_hum = hum[i];
    }
    avg_temp /= test_length;
    avg_hum /= test_length;

    HOR_SEP_UP(DISPLAY_SETTINGS[0]);
    printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "SUMMARY");
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
    /* SENSOR INFO 
        ACCURACY:
        TEMPERATURE -> +-2ºC
        HUMIDITY -> +-5%

        RESOLUTION:
        Temperature -> 1ºC (8-bit)
        Humidity -> 1% (8-bit)

        RANGE:
        Temperature -> 0ºC to 50ºC
        Humidity -> 20% to 90%
    */
    printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "SENSOR INFO -> Accuracy: +-2ºC +-5%%");
    printf("│ %-*s ", (DISPLAY_SETTINGS[0]/2)-2, "TEMPERATURE");
    printf(" %-*s │\n", (DISPLAY_SETTINGS[0]/2)-2, "HUMIDITY");
    HOR_SEP_TABLE_MID(DISPLAY_SETTINGS[0]);

    printf("│ %-*s %-*.1f │", 15, "Average", (DISPLAY_SETTINGS[0]/2)-19,avg_temp);
    printf(" %-*s %-*.1f │\n", 15, "Average", (DISPLAY_SETTINGS[0]/2)-18,avg_hum);
    printf("│ %-*s %-*.1f │", 15, "Max", (DISPLAY_SETTINGS[0]/2)-19,max_temp);
    printf(" %-*s %-*.1f │\n", 15, "Max", (DISPLAY_SETTINGS[0]/2)-18,max_hum);
    printf("│ %-*s %-*.1f │", 15, "Min", (DISPLAY_SETTINGS[0]/2)-19,min_temp);
    printf(" %-*s %-*.1f │\n", 15, "Min", (DISPLAY_SETTINGS[0]/2)-18,min_hum);
    printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "-> Exit");
    HOR_SEP_DW(DISPLAY_SETTINGS[0]);

    if(wait_enter(0) == 1) return;
}

static void serial_print(){
    printf("\\start DHT11\n");
    for(int i = 0; i < test_length_i; i++){
        printf("%f, %f\n", temp_array[i], hum_array[i]);
    }
    printf("\\end\n");
    wait_enter(0);
}