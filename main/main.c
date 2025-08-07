#include "led_panel.h"

void drawing_task(void *arg)
{
	 while (1) {

	clear_back_buffer();    
	test_pixel_by_pixel_fill();  // Final test
	//swap_buffers();  // Display what was drawn
    vTaskDelay(pdMS_TO_TICKS(1000));



	clear_back_buffer();		
	draw_bitmap(smiley, 8, 8, 60, 12,0,1,0);  // Draw at (60,12)
	// Once all drawing is done
	swap_buffers();	
	vTaskDelay(pdMS_TO_TICKS(2000));


	clear_back_buffer();	
	draw_text("HELLO", 20, 10, 1, 1, 1);  // White text at top-left
	swap_buffers();	
	vTaskDelay(pdMS_TO_TICKS(2000));




	clear_back_buffer();
	draw_text("WORLD", 70, 10, 1, 0, 1);  // White text at top-left	
	// Once all drawing is done
	swap_buffers();	
	vTaskDelay(pdMS_TO_TICKS(1000));




	clear_back_buffer();
	scroll_text("ESP32 LED DEMO  ", 10, 1, 0, 0, 40);  // red, y=10, speed=40ms per frame
	// Once all drawing is done
	swap_buffers();	
	vTaskDelay(pdMS_TO_TICKS(500));




    }
}




void app_main(void)
{
	init_pins();

	init_oe_pwm();
	set_global_brightness(255); //0 - 255

	
	xTaskCreatePinnedToCore(refresh_display_task, "Refresh", 2048, NULL, 1, NULL, 0);
	xTaskCreatePinnedToCore(drawing_task,         "Draw",    4096, NULL, 0, NULL, 1);
	//xTaskCreatePinnedToCore(background_task,      "BG",      1024, NULL, 1, NULL, 1);


   while (1) {
		vTaskDelay(pdMS_TO_TICKS(2));
		

    }
}
