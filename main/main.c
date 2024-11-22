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
#include "constants.h"
#include "sensors.h"


#define REPEAT_CHAR(ch, count) for(int i = 0; i < (count); i++) printf("%s", ch)

#define HOR_SEP_UP(width)    printf("┌"); REPEAT_CHAR("─", width); printf("┐\n")
#define HOR_SEP_MID(width)   printf("├"); REPEAT_CHAR("─", width); printf("┤\n")
#define HOR_SEP_DW(width)    printf("└"); REPEAT_CHAR("─", width); printf("┘\n")
#define HOR_SEP_TABLE_UP(width) printf("┌"); REPEAT_CHAR("─", (width/2)-1); printf("┬"); REPEAT_CHAR("─", (width/2)); printf("┐\n");
#define HOR_SEP_TABLE_MID(width) printf("├"); REPEAT_CHAR("─", (width/2)-1); printf("┼"); REPEAT_CHAR("─", (width/2)); printf("┤\n");
#define HOR_SEP_TABLE_DW(width) printf("└"); REPEAT_CHAR("─", (width/2)-1); printf("┴"); REPEAT_CHAR("─", (width/2)); printf("┘\n");

// Doesn't work for some reason
//#define HOR_SEP_UP(width)    printf("┌%-*c┐\n", width, 126)
//#define HOR_SEP_MID(width)   printf("├%-*c┤\n", width, 196)
//#define HOR_SEP_DW(width)    printf("└%-*c┘\n", width, 196)



typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;

/**
 * @brief A sample structure to pass events from the timer ISR to task
 *
 */
typedef struct {
    example_timer_info_t info;
    uint64_t timer_counter_value;
} example_timer_event_t;


static uint16_t settings[] =   {100,1000};

static const char *TAG = "example";

int wait_enter(float wait_time){
    fpurge(stdin); //clears any junk in stdin
    char bufp[10];
    int timeout = 100000;
    int rep = (wait_time == 0) ? timeout/100 : wait_time/100;

    for(int i = 0; i < rep; i++){
        vTaskDelay(pdMS_TO_TICKS(100));
        *bufp = getchar();
        if(*bufp != '\0' && *bufp != 0xFF && *bufp != '\r') //ignores null input, 0xFF, CR in CRLF
        {
            //'enter' (EOL) handler 
            if(*bufp == '\n'){
                *bufp = '\0';
                return 1;
            }
            if(*bufp == '\033'){
                getchar();
                switch(getchar()){
                    case 'A': // arrow up
                        return 2;
                    case 'B': // arrow down
                        return 3;
                    case 'C': // arrow right
                        return 4;
                    case 'D': // arrow left
                        return 5;
                    default:

                        ;
                }
            }
        }
    }
    return -1;

}

/*
    Clears the screen by moving the cursor to the top left corner
    and then clearing the screen.

    \e[1;1H moves the cursor to the top left corner
        - ESC[{line};{column}H
    \e[2J erases the screen
        - ESC[2J
*/
void clear_screen(){
    printf("\e[1;1H\e[2J");
}

int print_main_menu(int option){
    // HOR_SEP_UP;
    while(1){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "ESP32 SENSOR MEASUREMENT TOOL");
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        for(uint8_t i = 1; i<5;i++){
            printf("│ ");
            (option == i) ? printf("%2s","->"): printf("  ");
            printf("%-*s │\n", DISPLAY_SETTINGS[0]-4, MENU_STRING[i-1]);
        }
        HOR_SEP_DW(DISPLAY_SETTINGS[0]);
        uint8_t move_temp = wait_enter(0);
        if(move_temp == 1){
            return option;
        }else{
            if(move_temp == 2){
                if(option != 1){
                    option--;
                }
            }
            if(move_temp == 3){
                if(option != 4){
                    option++;
                }
            }
        }
        clear_screen();
    }
    return 1;
}

uint16_t print_settings(uint16_t option){
    
    while(1){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ %-*s │\n", DISPLAY_SETTINGS[0], "SETTINGS");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        for(uint8_t i = 1; i<4; i++){
            printf("│ ");
            (option == i) ? printf("%2s","->"): printf("  ");
            printf("%-*s │\n", DISPLAY_SETTINGS[0], SETTINGS_1[i-1]);
        }
        HOR_SEP_DW(DISPLAY_SETTINGS[0]);
        uint8_t move_temp = wait_enter(0);
        if(move_temp == 1){
            return option;
        }else{
            if(move_temp == 2){
                if(option != 1){
                    option--;
                }
            }
            if(move_temp == 3){
                if(option != 4){
                    option++;
                }
            }
        }
        clear_screen();
    }
    return 1;
}

