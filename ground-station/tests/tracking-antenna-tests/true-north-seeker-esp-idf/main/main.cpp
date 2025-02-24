#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "LSM9DS1_ESP_IDF.h"
#include "NewUW_MahonyAHRS.h"
#include "AccelStepper.h"

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA GPIO_NUM_21
#define I2C_MASTER_SCL GPIO_NUM_22
#define I2C_MASTER_FREQ 400000 // 400 kHz
#define SPR 1600

static const char *MAIN_TAG = "LSM9DS1_main";
static const char *AVG_YAW = "average_yaw_calc";

long radians_to_steps(double radians)
{
    return static_cast<int>((radians / (2.0 * M_PI)) * SPR);
}

long degrees_to_radians(float degrees)
{
    return degrees * (M_PI / 180);
}

float calc_average_yaw(LSM9DS1_ESP_IDF lsm)
{
    uint64_t now = 0;
    uint64_t last = 0;
    float yaw_sum = 0;
    float yaw_avg = 0;

    ESP_LOGE(AVG_YAW, "Beginning average yaw calculation...");
    for (int i = 0; i < 1000; i++)
    {
        sensors_event_t aevt, megt, gevt, tevt;
        lsm.getEvent(&aevt, &megt, &gevt, &tevt);

        float Gxyz[3], Axyz[3], Mxyz[3];
        get_scaled_IMU(Gxyz, Axyz, Mxyz, aevt, megt, gevt);

        Axyz[0] = -Axyz[0]; // fix accel/gyro handedness
        Gxyz[0] = -Gxyz[0]; // must be done after offsets & scales applied to raw data

        now = esp_timer_get_time();
        double deltat = (now - last) * 1.0e-6; // seconds since last update
        last = now;

        euler_angles temp;
        temp.yaw = 0; // doing this stupid fix to egt rid of the uninitialized errors

        MahonyQuaternionUpdate(temp, Axyz[0], Axyz[1], Axyz[2], Gxyz[0], Gxyz[1], Gxyz[2],
                               Mxyz[0], Mxyz[1], Mxyz[2], deltat);

        yaw_sum += temp.yaw;
    }
    yaw_avg = yaw_sum / 1000;

    ESP_LOGE(AVG_YAW, "Average yaw: %lf", yaw_avg);

    return yaw_avg;
}

extern "C" void app_main(void)
{
    i2c_master_bus_config_t i2c_mst_config = {};
    i2c_mst_config.i2c_port = I2C_MASTER_NUM;
    i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
    i2c_mst_config.scl_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SCL);
    i2c_mst_config.sda_io_num = static_cast<gpio_num_t>(CONFIG_I2C_MASTER_SDA);
    i2c_mst_config.glitch_ignore_cnt = 7;
    i2c_mst_config.flags.enable_internal_pullup = true;

    i2c_master_bus_handle_t bus_handle = NULL;
    esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(MAIN_TAG, "Failed to create I2C bus: %s", esp_err_to_name(err));
        return;
    }

    // 2) Instantiate LSM9DS1 object and AccelStepper object
    LSM9DS1_ESP_IDF lsm;
    AccelStepper stepper(AccelStepper::DRIVER, static_cast<gpio_num_t>(CONFIG_STEP_PIN), static_cast<gpio_num_t>(CONFIG_DIRECTION_PIN));

    // 3) Init I2C with 7-bit addresses for both subchips in the LSM9DS1
    err = lsm.initI2C(bus_handle,
                      LSM9DS1_ADDRESS_ACCELGYRO,
                      LSM9DS1_ADDRESS_MAG,
                      I2C_MASTER_FREQ);
    if (err != ESP_OK)
    {
        ESP_LOGE(MAIN_TAG, "LSM9DS1 initI2C failed: %s", esp_err_to_name(err));
        return;
    }

    // 4) Begin (check IDs, do resets, etc.)
    if (!lsm.begin())
    {
        ESP_LOGE(MAIN_TAG, "LSM9DS1 not detected or config error!");
        return;
    }
    ESP_LOGI(MAIN_TAG, "LSM9DS1 found and initialized.");

    // 5) Setting up the stepper
    stepper.setMaxSpeed(2000);
    stepper.setAcceleration(1500);

    // 6) Getting average angular distance from true north and pointing there
    float distance_to_north = calc_average_yaw(lsm);
    stepper.move(-radians_to_steps(degrees_to_radians(distance_to_north)));
    if (stepper.distanceToGo() != 0)
    {
        stepper.run();
    }

    // 7) loop where we point towards the rocket ideally
    while (1)
    {
        // Option A: read raw data directly
        // lsm.read();

        // ESP_LOGI(TAG, "Accel: X:%d Y:%d Z:%d   Gyro: X:%d Y:%d Z:%d   Mag: X:%d Y:%d Z:%d   Temp: %d",
        //          lsm.accelData.x, lsm.accelData.y, lsm.accelData.z,
        //          lsm.gyroData.x, lsm.gyroData.y, lsm.gyroData.z,
        //          lsm.magData.x, lsm.magData.y, lsm.magData.z,
        //          lsm.temperature);

        // Option B: get "unified" style events

        sensors_event_t aevt, megt, gevt, tevt;
        lsm.getEvent(&aevt, &megt, &gevt, &tevt);
        ESP_LOGI(MAIN_TAG, "Accel: %.2f,%.2f,%.2f m/s^2   Gyro: %.2f,%.2f,%.2f rad/s   "
                           "Mag: %.2f,%.2f,%.2f gauss   Temp: %.2f C",
                 aevt.acceleration.x, aevt.acceleration.y, aevt.acceleration.z,
                 gevt.gyro.x, gevt.gyro.y, gevt.gyro.z,
                 megt.magnetic.x, megt.magnetic.y, megt.magnetic.z,
                 tevt.temperature);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
