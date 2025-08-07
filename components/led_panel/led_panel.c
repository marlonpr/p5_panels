#include "led_panel.h"

#define PIN_R1   GPIO_NUM_2
#define PIN_G1   GPIO_NUM_4
#define PIN_B1   GPIO_NUM_5
#define PIN_R2   GPIO_NUM_18
#define PIN_G2   GPIO_NUM_19
#define PIN_B2   GPIO_NUM_21
#define PIN_CLK  GPIO_NUM_13
#define PIN_LAT  GPIO_NUM_12
#define PIN_OE   GPIO_NUM_14
#define PIN_A    GPIO_NUM_15
#define PIN_B    GPIO_NUM_22
#define PIN_C    GPIO_NUM_23

#define NUM_PANELS 2
#define WIDTH 64 * NUM_PANELS
#define HEIGHT 32

void init_pins(void)
{
    uint64_t mask =  (1ULL<<PIN_R1) | (1ULL<<PIN_G1) | (1ULL<<PIN_B1) |
                     (1ULL<<PIN_R2) | (1ULL<<PIN_G2) | (1ULL<<PIN_B2) |
                     (1ULL<<PIN_A)  | (1ULL<<PIN_B)  | (1ULL<<PIN_C)  |
                     (1ULL<<PIN_CLK)| (1ULL<<PIN_LAT)| (1ULL<<PIN_OE);

    gpio_config_t io_conf = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(PIN_OE, 1);
    gpio_set_level(PIN_LAT, 0);
    gpio_set_level(PIN_CLK, 0);
}


// Framebuffer: 3-bit packed RGB (bit0=R, bit1=G, bit2=B)
uint8_t framebuffer[HEIGHT][WIDTH] = {0};

void set_pixel(int x, int y, int r, int g, int b) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;

    uint8_t color = (r ? 0x01 : 0) | (g ? 0x02 : 0) | (b ? 0x04 : 0);
    framebuffer[y][x] = color;
}

void refresh_display_task(void *arg) {
    
	memset(framebuffer, 0, sizeof(framebuffer));
	int r1 = 0;
	int g1 = 0;
	int b1 = 0;
	
	int r2 = 0;
	int g2 = 0;
	int b2 = 0;

	while (true) {
        for (int row = 0; row < 8; row++) {
            gpio_set_level(PIN_OE, 1);
            esp_rom_delay_us(5);

            gpio_set_level(PIN_A, row & 0x01);
            gpio_set_level(PIN_B, (row >> 1) & 0x01);
            gpio_set_level(PIN_C, (row >> 2) & 0x01);

			for (int col = 0; col < 256; col++) {
			    r1 = g1 = b1 = 0;
			    r2 = g2 = b2 = 0;
			
			    int panel = col / 64;       // 0–3
			    int local_x = col % 64;     // Always 0–63
			    int framebuffer_x = (panel >= 2) ? local_x + 64 : local_x; // panels 2 and 3 are right half
			
			    int y1 = (panel & 1) ? row : row + 8;     // Even panel: lower half, Odd: upper half
			    int y2 = y1 + 16;
			
			    if (y1 < HEIGHT && framebuffer_x < WIDTH) {
			        uint8_t pix = framebuffer[y1][framebuffer_x];
			        r1 = (pix >> 0) & 1;
			        g1 = (pix >> 1) & 1;
			        b1 = (pix >> 2) & 1;
			    }
			
			    if (y2 < HEIGHT && framebuffer_x < WIDTH) {
			        uint8_t pix = framebuffer[y2][framebuffer_x];
			        r2 = (pix >> 0) & 1;
			        g2 = (pix >> 1) & 1;
			        b2 = (pix >> 2) & 1;
			    }
			
			    gpio_set_level(PIN_R1, r1);
			    gpio_set_level(PIN_G1, g1);
			    gpio_set_level(PIN_B1, b1);
			    gpio_set_level(PIN_R2, r2);
			    gpio_set_level(PIN_G2, g2);
			    gpio_set_level(PIN_B2, b2);
			
			    gpio_set_level(PIN_CLK, 1);
			    gpio_set_level(PIN_CLK, 0);
			}


            gpio_set_level(PIN_LAT, 1);
            gpio_set_level(PIN_LAT, 0);
            gpio_set_level(PIN_OE, 0);
            esp_rom_delay_us(120);
        }
    }
}