void print_summary(int *avg_values, int test_length){
    HOR_SEP_UP(DISPLAY_SETTINGS[0]);
    printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "SUMMARY");
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);

    printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "-> Exit");
    HOR_SEP_DW(DISPLAY_SETTINGS[0]);

    if(wait_enter(0) == 1) return;
}

static void IRAM_ATTR get_timer_value(){
    timer_pause(0,0);
    printf("timer value: %lld\n", timer_group_get_counter_value_in_isr(0,0));
    //return timer_group_get_counter_value_in_isr(0,0);
}


int measuring_settings(){
    int option = 1;
    
    while(1){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ %-*s │\n", DISPLAY_SETTINGS[0], "SETTINGS");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        for(int i = 1; i<4; i++){
            printf("│ ");
            (option == i) ? printf("->"): printf("  ");
            (i<3)?printf("%-28s <%7d> │\n", SETTINGS_1[i-1], settings[i-1]) : printf("%-38s │\n", SETTINGS_1[i-1]);
        }
        
        HOR_SEP_DW(DISPLAY_SETTINGS[0]);
        uint8_t move_temp = wait_enter(0);
        if(move_temp == 1 && option == 3){
            return option;
        }else{
            if(move_temp == 2){
                if(option != 1){
                    option--;
                }
            }
            if(move_temp == 3){
                if(option != 3){
                    option++;
                }
            }
            if(move_temp == 4){
                settings[option-1]+=100;
            }
            if(move_temp == 5){
                settings[option-1]--;
            }
        }
        clear_screen();
    }
}

void print_measure_type(){

    uint16_t option = 0;
    int temp_sensor[] = {0,1,1000,0,0b1000000};

    while(1){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        for(int i = 0; i < 7; i++) {
            printf("│ ");
            (option == i) ? printf("%2s","->"): printf("  ");
            if(i < 5) {
                if(i == 0) {
                    printf("%-*s<%*s> │\n", (int)((float)DISPLAY_SETTINGS[0]*0.6f)-2, TEST_SETTINGS[i], (int)((float)DISPLAY_SETTINGS[0]*0.4f)-4, SENSORS[temp_sensor[i]]);
                } else if (i == 3) {
                    printf("%-*s<%*s> │\n", (int)((float)DISPLAY_SETTINGS[0]*0.6f)-2, TEST_SETTINGS[i], (int)((float)DISPLAY_SETTINGS[0]*0.4f)-4, TEST_LENGTH[temp_sensor[i]]);
                } else {
                    printf("%-*s<%*d> │\n", (int)((float)DISPLAY_SETTINGS[0]*0.6f)-2, TEST_SETTINGS[i], (int)((float)DISPLAY_SETTINGS[0]*0.4f)-4, temp_sensor[i]);
                }
            } else {
                printf("%-*s │\n", DISPLAY_SETTINGS[0]-4, TEST_SETTINGS[i]);
            }
        }

        HOR_SEP_DW(DISPLAY_SETTINGS[0]);
        uint8_t move_temp = wait_enter(0);
        float freq = (float)TEST_LENGTH_VALUES[temp_sensor[3]]/temp_sensor[4];
        switch(move_temp){
            case 1:
                if(option == 5) {
                    gpio_set_level(GPIO_NUM_11, 0);
                    gpio_set_level(GPIO_NUM_10, 1);
                    switch(temp_sensor[0]){
                        case 0:
                            ultrasonic();
                            break;
                        case 1:
                            temperature(temp_sensor[1], freq, temp_sensor[4]);
                            break;
                        case 2:
                            //measure_old(temp_sensor[2], freq, temp_sensor[0], temp_sensor[4]);
                            break;
                    }   
                    gpio_set_level(GPIO_NUM_11, 1);
                    gpio_set_level(GPIO_NUM_10, 0);
                }else if(option == 6){
                    return;  // exit    
                }   
                break;
            case 2:
                if(option != 0) option--;
                break;
            case 3:
                if(option != 6) option++;
                break;
            case 4:
                if(option == 0 && temp_sensor[option] < 2){
                    temp_sensor[option]++;
                }else if(option == 1 && temp_sensor[option] < 32){
                    temp_sensor[option]++;
                }else if(option == 2 && temp_sensor[option] < 5000){
                    temp_sensor[option]+=100;
                }else if(option == 3 && temp_sensor[option] < 15){
                    temp_sensor[option]++;
                }else if(option == 4 && temp_sensor[option] < 0b1000000000000){
                    temp_sensor[option]<<= 1;  
                }
                break;
            case 5:
                if(option == 0 && temp_sensor[option] > 0){
                    temp_sensor[option]--;
                }else if(option == 1 && temp_sensor[option] > 1){
                    temp_sensor[option]--;
                }else if(option == 2 && temp_sensor[option] > 100){
                    temp_sensor[option]-=100;
                }else if(option == 3 && temp_sensor[option] > 0){
                    temp_sensor[option]--;
                }else if(option == 4 && temp_sensor[option] > 0b10000){
                    temp_sensor[option]>>= 1;  
                }
                break;
        }
        clear_screen();
    }
    return;
}

