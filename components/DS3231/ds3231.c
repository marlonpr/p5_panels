#include "ds3231.h"
#include "esp_log.h"

#define DS3231_ADDR 0x68
#define TAG "DS3231"

#define SDA_PIN 21
#define SCL_PIN 22

esp_err_t init_i2c_bus(i2c_master_bus_handle_t *out_bus)
{
    if (!out_bus) return ESP_ERR_INVALID_ARG;

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false // external pull-ups recommended
    };

    return i2c_new_master_bus(&bus_cfg, out_bus);
}


// BCD conversion helpers
static inline uint8_t dec2bcd(uint8_t val) { return ((val / 10) << 4) | (val % 10); }
static inline uint8_t bcd2dec(uint8_t val) { return ((val >> 4) * 10) + (val & 0x0F); }

esp_err_t ds3231_init(i2c_master_bus_handle_t bus, ds3231_dev_t *dev) {
    if (!bus || !dev) return ESP_ERR_INVALID_ARG;
    i2c_device_config_t dev_cfg = {
        .device_address = DS3231_ADDR,
        .scl_speed_hz   = 100000, // 100kHz
    };
    return i2c_master_bus_add_device(bus, &dev_cfg, &dev->i2c_dev);
}

esp_err_t ds3231_set_time(ds3231_dev_t *dev, const ds3231_time_t *time) {
    if (!dev || !time) return ESP_ERR_INVALID_ARG;
    uint8_t data[7];
    data[0] = dec2bcd(time->second);
    data[1] = dec2bcd(time->minute);
    data[2] = dec2bcd(time->hour & 0x3F);  // 24h mode
    data[3] = 1; // Day of week (not used here)
    data[4] = dec2bcd(time->day);
    data[5] = dec2bcd(time->month);
    data[6] = dec2bcd(time->year % 100);
    uint8_t buf[8];
    buf[0] = 0x00; // Start register
    for (int i = 0; i < 7; i++) buf[i+1] = data[i];
    return i2c_master_transmit(dev->i2c_dev, buf, sizeof(buf), -1);
}

esp_err_t ds3231_get_time(ds3231_dev_t *dev, ds3231_time_t *time) {
    if (!dev || !time) return ESP_ERR_INVALID_ARG;
    uint8_t reg = 0x00;
    uint8_t data[7];
    ESP_ERROR_CHECK(i2c_master_transmit(dev->i2c_dev, &reg, 1, -1));
    ESP_ERROR_CHECK(i2c_master_receive(dev->i2c_dev, data, 7, -1));

    time->second = bcd2dec(data[0] & 0x7F);
    time->minute = bcd2dec(data[1] & 0x7F);
    time->hour   = bcd2dec(data[2] & 0x3F);
    time->day    = bcd2dec(data[4] & 0x3F);
    time->month  = bcd2dec(data[5] & 0x1F);
    time->year   = 2000 + bcd2dec(data[6]); // DS3231 base year
    return ESP_OK;
}
