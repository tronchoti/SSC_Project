#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_intr_types.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "menus.h"
#include "constants.h"

typedef void (*ptr)();

ptr hc_sr05_start(gpio_num_t GPIO_PIN, float freq, int test_length);
static void print_summary(int test_length, float *distance);
ptr hc_sr05_serial_mode(gpio_num_t GPIO_PIN, float freq, int test_length);
static void serial_print();

static QueueHandle_t gpio_evt_queue;

static float* distance_array = NULL;

static uint64_t first_time = 0;
static uint64_t last_time = 0;
static float last_distance = 0;

static bool is_first_edge = true;

static int test_length_i = 0;

/* ISR handler for the gpio
    It sends the gpio number to the queue
    Don't stop timer in ISR because it will cause a deadlock
*/
static void IRAM_ATTR gpio_isr_handler(void* arg){
    if(is_first_edge){
        is_first_edge = false;
        first_time = esp_timer_get_time();
    }else{
        is_first_edge = true;
        last_time = esp_timer_get_time();
        uint64_t time = esp_timer_get_time();
        xQueueSendFromISR(gpio_evt_queue, &time, NULL);
    }
}

/* Task to handle gpio events
    It checks if there is an event on the queue and prints the event
*/
static void gpio_event_task(void* arg){
    uint64_t time;
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &time, portMAX_DELAY)) {
            last_distance = (last_time - first_time) * 0.0343 / 2;
            //printf("Distance: %.2f cm\n", distance);
        }
    }
}

/* ULTRASONIC RANGE MODULE

    GPIO_PIN: GPIO pin number
    freq: sampling frequency
    test_length: test duration

*/
ptr hc_sr05_start(gpio_num_t GPIO_PIN, float freq, int test_length){
    test_length_i = test_length;
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << GPIO_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    
    gpio_config(&io_conf);

    /* create queue to handle gpio events */
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    if(gpio_evt_queue == 0){
        ESP_LOGE("HC_SR05", "Failed to create event queue");
        return NULL;
    }

    /* create task to handle gpio events 
    - gpio_event_task: task function
    - "gpio_event_task": task name
    - 2048: stack size
    - NULL: task parameters
    - 5: task priority
    - NULL: task handle
    */
    xTaskCreate(gpio_event_task, "gpio_event_task", 2048, NULL, 5, NULL);

    // install gpio isr service
    // this one provides individual gpio interrupts/handlers
    gpio_install_isr_service(0);

    gpio_isr_handler_add(GPIO_PIN, gpio_isr_handler, NULL);

    gpio_set_direction(10, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_PIN, GPIO_MODE_INPUT);
    
    float last_avg_distance = 0;
    float delta_num_distance = 0;
    float delta_perc_distance = 0;
    int counter = 0;

    distance_array = malloc(test_length * sizeof(float));
    CLEAR_SCREEN;
    for(int i = 0; i < test_length/16; i++){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ MEASURING%*s │\n", DISPLAY_SETTINGS[0]-11, "Ultrasonic range module");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s %*ld bytes │\n", 15, "Free memory", DISPLAY_SETTINGS[0]-24, esp_get_free_heap_size());
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-10s %-*.3f │\n",  "Sampling frequency", DISPLAY_SETTINGS[0]-21, freq);
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s ", (DISPLAY_SETTINGS[0]/2)-3, "Raw value");
        printf("│ %-*s %*.1f %*c │\n", 20, "Last distance avg", 5, last_avg_distance, (DISPLAY_SETTINGS[0]/2)-29, ' ');
        HOR_SEP_TABLE_MID(DISPLAY_SETTINGS[0]);
        for(int j = 0; j < 16; j++){
            // Trigger pulse
            gpio_set_level(10, 0);
            gpio_set_level(10, 1);
            esp_rom_delay_us(10);
            gpio_set_level(10, 0);
            if(last_distance == 0){
                vTaskDelay(pdMS_TO_TICKS(500));
            }
            distance_array[i*16+j] = last_distance;

            if(last_avg_distance != 0){
                delta_num_distance = last_distance - last_avg_distance;
                delta_perc_distance = (delta_num_distance/(float)last_avg_distance)*100;
            }
            printf("│   Dist_rw(%*d): %*.1f", 4, counter++, 5, last_distance); 
            printf("%*c", DISPLAY_SETTINGS[0]-(22+23), ' ');
            printf("\u0394: %*.2f/%6.2f%%   │\n", 8, delta_num_distance, delta_perc_distance);
            vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));
        }
        last_avg_distance = 0;
        for(int j = 0; j < 16; j++){
            last_avg_distance += distance_array[i*16+j];
        }
        last_avg_distance /= 16;
        HOR_SEP_TABLE_DW(DISPLAY_SETTINGS[0]);
        CLEAR_SCREEN;
    }
    print_summary(test_length, distance_array);
    return &serial_print;
}

ptr hc_sr05_serial_mode(gpio_num_t GPIO_PIN, float freq, int test_length){
    CLEAR_SCREEN;

    printf("\\start\n");
    wait_enter(0);
    if(distance_array != NULL){
        free(distance_array);
    }
    distance_array = malloc(test_length * sizeof(float));
    printf("Distance\n");
    for(int i = 0; i < test_length; i++){
        distance_array[i] = last_distance;
        printf("%f\n", distance_array[i]);
        vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));
    }
    wait_enter(0);
    printf("\\end\n");
    wait_enter(0);
    return &serial_print;
}

/* Print the summary of the test */
static void print_summary(int test_length, float *distance){
    float avg_distance = 0;
    float max_distance = distance[0];
    float min_distance = distance[0];
    for(int i = 0; i < test_length; i++){
        avg_distance += distance[i];
        if(distance[i] > max_distance) max_distance = distance[i];
        if(distance[i] < min_distance) min_distance = distance[i];
    }
    avg_distance /= test_length;

    HOR_SEP_UP(DISPLAY_SETTINGS[0]);
    printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "SUMMARY");
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
    printf("│ %-*s %-*.1f %*c │\n", 20, "Average distance", 7, avg_distance, DISPLAY_SETTINGS[0]-31, ' ');
    printf("│ %-*s %-*.1f %*c │\n", 20, "Max distance", 7, max_distance, DISPLAY_SETTINGS[0]-31, ' ');
    printf("│ %-*s %-*.1f %*c │\n", 20, "Min distance", 7, min_distance, DISPLAY_SETTINGS[0]-31, ' ');
    HOR_SEP_DW(DISPLAY_SETTINGS[0]);

    if(wait_enter(0) == 1) return;
}

static void serial_print(){
    printf("\\start\n");
    for(int i = 0; i < test_length_i; i++){
        printf("%f\n", distance_array[i]);
    }
    printf("\\end\n");
    wait_enter(0);
}