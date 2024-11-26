#include <stdio.h>
#include "dht.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "constants.h"
#include "menus.h"

void dht11_start(gpio_num_t GPIO_PIN, float freq, int test_length);
static void print_summary(int test_length, float *temp, float *hum);


/*  TEMPERATURE SENSOR

    GPIO1 - ADC1

    Parameters:
    - GPIO_PIN: GPIO pin number
    - freq: sampling frequency
    - test_length: test duration

*/
void dht11_start(gpio_num_t GPIO_PIN, float freq, int test_length){

    printf("\e[1;1H\e[2J");
    float last_avg_temp = 0;
    float last_avg_hum = 0;
    float last_vals_temp[16] = {0};  
    float last_vals_hum[16] = {0};  
    float delta_num_temp = 0;
    float delta_perc_temp = 0.0;
    float delta_num_hum = 0;
    float delta_perc_hum = 0.0;
    int counter = 0;
    float temp[test_length];
    float hum[test_length];

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
            temp[i*16+j] = last_vals_temp[j];
            hum[i*16+j] = last_vals_hum[j];
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
        print_summary(test_length, temp, hum);
    }
    return;
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

