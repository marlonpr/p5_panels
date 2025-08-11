#ifndef LED_PANEL_H
#define LED_PANEL_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>
#include "inttypes.h"
#include <stdio.h>
#include "ds18b20.h"




// === API Functions ===
void init_pins(void);
void set_pixel(int x, int y, int r, int g, int b);

void refresh_display_task(void *arg);

void swap_buffers();


void init_oe_pwm(void);
void set_global_brightness(uint8_t percent);


extern uint8_t smiley[];

extern ds18b20_t sensor;

void clear_back_buffer(void);

unsigned long millis();

void show_time(int x,int y,int r, int g, int b);
void show_date(int y, int r, int g, int b);


void draw_text(const char *str, int x, int y, int r, int g, int b);
void scroll_text(const char *text, int y, int r, int g, int b, int speed_ms);
void draw_bitmap(const uint8_t *bmp, int w, int h, int x0, int y0, int r, int g, int b);
void test_pixel_by_pixel_fill();

void show_temperature(int x, int y, int r, int g, int b);

#endif // LED_PANEL_H