#include "led_panel.h"
#include "ds3231.h"
#include "esp_log.h"



void drawing_task(void *arg)
{


	while(1)
	{

		show_time(10,10,1,1,1);
		show_date(10,1,1,1);
		show_temperature( 10, 10, 0, 1, 0);
	}
}




void app_main(void)
{
	init_pins();

	ds18b20_init(&sensor, GPIO_NUM_26); // Use GPIO4 with 4.7kΩ pull-up resistor

/*
    ds3231_dev_t rtc;
    ESP_ERROR_CHECK(init_ds3231(&rtc));

    // Set a time for testing
    ds3231_time_t set_time = {2025, 8, 11, 15, 0, 0};
    ESP_ERROR_CHECK(ds3231_set_time(&rtc, &set_time));
*/

	init_oe_pwm();
	set_global_brightness(255); //0 - 255
	
	xTaskCreatePinnedToCore(refresh_display_task, "Refresh", 2048, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(drawing_task,         "Draw",    4096, NULL, 0, NULL, 1);
	//xTaskCreatePinnedToCore(background_task,      "BG",      1024, NULL, 1, NULL, 1);

    
    


    while (1) {
/*
        ds3231_time_t now;
        ESP_ERROR_CHECK(ds3231_get_time(&rtc, &now));
        ESP_LOGI("RTC", "%04d-%02d-%02d %02d:%02d:%02d",
                 now.year, now.month, now.day,
                 now.hour, now.minute, now.second);
        
*/
		vTaskDelay(pdMS_TO_TICKS(1));
    }
}


