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

#include "onewire.h"
#include "dht11_w.h"
#include "menus.h"
#include "hc_sr05.h"

uint16_t settings[] =   {100,1000};


/* PRINT SETTINGS
    Prints the settings menu and waits for user input.
    The options can be found in constants.h as SETTINGS_1.
*/
uint16_t print_settings(uint16_t option){
    
    while(1){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ %-*s │\n", DISPLAY_SETTINGS[0], "SETTINGS");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        for(uint8_t i = 1; i<3; i++){
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
        CLEAR_SCREEN;
    }
    return 1;
}



/* PRINT MEASURE TYPE
    Prints the measure type menu and waits for user input.
    The options can be found in constants.c as TEST_SETTINGS.
*/
void print_measure_type(){

    // pointer to the function that will be returned
    void (*last_test_print_serial)() = NULL;
    uint16_t option = 0;
    /*
        0 -> sensor type
        1 -> GPIO pin
        2 -> test duration
        3 -> number of samples
    */
    int temp_sensor[] = {0,1,0,0b1000000};

    while(1){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        for(int i = 0; i < 8; i++) {
            printf("│ ");
            (option == i) ? printf("%2s","->"): printf("  ");
            if(i < 4) {
                if(i == 0) {
                    printf("%-*s<%*s> │\n", (int)((float)DISPLAY_SETTINGS[0]*0.6f)-2, TEST_SETTINGS[i], (int)((float)DISPLAY_SETTINGS[0]*0.4f)-4, SENSORS[temp_sensor[i]]);
                } else if (i == 2) {
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
        float freq = (float)TEST_LENGTH_VALUES[temp_sensor[2]]/temp_sensor[3];
        switch(move_temp){
            case 1:
                if(option == 4) {
                    
                    switch(temp_sensor[0]){
                        case 0:
                            last_test_print_serial = hc_sr05_start(temp_sensor[1], freq, temp_sensor[3]);
                            break;
                        case 1:
                            last_test_print_serial = dht11_start(temp_sensor[1], freq, temp_sensor[3]);
                            break;
                        case 2:
                            //light();//measure_old(temp_sensor[2], freq, temp_sensor[0], temp_sensor[4]);
                            break;
                    }   
                }else if(option == 5){
                    last_test_print_serial = dht11_serial_mode(temp_sensor[0], freq, temp_sensor[3], temp_sensor[1]);
                }else if(option == 6){
                    if(last_test_print_serial != NULL){
                        (*last_test_print_serial)();
                    }
                }else if(option == 7){
                    return;  // exit    SETTINGS_1
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
                }else if(option == 2 && temp_sensor[option] < 15){
                    temp_sensor[option]++;
                }else if(option == 3 && temp_sensor[option] < 0b1000000000000){
                    temp_sensor[option]<<= 1;  
                }
                break;
            case 5:
                if(option == 0 && temp_sensor[option] > 0){
                    temp_sensor[option]--;
                }else if(option == 1 && temp_sensor[option] > 1){
                    temp_sensor[option]--;
                }else if(option == 2 && temp_sensor[option] > 0){
                    temp_sensor[option]--;
                }else if(option == 3 && temp_sensor[option] > 0b10000){
                    temp_sensor[option]>>= 1;  
                }
                break;
        }
        CLEAR_SCREEN;
    }
    return;
}

/* DISPLAY SETTINGS
    Prints the display settings menu.
    The options can be found in constants.h as SETTINGS_2.
*/
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
        CLEAR_SCREEN;
    }
    return 1;
}

/* APP MAIN
    Main function of the program.
    It will start the program and wait for user input to start the measurement.
*/
void app_main(void){
    
    // START
    while(1){
        printf("Press enter to start");
        if(wait_enter(0)==1) break;
        CLEAR_SCREEN;
    }

    // SETUP STATUS LED
    //setup_status_led();

    // set RED LED STATUS ON
    //gpio_set_level(GPIO_NUM_11, 1);
    //gpio_set_level(GPIO_NUM_10, 0);

    CLEAR_SCREEN;
    while(1){
        switch(print_main_menu(1)){
            case 1: // start measuring
                CLEAR_SCREEN;
                print_measure_type();
                CLEAR_SCREEN;
                break;
            case 2: // display settings
                CLEAR_SCREEN;
                display_settings();
                CLEAR_SCREEN;
                break;
            case 3: // exit
                return;
        }
    }
    CLEAR_SCREEN;
}