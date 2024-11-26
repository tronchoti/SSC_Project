
void ultrasonic();
void sensor_read(int GPIO_PIN);
void temperature_humidity(int GPIO_PIN, float freq, int test_length, float *temp, float *hum);
void light();
void measure(int GPIO_PIN, float freq, int measure_type, int test_length);