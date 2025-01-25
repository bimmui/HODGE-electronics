/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define TAG "BMP581_INIT"

#define I2C_MASTER_PORT                 I2C_NUM_0
#define I2C_MASTER_SCL_IO               22
#define I2C_MASTER_SDA_IO               21
#define BMP581_I2C_ADDRESS_DEFAULT      0x46
#define BMP581_I2C_ADDRESS_SECONDARY    0x47
#define MASTER_TRANSMIT_TIMEOUT         (500)
#define POWER_UP_DELAY_MS               20
#define SOFT_RESET_DELAY_MS             5
/* BMP581 Registers */
#define ODR_CONFIG_REG                  0x37
#define OSR_CONFIG_REG                  0x36
#define CMD_REG                         0x7E
#define STATUS_REG                      0x28
#define STATUS_NVM_RDY_BIT              0x02
#define STATUS_NVM_RDY_SHIFT            1
#define STATUS_NVM_ERR_BIT              0x04
#define STATUS_NVM_ERR_SHIFT            2
#define CHIP_ID_REG                     0x01
#define INT_STATUS_REG                  0x27
#define INT_STATUS_POR_BIT              0x10
#define PRESS_DATA_MSB_REG              0x22
#define PRESS_DATA_LSB_REG              0x21
#define PRESS_DATA_XLSB_REG             0x20
#define TEMP_DATA_MSB_REG               0x1F
#define TEMP_DATA_LSB_REG               0x1E
#define TEMP_DATA_XLSB_REG              0x1D

/* BMP581 Commands*/
#define SOFT_RESET_CMD                  0xB6
#define STANDBY_MODE_MSB                0x0B
#define STANDBY_MODE_LSB                0x00

i2c_master_bus_handle_t bus_handle =    NULL;
i2c_master_dev_handle_t device_handle = NULL;

/* Function contracts */
void i2c_controller_init(void);
void i2c_peripheral_init(void);
static void sensor_check(void);

void i2c_controller_init(void)
{
    /* I was able to get the correct ordering from 
    https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/i2c.html#_CPPv4N23i2c_master_bus_config_t8i2c_portE*/
    i2c_master_bus_config_t i2c_controller_config = {};
    i2c_controller_config.i2c_port = I2C_MASTER_PORT;
    i2c_controller_config.sda_io_num = static_cast<gpio_num_t>(I2C_MASTER_SDA_IO);
    i2c_controller_config.scl_io_num = static_cast<gpio_num_t>(I2C_MASTER_SCL_IO);
    i2c_controller_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_controller_config.glitch_ignore_cnt = 7;
    i2c_controller_config.flags.enable_internal_pullup = true;

    /* Create new I2C bus in master mode */
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_controller_config, &bus_handle));

    /* Successfully init bus */
    ESP_LOGI(TAG, "I2C controller bus initialized.");
}

void i2c_peripheral_init(void){
    
    /* Adding this delay to ensure sensor has stabalized after power up */
    vTaskDelay(pdMS_TO_TICKS(POWER_UP_DELAY_MS));

    /* Create a handle for the specific slave device (0x46) Baro */
    i2c_device_config_t i2c_peripheral_cfg = {};
    i2c_peripheral_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    i2c_peripheral_cfg.device_address = BMP581_I2C_ADDRESS_DEFAULT;
    i2c_peripheral_cfg.scl_speed_hz = 100000; // 100 kHz for normal mode

    /* Place the device on the the bus */
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &i2c_peripheral_cfg,
                                              &device_handle));
    ESP_LOGI(TAG, "BMP581 sensor added to I2C bus.");
}

static void i2c_write(i2c_master_dev_handle_t sensor,
                      uint8_t const *data_buf, const uint8_t data_len){

    ESP_ERROR_CHECK(i2c_master_transmit(sensor, data_buf, data_len,
                    MASTER_TRANSMIT_TIMEOUT));
}

static void i2c_read(i2c_master_dev_handle_t sensor,
                     const uint8_t reg_start_addr, uint8_t *rx,
                     uint8_t rx_size){
        
    const uint8_t tx[] = {reg_start_addr};
    // In order to read, we first have to write?
    // ESP_ERROR_CHECK(i2c_master_transmit(sensor, tx, sizeof(tx), MASTER_TRANSMIT_TIMEOUT));
    // ESP_ERROR_CHECK(i2c_master_transmit_receive(sensor, tx, sizeof(tx), rx, rx_size, MASTER_TRANSMIT_TIMEOUT));
}
static void reset_bmp581(void){

    /* We organize the data in a container that we will write to reg */
    uint8_t reset_data[2];
    reset_data[0] = CMD_REG;
    reset_data[1] = SOFT_RESET_CMD;
    
    /* We perform the soft reset */
    ESP_ERROR_CHECK(i2c_master_transmit(device_handle, reset_data,
                                    sizeof(reset_data), MASTER_TRANSMIT_TIMEOUT));

    /* Lastly, let's enforce a 2 ms timeout which is the time needed for the
        reboot to happen but we will add a buffer */
    vTaskDelay(pdMS_TO_TICKS(SOFT_RESET_DELAY_MS));
    ESP_LOGI(TAG, "Soft reset successfully executed.");

}

