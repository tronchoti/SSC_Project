#ifndef HC_SR05_H
#define HC_SR05_H

typedef void (*ptr)();
ptr hc_sr05_start(gpio_num_t GPIO_PIN, float freq, int test_length);
ptr hc_sr05_serial_mode(gpio_num_t GPIO_PIN, float freq, int test_length);
#endif

