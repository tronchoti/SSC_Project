#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "dht.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

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

/*
*/
const char *SETTINGS_1[] = {
    "1. Update frequency",\
    "2. Average buffer size",\
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

