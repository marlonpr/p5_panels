#include "led_panel.h"

void drawing_task(void *arg)
{
	 while (1) {


		vTaskDelay(pdMS_TO_TICKS(2000));
		
		set_pixel(70,5,0,0,1);
		set_pixel(10, 10, 1, 0, 0);  // Red at (10,10)
		set_pixel(80, 20, 0, 1, 0);  // Green at (80,20)


    }
}




void app_main(void)
{
	init_pins();


	
	xTaskCreatePinnedToCore(refresh_display_task, "Refresh", 2048, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(drawing_task,         "Draw",    4096, NULL, 0, NULL, 1);
	//xTaskCreatePinnedToCore(background_task,      "BG",      1024, NULL, 1, NULL, 1);


   while (1) {
		vTaskDelay(pdMS_TO_TICKS(2));
		

    }
}
