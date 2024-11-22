#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "rom/ets_sys.h"
#include <string.h>
#include "driver/timer.h"
#include <math.h>
#include "hal/timer_hal.h"
#include "esp_intr_types.h"

#define REPEAT_CHAR(ch, count) for(int i = 0; i < (count); i++) printf("%s", ch)

#define HOR_SEP_UP(width)    printf("┌"); REPEAT_CHAR("─", width); printf("┐\n")
#define HOR_SEP_MID(width)   printf("├"); REPEAT_CHAR("─", width); printf("┤\n")
#define HOR_SEP_DW(width)    printf("└"); REPEAT_CHAR("─", width); printf("┘\n")
#define HOR_SEP_TABLE_UP(width) printf("┌"); REPEAT_CHAR("─", (width/2)-1); printf("┬"); REPEAT_CHAR("─", (width/2)); printf("┐\n");
#define HOR_SEP_TABLE_MID(width) printf("├"); REPEAT_CHAR("─", (width/2)-1); printf("┼"); REPEAT_CHAR("─", (width/2)); printf("┤\n");
#define HOR_SEP_TABLE_DW(width) printf("└"); REPEAT_CHAR("─", (width/2)-1); printf("┴"); REPEAT_CHAR("─", (width/2)); printf("┘\n");

static uint16_t DISPLAY_SETTINGS[] = {80, 24}; // console_width, console_height

static const char *MEASURE_TYPES[] = {
    "1. Ultrasonic ranging module",\
    "2. Temperature sensor", \
    "3. Ligth sensor",\
    "4. Exit"
};

static const char *SENSORS[] = {
    "Ultrasonic ranging",
    "Temperature sensor",
    "Light sensor"
};

/*  ULTRASONIC RANGING MODULE

    GPIO22 - Echo
    GPIO23 - Trigger

    Trigger pulse: 10us

    The ultrasonic sensor works by sending a trigger pulse, waiting for 10us and then reading the Echo pin.
    The value returned is the ammount of time the echo pin was high, which is then converted to distance in cm.
    The sensor has a range of 2cm to 400cm. 
*/
void ultrasonic(){
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};

    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_22);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
    
    //install gpio isr service
    gpio_install_isr_service(0);
    
    //hook isr handler for specific gpio pin
    //gpio_isr_handler_add(GPIO_NUM_22, get_timer_value, (void*) GPIO_NUM_22);


    timer_group_t timer_group = 0;
    timer_idx_t timer_idx = 0;

    timer_config_t config = {
        .divider = 16,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = false,
    }; // default clock source is APB
    timer_init(timer_group, timer_idx, &config);

    esp_err_t err = timer_init(timer_group, timer_idx, &config);
    if(err != ESP_OK){
        ESP_LOGE("Ultrasonic", "Failed to initialize timer");
    }
    gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_22, GPIO_MODE_INPUT);

    // send trigger pulse
    gpio_set_level(GPIO_NUM_23, 1);
    ets_delay_us(10);
    gpio_set_level(GPIO_NUM_23, 0);
    ets_delay_us(10);
    timer_start(timer_group, timer_idx);
}

