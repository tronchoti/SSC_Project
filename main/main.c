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
#include "dht.h"

#define REPEAT_CHAR(ch, count) for(int i = 0; i < (count); i++) printf("%s", ch)

#define HOR_SEP_UP(width)    printf("┌"); REPEAT_CHAR("─", width); printf("┐\n")
#define HOR_SEP_MID(width)   printf("├"); REPEAT_CHAR("─", width); printf("┤\n")
#define HOR_SEP_DW(width)    printf("└"); REPEAT_CHAR("─", width); printf("┘\n")
#define HOR_SEP_TABLE_UP(width) printf("┌"); REPEAT_CHAR("─", (width/2)-1); printf("┬"); REPEAT_CHAR("─", (width/2)); printf("┐\n");
#define HOR_SEP_TABLE_MID(width) printf("├"); REPEAT_CHAR("─", (width/2)-1); printf("┼"); REPEAT_CHAR("─", (width/2)); printf("┤\n");
#define HOR_SEP_TABLE_DW(width) printf("└"); REPEAT_CHAR("─", (width/2)-1); printf("┴"); REPEAT_CHAR("─", (width/2)); printf("┘\n");

#define CLEAR_SCREEN printf("\e[1;1H\e[2J");
// Doesn't work for some reason
//#define HOR_SEP_UP(width)    printf("┌%-*c┐\n", width, 126)
//#define HOR_SEP_MID(width)   printf("├%-*c┤\n", width, 196)
//#define HOR_SEP_DW(width)    printf("└%-*c┘\n", width, 196)

uint16_t settings[] =   {100,1000};

static const char *TAG = "example";

/* WAIT ENTER
    Waits for user input.
    If wait_time is 0, it waits for timeout/100 iterations.
    Otherwise, it waits for wait_time/100 iterations.
*/
int wait_enter(float wait_time){
    fpurge(stdin); //clears any junk in stdin
    char bufp[10];
    int timeout = 100000;   // 100s timeout 
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

/* PRINT MAIN MENU
    Prints the main menu and waits for user input.
    The options can be found in constants.h as MENU_STRING.
*/
int print_main_menu(int option){
    while(1){
        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "ESP32 SENSOR MEASUREMENT TOOL");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        for(uint8_t i = 1; i<4;i++){
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
        CLEAR_SCREEN;
    }
    return 1;
}

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

/* PRINT HEADER IN TEST
    Prints the header of the in_test function.
    It will show the sensor type, the sampling frequency and the last average value.
*/
void print_header_in_test(int type, float freq, int last_avg){
    HOR_SEP_UP(DISPLAY_SETTINGS[0]);
    printf("│ MEASURING%*s │\n", DISPLAY_SETTINGS[0]-11, MEASURE_TYPES[type]);
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
    printf("│ %-*s %*ld bytes │\n", 15, "Free memory", DISPLAY_SETTINGS[0]-24, esp_get_free_heap_size());
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
    printf("│ %-10s %-*.3f │\n",  "Sampling frequency", DISPLAY_SETTINGS[0]-21, freq);
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
    printf("│ %-*s ", (DISPLAY_SETTINGS[0]/2)-3, "Raw value");
    printf("│ %-*s %*d %*c │\n", 14, "Last avg value", 4, last_avg, (DISPLAY_SETTINGS[0]/2)-22, ' ');
    HOR_SEP_TABLE_MID(DISPLAY_SETTINGS[0]);
}

/* IN_TEST
    Measures the sensor in a loop for a given number of samples.
    Prints the raw value, its delta to the last average value and the percentage of delta.

    The main point of this function is to generalize the measurement process, with only
    changing in some sensors, where special measure processes are needed.
    Before this function, each sensor had its own function, but now it's not necessary.
*/
void in_test(int GPIO_PIN, float freq, int type, int samples){

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_12);
    
    CLEAR_SCREEN;
    int avg_values[samples];
    int last_avg = 0;           // LAST AVERAGE VALUE
    int last_vals[16];          // LAST VALUES ARRAY
    int delta_num = 0;          // DELTA NUMBER
    float delta_perc = 0.0;     // DELTA PERCENTAGE
    int counter = 0;            // COUNTER

    int temperature = 0;
    int humidity = 0;

    for(int i = 0; i < samples/16; i++){
        print_header_in_test(type, freq, last_avg);
        
        // print values in same page, up to 16 values
        for(int i = 0; i < 16; i++){
            //dht_read_data(DHT_TYPE_DHT11, GPIO_NUM_22, &temperature, &humidity);
            printf("Temperature: %d\n", temperature);
            printf("Humidity: %d\n", humidity);
            avg_values[i] = last_vals[i] = adc1_get_raw(ADC1_CHANNEL_1);
            if(last_avg != 0){
            delta_num = last_vals[i] - last_avg;
                delta_perc = (delta_num/(float)last_avg)*100;
            }
            
            // print raw value, it's delta to the last average value and the percentage of delta
            printf("│   Raw val(%*d): %*d", 3, counter, 4, last_vals[i]); 
            printf("%*c", DISPLAY_SETTINGS[0]-(20+20), ' ');
            printf("\u0394: %*d/%6.2f%%   │\n", 5, delta_num, delta_perc);
            // I should fix this hardcoded print above

            // delay between samples
            vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));

            counter++;
        }
        // calculate the last average value
        last_avg = 0;   
        for(int i = 0; i < 16; i++){
            last_avg += last_vals[i];
        }
        last_avg /= 16;

        HOR_SEP_TABLE_DW(DISPLAY_SETTINGS[0]);

        CLEAR_SCREEN;
    }
}

/* PRINT SUMMARY
    Prints the summary menu and waits for user input.
*/
void print_summary(int16_t *avg_values, int test_length){
    HOR_SEP_UP(DISPLAY_SETTINGS[0]);
    printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "SUMMARY");
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);

    printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "-> Exit");
    HOR_SEP_DW(DISPLAY_SETTINGS[0]);

    if(wait_enter(0) == 1) return;
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
void serial_mode(int type, float freq, int samples){
    print_header_in_test(type, freq, 0);
    printf("\\start\n");
    for(int i = 0; i < samples; i++){
        printf("%d\n", adc1_get_raw(ADC1_CHANNEL_1));
        vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));
    }
    printf("\\end");
    wait_enter(0);
}

/* PRINT MEASURE TYPE
    Prints the measure type menu and waits for user input.
    The options can be found in constants.c as TEST_SETTINGS.
*/
void print_measure_type(){

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
        for(int i = 0; i < 7; i++) {
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

        float values[temp_sensor[3]];
        HOR_SEP_DW(DISPLAY_SETTINGS[0]);
        uint8_t move_temp = wait_enter(0);
        float freq = (float)TEST_LENGTH_VALUES[temp_sensor[2]]/temp_sensor[3];
        switch(move_temp){
            case 1:
                if(option == 4) {
                    
                    switch(temp_sensor[0]){
                        case 0:
                            ultrasonic();
                            break;
                        case 1:
                            temperature_humidity(temp_sensor[1], freq, temp_sensor[3], values);
                            break;
                        case 2:
                            light();//measure_old(temp_sensor[2], freq, temp_sensor[0], temp_sensor[4]);
                            break;
                    }   
                    //print_summary(values, temp_sensor[4]);
                }else if(option == 5){
                    serial_mode(temp_sensor[0], freq, temp_sensor[4]);
                }else if(option == 6){
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