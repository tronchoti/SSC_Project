#include <stdio.h>
#include "constants.h"
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


/* PRINT HEADER IN TEST
    Prints the header of the in_test function.
    It will show the sensor type, the sampling frequency and the last average value.
*/
void print_header_in_test(int type, float freq, int last_avg){
    HOR_SEP_UP(DISPLAY_SETTINGS[0]);
    printf("│ MEASURING%*s │\n", DISPLAY_SETTINGS[0]-11, SENSORS[type]);
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
    printf("│ %-*s %*ld bytes │\n", 15, "Free memory", DISPLAY_SETTINGS[0]-24, esp_get_free_heap_size());
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
    printf("│ %-10s %-*.3f │\n",  "Sampling frequency", DISPLAY_SETTINGS[0]-21, freq);
    HOR_SEP_MID(DISPLAY_SETTINGS[0]);
    printf("│ %-*s ", (DISPLAY_SETTINGS[0]/2)-3, "Raw value");
    printf("│ %-*s %*d %*c │\n", 14, "Last avg value", 4, last_avg, (DISPLAY_SETTINGS[0]/2)-22, ' ');
    HOR_SEP_TABLE_MID(DISPLAY_SETTINGS[0]);
}