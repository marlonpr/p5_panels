#include "led_panel.h"
#include "driver/ledc.h"
#include "font5x7.h"
#include <math.h>
#include "esp_err.h"


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
                     (1ULL<<PIN_CLK)| (1ULL<<PIN_LAT);

    gpio_config_t io_conf = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = 0,
        .pull_up_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    //gpio_set_level(PIN_OE, 1);
    gpio_set_level(PIN_LAT, 0);
    gpio_set_level(PIN_CLK, 0);
}



//-------------------------------------------//-------------------------------------------


// Choose a high-speed channel/timer so duty updates are as fast as possible
#define OE_SPEED_MODE    LEDC_HIGH_SPEED_MODE
#define OE_TIMER_NUM     LEDC_TIMER_0
#define OE_CHANNEL       LEDC_CHANNEL_0
#define OE_DUTY_RES      LEDC_TIMER_8_BIT    // 256 steps
#define OE_FREQUENCY_HZ  1000000             // 1 MHz PWM freq

void init_oe_pwm(void)
{
    // Configure a high-speed LEDC timer for OE pin
    ledc_timer_config_t timer_conf = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_6_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 1000000,        // 1 MHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    // Map OE pin into that PWM channel
    ledc_channel_config_t chan_conf = {
        .speed_mode     = LEDC_HIGH_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = PIN_OE,
        .duty           = 0,                // start OFF
        .hpoint         = 0,
		.flags = {
            .output_invert = 1   // <-- invert the PWM signal
        }
    };
    ESP_ERROR_CHECK(ledc_channel_config(&chan_conf));
}




// Global brightness in percent (0–100)
static volatile uint8_t global_brightness = 255;

// Call this once after your LEDC timer/channel have been initialized
// to set initial duty according to global_brightness.
static void update_oe_duty(void)
{
    const uint32_t max_duty = (1 << OE_DUTY_RES) - 1;         // 255
    uint32_t duty = (max_duty * global_brightness) / 255;     // scale 0–255
    // Apply it immediately
    ESP_ERROR_CHECK( ledc_set_duty(OE_SPEED_MODE, OE_CHANNEL, duty) );
    ESP_ERROR_CHECK( ledc_update_duty(OE_SPEED_MODE, OE_CHANNEL) );
}

// Call this from wherever you want to change brightness (e.g. CLI, button handler)
void set_global_brightness(uint8_t level)
{
    if (level > 255) {
        level = 255;
    }
    global_brightness = level;
    update_oe_duty();
}


//---------------------------------------------//-------------------------------------------






static uint8_t framebufferA[HEIGHT][WIDTH];
static uint8_t framebufferB[HEIGHT][WIDTH];

static volatile uint8_t (*front_buffer)[WIDTH] = framebufferA;
static volatile uint8_t (*back_buffer)[WIDTH] = framebufferB;

void set_pixel(int x, int y, int r, int g, int b) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;

    uint8_t color = (r ? 0x01 : 0) | (g ? 0x02 : 0) | (b ? 0x04 : 0);
    back_buffer[y][x] = color;
}

void swap_buffers(void) {
    uint8_t (*temp)[WIDTH] = (uint8_t (*)[WIDTH])front_buffer;
    front_buffer = back_buffer;
    back_buffer = temp;
}

void clear_back_buffer(void) {
    memset((void *)back_buffer, 0, WIDTH * HEIGHT);
}


void refresh_display_task(void *arg) {
    
	clear_back_buffer();
	int r1 = 0;
	int g1 = 0;
	int b1 = 0;
	
	int r2 = 0;
	int g2 = 0;
	int b2 = 0;

	while (true) {
        for (int row = 0; row < 8; row++) {



            //gpio_set_level(PIN_OE, 1);
			// Duty = max → PWM is always HIGH → OE stays HIGH → panel off
			ledc_set_duty(OE_SPEED_MODE, OE_CHANNEL, 0);
			ledc_update_duty(OE_SPEED_MODE, OE_CHANNEL);



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
			        uint8_t pix = front_buffer[y1][framebuffer_x];
			        r1 = (pix >> 0) & 1;
			        g1 = (pix >> 1) & 1;
			        b1 = (pix >> 2) & 1;
			    }
			
			    if (y2 < HEIGHT && framebuffer_x < WIDTH) {
			        uint8_t pix = front_buffer[y2][framebuffer_x];
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

            //gpio_set_level(PIN_OE, 0);
			// Duty = 0 → PWM is always LOW → OE stays LOW → panel on
			//ledc_set_duty(OE_SPEED_MODE, OE_CHANNEL, (1 << OE_DUTY_RES) - 1);
			//ledc_update_duty(OE_SPEED_MODE, OE_CHANNEL);
			update_oe_duty();

            esp_rom_delay_us(120);
        }
    }
}