/*  TEMPERATURE SENSOR

    GPIO1 - ADC1

    Parameters:
    - GPIO_PIN: GPIO pin number
    - freq: sampling frequency
    - test_length: test duration

*/
void temperature(int GPIO_PIN, float freq, int test_length){

    // set GREEN LED STATUS ON
    gpio_set_level(11, 0);
    gpio_set_level(10, 1);

    int avg_values[test_length];    // AVERAGE VALUES ARRAY
    
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_12); //prints info
    printf("\e[1;1H\e[2J");
    int last_avg = 0;
    int last_vals[16];  
    int delta_num = 0;
    float delta_perc = 0.0;
    int counter = 0;
    
    for(int i = 0; i < test_length/16; i++){

        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ MEASURING%*s │\n", DISPLAY_SETTINGS[0]-11, "Temperature sensor");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s %*ld bytes │\n", 15, "Free memory", DISPLAY_SETTINGS[0]-24, esp_get_free_heap_size());
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-10s %-*.3f │\n",  "Sampling frequency", DISPLAY_SETTINGS[0]-21, freq);
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s ", (DISPLAY_SETTINGS[0]/2)-3, "Raw value");
        printf("│ %-*s %*d %*c │\n", 14, "Last avg value", 4, last_avg, (DISPLAY_SETTINGS[0]/2)-22, ' ');
        HOR_SEP_TABLE_MID(DISPLAY_SETTINGS[0]);
        for(int i = 0; i < 16; i++){
            avg_values[i] = last_vals[i] = adc1_get_raw(ADC1_CHANNEL_1);

            if(last_avg != 0){
                delta_num = last_vals[i] - last_avg;
                delta_perc = (delta_num/(float)last_avg)*100;
            }    
            printf("│   Raw val(%*d%c): %*d", 3, counter, 's', 4, last_vals[i]); 
            printf("%*c", DISPLAY_SETTINGS[0]-(21+20), ' ');
            printf("\u0394: %*d/%6.2f%%   │\n", 5, delta_num, delta_perc);
            vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));
            
            counter++;
        }
        last_avg = 0;
        for(int i = 0; i < 16; i++){
            last_avg += last_vals[i];
        }
        last_avg /= 16;

        HOR_SEP_TABLE_DW(DISPLAY_SETTINGS[0]);

        
        printf("\e[1;1H\e[2J");
    }

    // set RED LED STATUS ON -> end of measurement
    gpio_set_level(GPIO_NUM_11, 1);
    gpio_set_level(GPIO_NUM_10, 0);

    return;
}

void light(){

}

void measure(int GPIO_PIN, float freq, int measure_type, int test_length){

    // set GREEN LED STATUS ON
    gpio_set_level(11, 0);
    gpio_set_level(10, 1);

    int avg_values[test_length];    // AVERAGE VALUES ARRAY
    
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_12); //prints info
    printf("\e[1;1H\e[2J");
    int last_avg = 0;
    int last_vals[16];  
    int delta_num = 0;
    float delta_perc = 0.0;
    int counter = 0;
    
    for(int i = 0; i < test_length/16; i++){

        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ MEASURING%*s │\n", DISPLAY_SETTINGS[0]-11, MEASURE_TYPES[measure_type]);
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s %*ld bytes │\n", 15, "Free memory", DISPLAY_SETTINGS[0]-24, esp_get_free_heap_size());
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-10s %-*.3f │\n",  "Sampling frequency", DISPLAY_SETTINGS[0]-21, freq);
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s ", (DISPLAY_SETTINGS[0]/2)-3, "Raw value");
        printf("│ %-*s %*d %*c │\n", 14, "Last avg value", 4, last_avg, (DISPLAY_SETTINGS[0]/2)-22, ' ');
        HOR_SEP_TABLE_MID(DISPLAY_SETTINGS[0]);
        for(int i = 0; i < 16; i++){
            avg_values[i] = last_vals[i] = adc1_get_raw(ADC1_CHANNEL_1);

            if(last_avg != 0){
                delta_num = last_vals[i] - last_avg;
                delta_perc = (delta_num/(float)last_avg)*100;
            }    
            printf("│   Raw val(%*d%c): %*d", 3, counter, 's', 4, last_vals[i]); 
            printf("%*c", DISPLAY_SETTINGS[0]-(21+20), ' ');
            printf("\u0394: %*d/%6.2f%%   │\n", 5, delta_num, delta_perc);
            vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));
            
            counter++;
        }
        last_avg = 0;
        for(int i = 0; i < 16; i++){
            last_avg += last_vals[i];
        }
        last_avg /= 16;

        HOR_SEP_TABLE_DW(DISPLAY_SETTINGS[0]);

        
        printf("\e[1;1H\e[2J");
    }

    // set RED LED STATUS ON -> end of measurement
    gpio_set_level(GPIO_NUM_11, 1);
    gpio_set_level(GPIO_NUM_10, 0);

    return;
}