void setup_status_led(){

    // RED STATUS LED
    // set GPIO pin 11 to OUTPUT mode;
    // check for errors and inform in case
    esp_err_t dir_11 = gpio_set_direction(GPIO_NUM_11, GPIO_MODE_OUTPUT);
    if (dir_11 == ESP_ERR_INVALID_ARG){
        ESP_LOGE(TAG, "Invalid GPIO pin: output mode");
        printf("Invalid GPIO pin: output mode\n");
    }

    // GREEN STATUS LED
    // set GPIO pin 10 to OUTPUT mode;
    // check for errors and inform in case
    esp_err_t dir_10 = gpio_set_direction(GPIO_NUM_10, GPIO_MODE_OUTPUT);
    if (dir_10 == ESP_ERR_INVALID_ARG){
        ESP_LOGE(TAG, "Invalid GPIO pin: output mode");
    }
}

int display_settings() {
    int option = 1;
    
    while(1) {
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "DISPLAY SETTINGS");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        
        for(int i = 1; i<4; i++) {
            printf("│ ");
            (option == i) ? printf("->"): printf("  ");
            (i<3) ? printf("%-*s <%*d> │\n", (int)((float)DISPLAY_SETTINGS[0]*0.8f)-3, SETTINGS_2[i-1], (int)((float)DISPLAY_SETTINGS[0]*0.2f)-4, DISPLAY_SETTINGS[i-1]) 
                  : printf("%-*s │\n", DISPLAY_SETTINGS[0]-4, SETTINGS_2[i-1]);
        }
        
        HOR_SEP_DW(DISPLAY_SETTINGS[0]);
        uint8_t move_temp = wait_enter(0);
        
        if(move_temp == 1 && option == 3) {
            return option;
        } else {
            if(move_temp == 2) {
                if(option != 1) {
                    option--;
                }
            }
            if(move_temp == 3) {
                if(option != 3) {
                    option++;
                }
            }
            if(move_temp == 4) {
                if(option == 1 && DISPLAY_SETTINGS[option-1] < 120) {
                    DISPLAY_SETTINGS[option-1] += 20;
                } else if(option == 2 && DISPLAY_SETTINGS[option-1] < 40) {
                    DISPLAY_SETTINGS[option-1]++;
                }
            }
            if(move_temp == 5) {
                if(option == 1 && DISPLAY_SETTINGS[option-1] > 60) {
                    DISPLAY_SETTINGS[option-1] -= 20;
                } else if(option == 2 && DISPLAY_SETTINGS[option-1] > 10) {
                    DISPLAY_SETTINGS[option-1]--;
                }
            }
        }
        clear_screen();
    }
    return 1;
}

void app_main(void){
    
    // START
    while(1){
        printf("Press enter to start");
        if(wait_enter(0)==1) break;
        clear_screen();
    }

    // SETUP STATUS LED
    setup_status_led();

    // set RED LED STATUS ON
    gpio_set_level(GPIO_NUM_11, 1);
    gpio_set_level(GPIO_NUM_10, 0);

    clear_screen();
    while(1){
        switch(print_main_menu(1)){
            case 1: // start measuring
                clear_screen();
                print_measure_type();
                clear_screen();
                break;
            case 2: // measuring settings
                clear_screen();
                measuring_settings();
                clear_screen();
                break;
            case 3: // display settings
                clear_screen();
                display_settings();
                clear_screen();
                break;
            case 4: // exit
                return;
        }
    }

    // dump IO ports configuration into the console
    //gpio_dump_io_configuration(stdout, (1ULL << 10) | (1ULL << 11));

    clear_screen();


    
}