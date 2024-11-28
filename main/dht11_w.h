#ifndef DHT11_W_H
#define DHT11_W_H

typedef void (*ptr)();
ptr dht11_start(gpio_num_t GPIO_PIN, float freq, int test_length);
ptr dht11_serial_mode(int type, float freq, int samples, gpio_num_t GPIO_PIN);
void serial_print();
#endif