void draw_char(char c, int x, int y, int r, int g, int b) {
    if (c < 32 || c > 126) return;  // Ignore unsupported chars

    const uint8_t* glyph = font5x7[c - 32];

    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                set_pixel(x + col, y + row, r, g, b);
            }
        }
    }
}


void draw_text(const char* str, int x, int y, int r, int g, int b) {
    while (*str) {
        draw_char(*str++, x, y, r, g, b);
        x += 6;  // 5 pixels wide + 1 space between chars
    }
}

void scroll_text(const char *text, int y, int r, int g, int b, int speed_ms) {
    int len = strlen(text);
    int text_width = len * 6;
    if (text_width <= 0) return;

    // Create an offscreen pixel buffer for the full text
    uint8_t text_buffer[7][text_width];  // 7 rows tall

    // Clear the text buffer
    memset(text_buffer, 0, sizeof(text_buffer));

    // Render text into the text_buffer
    for (int i = 0; i < len; i++) {
        char c = text[i];
        if (c < 32 || c > 126) c = '?';
        const uint8_t *glyph = font5x7[c - 32];
        for (int col = 0; col < 5; col++) {
            uint8_t bits = glyph[col];
            for (int row = 0; row < 7; row++) {
                if (bits & (1 << row)) {
                    int x = i * 6 + col;
                    text_buffer[row][x] = (r << 0) | (g << 1) | (b << 2);
                }
            }
        }
    }

    // Scroll loop
    for (int scroll_x = 0; scroll_x < text_width + WIDTH; scroll_x++) {
        clear_back_buffer();

        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < WIDTH; col++) {
                int src_x = col + scroll_x - WIDTH;
                if (src_x >= 0 && src_x < text_width) {
                    uint8_t pix = text_buffer[row][src_x];
                    int px = col;
                    int py = y + row;
                    int pr = (pix >> 0) & 1;
                    int pg = (pix >> 1) & 1;
                    int pb = (pix >> 2) & 1;
                    set_pixel(px, py, pr, pg, pb);
                }
            }
        }

        swap_buffers();
        vTaskDelay(pdMS_TO_TICKS(speed_ms));  // Delay between frames
    }
}

void draw_bitmap(const uint8_t *bmp, int w, int h, int x0, int y0, int r, int g, int b) {
    for (int y = 0; y < h; y++) {
        int dst_y = y0 + y;
        if (dst_y < 0 || dst_y >= HEIGHT) continue;

        for (int x = 0; x < w; x++) {
            int dst_x = x0 + x;
            if (dst_x < 0 || dst_x >= WIDTH) continue;

            uint8_t pix = bmp[y * w + x];

            if (pix != 0) {
                set_pixel(dst_x, dst_y, r, g, b);  // Use provided color
            }
        }
    }
}



uint8_t smiley[] = {
    0,0,1,1,1,1,0,0,
    0,1,0,0,0,0,1,0,
    1,0,1,0,0,1,0,1,
    1,0,0,0,0,0,0,1,
    1,0,1,0,0,1,0,1,
    1,0,0,1,1,0,0,1,
    0,1,0,0,0,0,1,0,
    0,0,1,1,1,1,0,0
};


void test_pixel_by_pixel_fill()
{
    clear_back_buffer();     // Clear draw framebuffer
    swap_buffers();          // Show black screen
    clear_back_buffer();     // Clear the new draw_fb too
    vTaskDelay(pdMS_TO_TICKS(500));

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            set_pixel(x, y, 1, 1, 1);  // Add this pixel to the framebuffer

            // Show this pixel plus all previous ones
            swap_buffers();           // Show accumulated pixels
            vTaskDelay(pdMS_TO_TICKS(5));

            // Copy framebuffer state back to draw_fb (since swap flips them)
            memcpy((void*)back_buffer, (void*)front_buffer, sizeof(framebufferA));
        }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
}
