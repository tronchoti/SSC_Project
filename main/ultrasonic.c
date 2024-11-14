

//static bool example_pcnt_on_reach(pcnt_unit_handle_t unit, const pcnt_watch_event_data_t *edata, void *user_ctx)
//{
//    BaseType_t high_task_wakeup;
//    QueueHandle_t queue = (QueueHandle_t)user_ctx;
//    // send event data to queue, from this interrupt callback
//    xQueueSendFromISR(queue, &(edata->watch_point_value), &high_task_wakeup);
//    return (high_task_wakeup == pdTRUE);
//}
//
//void measure_ultrasonic(){
//    // measure every second
//    // pin 23 trigger
//    
//    // struct measures ultrasonic_m;
//
//    // ECHO ULTRASONIC
//    // set GPIO pin 1 to INPUT mode;
//    // check for errors and inform in case
//    // esp_err_t dir_1 = gpio_set_direction(GPIO_NUM_1, GPIO_MODE_INPUT);
//    // if (dir_1 == ESP_ERR_INVALID_ARG){
//    //     ESP_LOGE(TAG, "Invalid GPIO pin: input mode");
//    // }
//    
//    ESP_LOGI(TAG, "install pcnt unit");
//    pcnt_unit_config_t unit_config = {
//        .high_limit = 10,
//        .low_limit = -10,
//    };
//
//    pcnt_unit_handle_t pcnt_unit = NULL;
//    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));
//
//
//    ESP_LOGI(TAG, "set glitch filter");
//    pcnt_glitch_filter_config_t filter_config = {
//        .max_glitch_ns = 1000,
//    };
//
//    ESP_LOGI(TAG, "install pcnt channels");
//    pcnt_chan_config_t chan_a_config = {
//        .edge_gpio_num = 10,
//        .level_gpio_num = 10,
//    };
//
//    pcnt_channel_handle_t pcnt_chan_a = NULL;
//    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));
//
//    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
//    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP));
//    
//    
//    // pcnt_unit_enable(pcnt_unit);
//    // TRIGGER OUTPUT
//    // ultrasonic measurement
//    esp_err_t dir_23 = gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);
//    if (dir_23 == ESP_ERR_INVALID_ARG){
//        ESP_LOGE(TAG, "Invalid GPIO pin: output mode");
//    }
//
//
//    ESP_LOGI(TAG, "add watch points and register callbacks");
//    int watch_points[] = {-1,1};
//    for (size_t i = 0; i < 2; i++) {
//        ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, watch_points[i]));
//    }
//    pcnt_event_callbacks_t cbs = {
//        .on_reach = example_pcnt_on_reach,
//    };
//    QueueHandle_t queue = xQueueCreate(10, sizeof(int));
//    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(pcnt_unit, &cbs, queue));
//
//    ESP_LOGI(TAG, "enable pcnt unit");
//    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
//    ESP_LOGI(TAG, "clear pcnt unit");
//    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
//    ESP_LOGI(TAG, "start pcnt unit");
//    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
//    
//#if CONFIG_EXAMPLE_WAKE_UP_LIGHT_SLEEP
//    // EC11 channel output high level in normal state, so we set "low level" to wake up the chip
//    ESP_ERROR_CHECK(gpio_wakeup_enable(EXAMPLE_EC11_GPIO_A, GPIO_INTR_LOW_LEVEL));
//    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup());
//    ESP_ERROR_CHECK(esp_light_sleep_start());
//#endif
//
//    // Report counter value
//    int pulse_count = 0;
//    int event_count = 0;
//
//    while (1) {
//        // vTaskDelay(pdMS_TO_TICKS(500));
//        // ESP_LOGI(TAG, "Pulse count: %d", pulse_count);
//
//        gpio_set_level(GPIO_NUM_23, 1);
//        ets_delay_us(10);
//        gpio_set_level(GPIO_NUM_23, 0);
//        if (xQueueReceive(queue, &event_count, pdMS_TO_TICKS(2000))) {
//            ESP_LOGI(TAG, "Watch point event, count: %d", event_count);
//        } else {
//            ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
//            ESP_LOGI(TAG, "Pulse count: %d", pulse_count);
//        }
//    }
//
//    // gpio_set_level(GPIO_NUM_23, 0);
//    // while(1){
//        // // send trigger signal
//        
//        // gpio_set_level(GPIO_NUM_23, 1);
//        // ets_delay_us(2);
//        // gpio_set_level(GPIO_NUM_23, 0);
//        // // 
//        // pcnt_unit_start(pcnt_unit);
//        // printf("test");
//        // vTaskDelay(pdMS_TO_TICKS(5000));
//        // pcnt_unit_stop(pcnt_unit);
//
//        // int pulse_count = 15;
//        // pcnt_unit_get_count(pcnt_unit, &pulse_count);
//        // printf("%d\n", pulse_count);
//        // vTaskDelay(pdMS_TO_TICKS(1000));
//    // }
//
//
//
//}
//