static void sensor_check(void){
        
    /* We perform a dummy read to initalize the sensor's interface */
    uint8_t dummy_reg = 0x00;  // A valid register address
    uint8_t dummy_data = 0;

    esp_err_t ret = i2c_master_transmit(device_handle, &dummy_reg, 1, MASTER_TRANSMIT_TIMEOUT);
    if (ret == ESP_OK) {
        ESP_LOGI("BMP581", "Dummy read successful.");
    } else {
        ESP_LOGE("BMP581", "Dummy read failed: %s", esp_err_to_name(ret));
    }
    
    /* We first check chip status to make sure we successfully init */
    uint8_t chip_id = 0;
    uint8_t chipid_reg_address = CHIP_ID_REG;
    ESP_ERROR_CHECK(i2c_master_transmit(device_handle, &chipid_reg_address, 1,
                    MASTER_TRANSMIT_TIMEOUT));
    ESP_ERROR_CHECK(i2c_master_receive(device_handle, &chip_id, 1,
                    MASTER_TRANSMIT_TIMEOUT));
    /* DEBUG: CHIP_ID */
    // ESP_LOGI(TAG, "Read Chip ID: 0x%02X", chip_id);
    ESP_ERROR_CHECK(chip_id != 0x50);
    ESP_LOGI(TAG, "CHIP_ID register check passed.");

    /* Next we check STATUS: status_nvm_rdy==1 (bit offset: 1), status_nvm_err == 0 (bit offset: 2)*/
    /* Interesting enough, the way the value is presented in the data sheet is not
       how we got it. The bits were revered which caused confusion */
    uint8_t status = 0;
    uint8_t status_reg_address = STATUS_REG;
    ESP_ERROR_CHECK(i2c_master_transmit(device_handle, &status_reg_address, 1,
                    MASTER_TRANSMIT_TIMEOUT));
    ESP_ERROR_CHECK(i2c_master_receive(device_handle, &status, 1,
                                        MASTER_TRANSMIT_TIMEOUT));
    
    /* DEBUG: STATUS */
    // ESP_LOGI(TAG, "Read Status Value: 0x%02X", status);
    // printf("Raw STATUS register value: 0x%02X\n", status);

    /* Check status_nvm_rdy (bit offset 1)*/
    bool nvm_ready = (status & STATUS_NVM_RDY_BIT) >> STATUS_NVM_RDY_SHIFT;
    if (!nvm_ready) {
        printf("Error: STATUS_NVM_RDY bit is not set!\n");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    /* Check nvm_error (bit offset 2)*/
    bool nvm_error = (status & STATUS_NVM_ERR_BIT) >> STATUS_NVM_ERR_SHIFT;
    if (nvm_error) {
        printf("Error: STATUS_NVM_ERR bit is set!\n");
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    // printf("STATUS register check passed: NVM ready and no errors.\n");
    ESP_LOGI(TAG, "STATUS register check passed: NVM ready and no errors.");

    /* Check INT_STATUS: We are expecting value of 0x10 */
    uint8_t int_status = 0;
    uint8_t int_status_reg_address = INT_STATUS_REG;
    
    /* We first read to clear register */
    ESP_ERROR_CHECK(i2c_master_receive(device_handle, &int_status, 1,
                                        MASTER_TRANSMIT_TIMEOUT));
    /* We next ask for clean sample */
    ESP_ERROR_CHECK(i2c_master_transmit(device_handle, &int_status_reg_address,
                                        1, MASTER_TRANSMIT_TIMEOUT));
    /* We read the new sample*/
    ESP_ERROR_CHECK(i2c_master_receive(device_handle, &int_status, 1,
                                        MASTER_TRANSMIT_TIMEOUT));
    
    /* DEBUG: INT_STATUS.por */
    // ESP_LOGI(TAG, "Read Int_Status Value: 0x%02X", int_status);
    ESP_ERROR_CHECK(int_status != INT_STATUS_POR_BIT);
    ESP_LOGI(TAG, "INT_STATUS.por check passed.");
}

static void configure_bmp581(){
        
        // /* Performing a reboot to clear out the registers before we start*/
        // reset_bmp581();

        /* Let's make sure that the sensor is properly up and running */
        // sensor_check();

        // /* Lets start configuring the sensor */
        
        // //First we configure the sensor to register both temp and pressure.
        // //The data sheet calls for 0x3 to be placed int here for both temp
        // //and pressure.
        // uint8_t tmp[1] = {0};
        // i2c_read(device_handle, OSR_CONFIG_REG, tmp, sizeof(tmp));
        // tmp[0] |= BIT6; 
        // const uint8_t reg_and_data[] = {OSR_CONFIG_REG, tmp[0]};
        // i2c_write(device_handle, reg_and_data, sizeof(reg_and_data));

        // //Next we put the device in continuous mode
        // uint8_t tmp2[1] = {0};
        // i2c_read(device_handle, ODR_CONFIG_REG, tmp2, sizeof(tmp2));
        // tmp2[0] |= BIT1; // esp_bit_defs.h
        // tmp2[0] |= BIT2;
        // const uint8_t reg_and_data2[] = {ODR_CONFIG_REG, tmp2[0]};
        // i2c_write(device_handle, reg_and_data2, sizeof(reg_and_data2));

}

extern "C" void app_main(void)
{
    /* Data sheet calls for us to wait 2 ms after power up before communicating
       with sensor but we will add a buffer */
    vTaskDelay(pdMS_TO_TICKS(3));
    
    /* Let's first reset and then wait */
    i2c_controller_init();
    i2c_peripheral_init();
    reset_bmp581();

    /* We initalize the device again*/
    i2c_peripheral_init();
    sensor_check();
    // configure_bmp581();
    ESP_LOGI(TAG, "BMP581 initialization complete.");
}
