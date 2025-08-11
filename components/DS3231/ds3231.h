#ifndef DS3231_H
#define DS3231_H

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t year;  // e.g., 2025
    uint8_t  month; // 1-12
    uint8_t  day;   // 1-31
    uint8_t  hour;  // 0-23
    uint8_t  minute;// 0-59
    uint8_t  second;// 0-59
} ds3231_time_t;

typedef struct ds3231_dev {
    i2c_master_dev_handle_t i2c_dev;
} ds3231_dev_t;

esp_err_t ds3231_init(i2c_master_bus_handle_t bus, ds3231_dev_t *dev);
esp_err_t ds3231_set_time(ds3231_dev_t *dev, const ds3231_time_t *time);
esp_err_t ds3231_get_time(ds3231_dev_t *dev, ds3231_time_t *time);

#ifdef __cplusplus
}
#endif

#endif // DS3231_H
