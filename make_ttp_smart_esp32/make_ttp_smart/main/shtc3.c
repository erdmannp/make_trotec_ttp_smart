
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"

#include "shtc3.h"


const static char *TAG = "shtc3";
const static uint16_t SHTC3_ADDR = 0x70;
const static uint32_t SHTC3_FREQ = 10000;
const static gpio_num_t I2C_MASTER_SDA_IO = 4;
const static gpio_num_t I2C_MASTER_SCL_IO = 3;



static const shtc3_t shtc3_empty_data = {
    .temperature = 0,
    .humidity = 0,
    .error = ESP_ERR_NOT_FINISHED,
};
static shtc3_t shtc3_data;


esp_err_t shtc3_init(void) {
    esp_err_t ret;

    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = SHTC3_FREQ,
    };
    ret = i2c_param_config(I2C_NUM_0, &config);
    ret += i2c_driver_install(I2C_NUM_0, config.mode, 0, 0, 0);


    return ret;
}


static void shtc3_wakeup(void) {
    uint8_t wakeup_command[] = {0x35, 0x17};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHTC3_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, wakeup_command, sizeof(wakeup_command), true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

shtc3_t shtc3_static_read(void) {
    return shtc3_data;
}

static void shtc3_i2c_read(void) {
    uint8_t command[2] = {0x7C, 0xA2}; // normal mode
    uint8_t data[6];
    shtc3_data = shtc3_empty_data;
    
    shtc3_wakeup();


    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHTC3_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, command, sizeof(command), true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHTC3_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, sizeof(data), I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        uint16_t temp_raw = (data[0] << 8) | data[1];
        uint16_t hum_raw = (data[3] << 8) | data[4];

        shtc3_data.temperature = -45 + 175 * ((float)temp_raw / 65535.0);
        shtc3_data.humidity = 100 * ((float)hum_raw / 65535.0);
        shtc3_data.error = ESP_OK;
    } else {
        shtc3_data.temperature = -300;
        shtc3_data.humidity = -300;
        shtc3_data.error = ret;
    }
}

void shtc3_task_f(void *arg) {
    const char *taskTag = "SHTC3Task";
    ESP_LOGI(taskTag, "SHTC3Task started");
    esp_err_t ret = shtc3_init();

    if (ret != ESP_OK) {
        ESP_LOGE(taskTag, "Error initializing SHTC3: %s", esp_err_to_name(ret));
        vTaskDelete(NULL);
       }

    while (1) {
        shtc3_i2c_read();
        if (shtc3_data.error != ESP_OK) {
            ESP_LOGE(taskTag, "Error reading SHTC3: %s", esp_err_to_name(shtc3_data.error));
        }
        vTaskDelay(200);
    }
}
