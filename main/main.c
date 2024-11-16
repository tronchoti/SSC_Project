#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "rom/ets_sys.h"
#include <string.h>
#include "driver/pulse_cnt.h"
#include <math.h>

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

struct measures{
    uint16_t raw_value;
    uint16_t average;
    uint16_t average_final; 
    uint16_t last_values[10];
};

static uint16_t DISPLAY_SETTINGS[] = {80, 24}; // console_width, console_height

static const char *SENSORS[] = {
    "Ultrasonic ranging",
    "Temperature sensor",
    "Light sensor"
};

static const char *MENU_STRING[] = {
    "1. Start measuring ",\
    "2. Settings        ",\
    "3. Display settings",\
    "4. Exit"
};

static const char *SETTINGS_1[] = {
    "1. Update frequency",\
    "2. Average buffer size",\
    "3. Exit"
};

static const char *SETTINGS_2[] = {
    "1. Console width",\
    "2. Console height",\
    "3. Exit"
};

static const char *TEST_SETTINGS[] = {
    "Sensor type", \
    "GPIO pin", \
    "_____________", \
    "Test duration", \
    "Number of samples", \
    "Continue", \
    "Exit"
};

static const char *MEASURE_TYPES[] = {
    "1. Ultrasonic ranging module",\
    "2. Temperature sensor", \
    "3. Ligth sensor",\
    "4. Exit"
};

static const char *TEST_LENGTH[] = {
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
static int TEST_LENGTH_VALUES[] = {
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

void clear_screen(){
    printf("\e[1;1H\e[2J");
}

void print_sensor_value(int data){
    printf("┌──────────────────────────────────────────┐\n");
    printf("│ Sensor value: %7d                        │\n", data);
    printf("├──────────────────────────────────────────┤\n");
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

void measure(int GPIO_PIN, float freq, int measure_type, int test_length){

    // set GREEN LED STATUS ON
    gpio_set_level(11, 0);
    gpio_set_level(10, 1);

    int *avg_values = calloc(test_length, sizeof(int));    // AVERAGE VALUES ARRAY
    
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_12); //prints info
    clear_screen();
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
            last_vals[i] = adc1_get_raw(ADC1_CHANNEL_1);
            if(last_avg != 0){
                delta_num = last_vals[i] - last_avg;
                delta_perc = (delta_num/(float)last_avg)*100;
            }    
            printf("│   Raw val(%*d%c): %*d", 3, counter, 's', 4, last_vals[i]); 
            printf("%*c", DISPLAY_SETTINGS[0]-(21+20), ' ');
            printf("\u0394: %*d/%6.2f%%   │\n", 5, delta_num, delta_perc);
            vTaskDelay(pdMS_TO_TICKS((int)(freq*1000)));
            //if(wait_enter(freq) == 1) return;
            counter++;
        }
        last_avg = 0;
        for(int i = 0; i < 16; i++){
            last_avg += last_vals[i];
        }
        last_avg /= 16;

        HOR_SEP_TABLE_DW(DISPLAY_SETTINGS[0]);

        
        clear_screen();
        //vTaskDelay(pdMS_TO_TICKS(settings[0]));
        //if (wait_enter(freq)==1) break;
        //clear_screen();
    }

    // set RED LED STATUS ON -> end of measurement
    gpio_set_level(GPIO_NUM_11, 1);
    gpio_set_level(GPIO_NUM_10, 0);

    return;
}

void measure_old(int GPIO_PIN, int freq){

    // set GREEN LED STATUS ON
    gpio_set_level(GPIO_NUM_11, 0);
    gpio_set_level(GPIO_NUM_10, 1);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_1, ADC_ATTEN_DB_12); //prints info
    clear_screen();
    
    struct measures m1;
    uint16_t counter = 0;
    while(1){
        m1.raw_value = adc1_get_raw(ADC1_CHANNEL_1);
        m1.last_values[counter++] = m1.raw_value;
        m1.average += m1.raw_value;
        if(counter == 10){
            m1.average_final = m1.average/10;
            m1.average = 0;
            counter = 0;
        }

        HOR_SEP_UP(DISPLAY_SETTINGS[0]);
        printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "MEASURING");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s %*d ", 15, "Raw value", (int)((float)(DISPLAY_SETTINGS[0]-30)*0.5)-4, m1.raw_value);
        printf("│ %-*s %*d │\n", 15, "Average value", (int)((float)(DISPLAY_SETTINGS[0]-30)*0.5)-3, m1.average_final);
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s │\n", DISPLAY_SETTINGS[0]-2, "-> Exit");
        HOR_SEP_MID(DISPLAY_SETTINGS[0]);
        printf("│ %-*s %*ld bytes │\n", 15, "Free memory", DISPLAY_SETTINGS[0]-24, esp_get_free_heap_size());
        HOR_SEP_DW(DISPLAY_SETTINGS[0]);    
        //vTaskDelay(pdMS_TO_TICKS(settings[0]));
        if (wait_enter(freq)==1) break;
        clear_screen();
    }

    // set RED LED STATUS ON -> end of measurement
    gpio_set_level(GPIO_NUM_11, 1);
    gpio_set_level(GPIO_NUM_10, 0);

    return;
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
                    measure(temp_sensor[option], freq, temp_sensor[0], temp_sensor[4]);   // start measuring
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

    // set GPIO pin 11 to OUTPUT mode;
    // check for errors and inform in case
    esp_err_t dir_11 = gpio_set_direction(GPIO_NUM_11, GPIO_MODE_OUTPUT);
    if (dir_11 == ESP_ERR_INVALID_ARG){
        ESP_LOGE(TAG, "Invalid GPIO pin: output mode");
        printf("Invalid GPIO pin: output mode\n");
    }
    
    // set GPIO pin 10 to INPUT mode;
    // check for errors and inform in case
    esp_err_t dir_10 = gpio_set_direction(GPIO_NUM_10, GPIO_MODE_INPUT);
    if (dir_10 == ESP_ERR_INVALID_ARG){
        ESP_LOGE(TAG, "Invalid GPIO pin: output mode");
    }
    
    //ESP_LOGI(TAG, "Raw value %d", raw_value);
    // printf("%d\n", raw_value);
    while(1){
        int raw_value = adc1_get_raw(ADC1_CHANNEL_1);
        
        
        if (gpio_get_level(GPIO_NUM_10) == 1){
            // set GPIO pin 10 to level high;
            // check for errors and inform in case
            //ESP_LOGI(TAG, "Output pin 10: on");
            //esp_err_t pin_11_out = 
            print_sensor_value(1);
            gpio_set_level(GPIO_NUM_11, 1);
            // if (pin_11_out == ESP_ERR_INVALID_ARG){
            //     ESP_LOGE(TAG, "Invalid GPIO pin: level");
            // }
        }else{
            gpio_set_level(GPIO_NUM_11, 0);
            //ESP_LOGI(TAG, "Output pin 11: off");
            print_sensor_value(0);
        }
        print_sensor_value(raw_value);
        vTaskDelay(pdMS_TO_TICKS(1000));
        clear_screen();
    }
    

    
}