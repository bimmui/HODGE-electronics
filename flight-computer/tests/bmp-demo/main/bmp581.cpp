#include "bmp581.h"
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
#include <math.h>

/* BMP581/ESP32 Address Information*/
#define I2C_MASTER_PORT                          I2C_NUM_0
#define I2C_MASTER_SCL_IO                        22
#define I2C_MASTER_SDA_IO                        21
#define BMP5_I2C_ADDR_PRIM                       0x46
#define BMP5_I2C_ADDR_SEC                        0x47
#define SEA_LEVEL_PRESSURE                       103460.0

/* Timeout Defines*/
#define MASTER_TRANSMIT_TIMEOUT                  500
#define POWER_UP_DELAY_MS                        20
#define SOFT_RESET_DELAY_MS                      5

/* BMP581 Registers*/
#define BMP5_REG_CHIP_ID                         0x01 //Initialization check
#define BMP5_REG_TEMP_DATA_XLSB                  0x1D
#define BMP5_REG_TEMP_DATA_LSB                   0x1E
#define BMP5_REG_TEMP_DATA_MSB                   0x1F
#define BMP5_REG_PRESS_DATA_XLSB                 0x20
#define BMP5_REG_PRESS_DATA_LSB                  0x21
#define BMP5_REG_PRESS_DATA_MSB                  0x22
#define BMP5_REG_INT_STATUS                      0x27 //Initialization check
#define BMP5_INT_ASSERTED_POR_SOFTRESET_COMPLETE 0x10
#define BMP5_REG_STATUS                          0x28 //Initialization check
#define BMP5_REG_STATUS_NVM_RDY_BIT              0x02 //Initalization check
#define BMP5_REG_STATUS_NVM_RDY_SHIFT            1    //Initalization check
#define BMP5_REG_STATUS_NVM_ERR_BIT              0x04 //Initalization check
#define BMP5_REG_STATUS_NVM_ERR_SHIFT            2    //Initalization check
#define BMP5_REG_OSR_CONFIG                      0x36
#define BMP5_REG_ODR_CONFIG                      0x37


/* BMP581 Initialization Defines */
#define CMD_REG                                  0x7E
#define STATUS_REG                               0x28
#define STATUS_NVM_RDY_BIT                       0x02
#define STATUS_NVM_RDY_SHIFT                     1
#define STATUS_NVM_ERR_BIT                       0x04
#define STATUS_NVM_ERR_SHIFT                     2
#define CHIP_ID_REG                              0x01
#define INT_STATUS_REG                           0x27
#define INT_STATUS_POR_BIT                       0x10
#define NORMAL_MODE                              0x01

#define PRESS_DATA_MSB_REG                       0x22
#define PRESS_DATA_LSB_REG                       0x21
#define PRESS_DATA_XLSB_REG                      0x20

#define TEMP_DATA_MSB_REG                        0x1F
#define TEMP_DATA_LSB_REG                        0x1E
#define TEMP_DATA_XLSB_REG                       0x1D

/* BMP581 Command defines */
#define SOFT_RESET_CMD                           0xB6

/* BMP581 Configurations defines */
#define ODR_CONFIG_REG                           0x37
#define OSR_CONFIG_REG                           0x36
#define ODR_CONTINUOUS_MODE_BITS                 0x03
#define OSR_ENABLE_PRESSURE_BIT                  0x40
#define RECONFIG_DELAY_MS                        3

void _write(i2c_master_dev_handle_t sensor,
            uint8_t const *data_buf, const uint8_t data_len)
{
    ESP_ERROR_CHECK(i2c_master_transmit(sensor, data_buf, data_len, MASTER_TRANSMIT_TIMEOUT));
}

void _read(i2c_master_dev_handle_t sensor, const uint8_t reg_start_addr, uint8_t *rx, uint8_t rx_size)
{
    const uint8_t tx[] = {reg_start_addr};

    ESP_ERROR_CHECK(i2c_master_transmit_receive(sensor, tx, sizeof(tx), rx, rx_size, MASTER_TRANSMIT_TIMEOUT));
}

BMP581::BMP581(i2c_port_num_t port, i2c_addr_bit_len_t addr_len,
                uint16_t bmp581_address, uint32_t scl_clk_speed)
                :bmp581_addr_len(addr_len),
                 bmp581_addr(bmp581_address), 
                 bmp581_dev_handle(nullptr)
{
    /* Create a handle for the specific slave device (0x46) Baro */
    i2c_device_config_t i2c_bmp581_cfg = {};
    i2c_bmp581_cfg.dev_addr_length = addr_len;
    i2c_bmp581_cfg.device_address = BMP5_I2C_ADDR_PRIM;
    i2c_bmp581_cfg.scl_speed_hz = 100000; // 100 kHz for normal mode

    /* Grab the bus and place the peripheral on the the bus */
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_master_get_bus_handle(port, &bus_handle));
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &i2c_bmp581_cfg,
                                              &bmp581_dev_handle));
}

BMP581::~BMP581()
{
    i2c_master_bus_rm_device(bmp581_dev_handle);
}

