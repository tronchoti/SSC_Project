#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "dht.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

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

#define CLEAR_SCREEN printf("\e[1;1H\e[2J");


/* WAIT ENTER
    Waits for user input.
    If wait_time is 0, it waits for timeout/100 iterations.
    Otherwise, it waits for wait_time/100 iterations.
*/
int wait_enter(float wait_time){
    fpurge(stdin); //clears any junk in stdin
    char bufp[10];
    int rep = wait_time/100;
    
    while(1){
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
        
        if(rep != 0 && --rep == 0){
            return 0;
        }
    }
    return -1;
}

/* SENSORS
    Array with name of the sensors that are programmed
    to be used in the program.
*/
const char *SENSORS[] = {
    "Ultrasonic ranging",
    "DHT11",
    "Light sensor"
};

/*  MENU STRINGS
    Array with the options of the main menu.
*/
const char *MENU_STRING[] = {
    "1. Start measuring ",\
    "2. Display settings",\
    "3. Exit"
};

/*  SETTINGS 2
    Array with the options of the second settings menu.
*/
const char *SETTINGS_2[] = {
    "1. Console width",\
    "2. Console height",\
    "3. Exit"
};

/*  TEST SETTINGS
    Array with the options of the test settings menu.
*/
const char *TEST_SETTINGS[] = {
    "Sensor type", \
    "GPIO pin", \
    "Test duration", \
    "Number of samples", \
    "Start", \
    "Serial mode", \
    "Print serial last test", \
    "Exit"
};

/*  TEST LENGTH
    Array with the options of the test length.
    Goes from 10s to 24h, allowing the user to choose
    the duration of the test.
*/
const char *TEST_LENGTH[] = {
    "10s",
    "30s",
    "60s",
    "120s",
    "5min",
    "10min",
    "20min",
    "30min",
    "1h",
    "2h",
    "5h",
    "10h",
    "24h",
    "2d",
    "5d",
    "10d"
};

/*  TEST LENGTH VALUES
    Array with the values of the test length in seconds.
    Used to convert the user's choice to seconds.
*/
const int TEST_LENGTH_VALUES[] = {
    10,
    30,
    60,
    120,
    300,
    600,
    1200,
    1800,
    3600,
    7200,
    18000,
    36000,
    86400,
    172800,
    345600,
    691200
};

uint16_t DISPLAY_SETTINGS[] = {80, 24}; // console_width, console_height