void BMP581::bmp581_configure(void)
{
    /* First we configure the sensor to register both temp and pressure. */
    //Let's first grab what is currently there
    uint8_t curr_config[1] = {0};
    uint8_t osr_config_reg = OSR_CONFIG_REG;
    _read(bmp581_dev_handle, osr_config_reg, curr_config, sizeof(curr_config));

    //Next lets update it with the 6th bit enabled for pressure
    curr_config[0] = curr_config[0] || OSR_ENABLE_PRESSURE_BIT;
    uint8_t enable_press_data[2] = {0};
    enable_press_data[0] = OSR_CONFIG_REG;
    enable_press_data[1] = curr_config[0];

    //Let's now send it over to the peripheral
    _write(bmp581_dev_handle, enable_press_data, sizeof(enable_press_data));

    /* Now we put the device in continuous mode */
    // We first grab what is currently there
    curr_config[0] = 0;
    uint8_t odr_config_reg = ODR_CONFIG_REG;
    _read(bmp581_dev_handle, odr_config_reg, curr_config, sizeof(curr_config));

    // We now enable the bit for continous mode
    curr_config[0] = curr_config[0] || NORMAL_MODE;
    uint8_t enable_continuous_data[2];
    enable_continuous_data[0] = ODR_CONFIG_REG;
    enable_continuous_data[1] = curr_config[0];

    //Let's now send it over to the peripheral
    _write(bmp581_dev_handle, enable_continuous_data, sizeof(enable_continuous_data));
    
    /* We set delay called for in datasheet*/
    vTaskDelay(pdMS_TO_TICKS(RECONFIG_DELAY_MS));
}

float convert_to_altitude(uint32_t raw_press)
{
    return 44330.0 * (1.0 - pow(raw_press / SEA_LEVEL_PRESSURE, 0.1903));
}

bmp581_data BMP581::bmp581_get_sample(void)
{
    uint8_t data[6] = {0};
    uint8_t data_reg = TEMP_DATA_XLSB_REG; //pressure data register
    _write(bmp581_dev_handle, &data_reg, sizeof(data_reg));
    _read(bmp581_dev_handle, data_reg, data, sizeof(data));

    // Convert raw pressure data
    int32_t raw_temperature = (int32_t) ((int32_t) ((uint32_t)(((uint32_t)data[2] << 16) | ((uint16_t)data[1] << 8) | data[0]) << 8) >> 8);
    float temperature = (float)(raw_temperature / 65536.0);  // Scale as per datasheet

    // Convert raw temperature data 
    uint32_t raw_pressure = (uint32_t) (((uint32_t) data[5] << 16) | ((uint16_t)data[4] << 8) | ((data[3]) << 8));

    // raw_pressure -= SEA_LEVEL_PRESSURE; COME BACK TO ME
    float pressure = (float)(raw_pressure / 64.0);
    
    //Let's extract the sample
    bmp581_data sample;
    sample.pressure = pressure;
    sample.temperature = temperature;
    sample.altitude = convert_to_altitude(pressure);
    sample.raw_pressure = raw_pressure;

    return sample;
}

int BMP581::soft_reset(void){

    /* We organize the data in a container that we will write to reg */
    uint8_t reset_data[2];
    reset_data[0] = CMD_REG;
    reset_data[1] = SOFT_RESET_CMD;
    
    /* We perform the soft reset */
    _write(bmp581_dev_handle, reset_data, sizeof(reset_data));

    /* Lastly, let's enforce a 2 ms timeout which is the time needed for the
        reboot to happen but we will add a buffer */
    vTaskDelay(pdMS_TO_TICKS(SOFT_RESET_DELAY_MS));
    
    return EXIT_SUCCESS;
}

int BMP581::power_up_check(void){
        
    /* We perform a dummy read to initalize the sensor's interface */
    uint8_t dummy_reg[1] = {0};  // A valid register address
    _write(bmp581_dev_handle, dummy_reg, sizeof(dummy_reg));
    
    /* We first check chip status to make sure we successfully init */
    uint8_t chip_id[1] = {0};
    uint8_t chipid_reg_address = CHIP_ID_REG;
    _write(bmp581_dev_handle, chip_id, sizeof(chip_id));
    _read(bmp581_dev_handle, chipid_reg_address, chip_id, sizeof(chip_id));
    ESP_ERROR_CHECK(chip_id[0] != 0x50);

    /* Next we check STATUS: status_nvm_rdy==1 (bit offset: 1), status_nvm_err == 0 (bit offset: 2)*/
    /* Interesting enough, the way the value is presented in the data sheet is not
       how we got it. The bits were revered which caused confusion */
    uint8_t status[1] = {0};
    uint8_t status_reg_address = STATUS_REG;
    _write(bmp581_dev_handle, status, sizeof(status));
    _read(bmp581_dev_handle, status_reg_address, status, sizeof(status));

    /* Check status_nvm_rdy (bit offset 1)*/
    bool nvm_ready = (status[0] & STATUS_NVM_RDY_BIT) >> STATUS_NVM_RDY_SHIFT;
    if (!nvm_ready) {
        printf("Error: STATUS_NVM_RDY bit is not set!\n");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    /* Check nvm_error (bit offset 2)*/
    bool nvm_error = (status[0] & STATUS_NVM_ERR_BIT) >> STATUS_NVM_ERR_SHIFT;
    if (nvm_error) {
        printf("Error: STATUS_NVM_ERR bit is set!\n");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    /* Check INT_STATUS: We are expecting value of 0x10 */
    uint8_t int_status[1] = {0};
    uint8_t int_status_reg_address = INT_STATUS_REG;
    
    /* We first read to clear register */
    _read(bmp581_dev_handle, int_status_reg_address, int_status,
        sizeof(int_status));

    /* We next ask for clean sample */
    _write(bmp581_dev_handle, int_status, sizeof(int_status));
    _read(bmp581_dev_handle, int_status_reg_address, int_status,
        sizeof(int_status));
    
    ESP_ERROR_CHECK(int_status[0] != INT_STATUS_POR_BIT);
    return EXIT_SUCCESS;
}
