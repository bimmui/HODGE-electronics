#ifndef ICM20948_H
#define ICM20948_H

#include "driver/gpio.h"
#include "driver/i2c_master.h"

#define ICM20948_I2C_ADDRESS 0x69
#define ICM20948_I2C_ADDRESS_1 0x68
#define ICM20948_WHO_AM_I_VAL 0xEA

typedef struct
{
    float accel_x;
    float accel_y;
    float accel_z;
} icm20948_accel_value_t;

typedef struct
{
    float gyro_x;
    float gyro_y;
    float gyro_z;
} icm20948_gyro_value_t;

typedef struct
{
    float mag_x;
    float mag_y;
    float mag_z;
} ak09916_mag_value_t;

class ICM20948
{
public:
    typedef enum
    {
        ACCEL_FS_2G = 0,  // Accelerometer full scale range is +/- 2g
        ACCEL_FS_4G = 1,  // Accelerometer full scale range is +/- 4g
        ACCEL_FS_8G = 2,  // Accelerometer full scale range is +/- 8g
        ACCEL_FS_16G = 3, // Accelerometer full scale range is +/- 16g
    } icm20948_accel_fs_t;

    typedef enum
    {
        GYRO_FS_250DPS = 0,  // Gyroscope full scale range is +/- 250 degree per sencond
        GYRO_FS_500DPS = 1,  // Gyroscope full scale range is +/- 500 degree per sencond
        GYRO_FS_1000DPS = 2, // Gyroscope full scale range is +/- 1000 degree per sencond
        GYRO_FS_2000DPS = 3, // Gyroscope full scale range is +/- 2000 degree per sencond
    } icm20948_gyro_fs_t;

    typedef enum
    {
        INTERRUPT_PIN_ACTIVE_HIGH = 0, // The icm20948 sets its INT pin HIGH on interrupt
        INTERRUPT_PIN_ACTIVE_LOW = 1   // The icm20948 sets its INT pin LOW on interrupt
    } icm20948_int_pin_active_level_t;

    typedef enum
    {
        INTERRUPT_PIN_PUSH_PULL = 0, // The icm20948 configures its INT pin as push-pull
        INTERRUPT_PIN_OPEN_DRAIN = 1 // The icm20948 configures its INT pin as open drain
    } icm20948_int_pin_mode_t;

    typedef enum
    {
        INTERRUPT_LATCH_50US = 0,         // The icm20948 produces a 50 microsecond pulse on interrupt
        INTERRUPT_LATCH_UNTIL_CLEARED = 1 // The icm20948 latches its INT pin to its active level, until interrupt is cleared
    } icm20948_int_latch_t;

    typedef enum
    {
        INTERRUPT_CLEAR_ON_ANY_READ = 0,   // INT_STATUS register bits are cleared on any register read
        INTERRUPT_CLEAR_ON_STATUS_READ = 1 // INT_STATUS register bits are cleared only by reading INT_STATUS value
    } icm20948_int_clear_t;

    typedef enum
    {
        ICM20948_DLPF_0,
        ICM20948_DLPF_1,
        ICM20948_DLPF_2,
        ICM20948_DLPF_3,
        ICM20948_DLPF_4,
        ICM20948_DLPF_5,
        ICM20948_DLPF_6,
        ICM20948_DLPF_7,
        ICM20948_DLPF_OFF
    } icm20948_dlpf_t;

    typedef struct
    {
        gpio_num_t interrupt_pin;                      // GPIO connected to icm20948 INT pin
        icm20948_int_pin_active_level_t active_level;  // Active level of icm20948 INT pin
        icm20948_int_pin_mode_t pin_mode;              // Push-pull or open drain mode for INT pin
        icm20948_int_latch_t interrupt_latch;          // The interrupt pulse behavior of INT pin
        icm20948_int_clear_t interrupt_clear_behavior; // Interrupt status clear behavior
    } icm20948_int_config_t;

    // not using these but might as well include them, will try to flesh these out in the future
    // extern const uint8_t icm20948_DATA_RDY_INT_BIT;      // DATA READY interrupt bit
    // extern const uint8_t icm20948_I2C_MASTER_INT_BIT;    // I2C MASTER interrupt bit
    // extern const uint8_t icm20948_FIFO_OVERFLOW_INT_BIT; // FIFO Overflow interrupt bit
    // extern const uint8_t icm20948_MOT_DETECT_INT_BIT;    // MOTION DETECTION interrupt bit
    // extern const uint8_t icm20948_ALL_INTERRUPTS;        // All interrupts supported by icm20948

    ICM20948(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t icm20948_address, uint32_t scl_clk_speed);
    ~ICM20948();

    // prob no need for copy constructor and copy assignments, that'll make things messy
    // ICM20948(const ICM20948 &other);
    // ICM20948 &operator=(const ICM20948 &other);

    void configureICM20948(icm20948_accel_fs_t acce_fs, icm20948_gyro_fs_t gyro_fs);
    uint8_t getICM20948ID();
    void wakeup();
    void sleep();
    void reset();

    icm20948_gyro_fs_t getGyroFS();
    void getGyro(icm20948_gyro_value_t *gyro_vals);

    icm20948_accel_fs_t getAccelFS();
    void getAccel(icm20948_accel_value_t *accel_vals);

    void initAK09916(i2c_port_num_t port, i2c_addr_bit_len_t addr_len, uint16_t ak09916_address, uint32_t scl_clk_speed);
    void configureAK09916();
    void getMag(ak09916_mag_value_t *mag_vals);

private:
    typedef struct
    {
        int16_t raw_accel_x;
        int16_t raw_accel_y;
        int16_t raw_accel_z;
    } icm20948_raw_accel_value_t;

    typedef struct
    {
        int16_t raw_gyro_x;
        int16_t raw_gyro_y;
        int16_t raw_gyro_z;
    } icm20948_raw_gyro_value_t;

    typedef struct
    {
        int16_t raw_mag_x;
        int16_t raw_mag_y;
        int16_t raw_mag_z;
    } icm20948_raw_mag_value_t;

    void setBank(uint8_t bank);
    void activateI2CBypass();
    void getRawMag();

    void setGyroFS(icm20948_gyro_fs_t gyro_fs);
    void setGyroSensitivity();
    void getRawGyro();

    void setAccelFS(icm20948_accel_fs_t accel_fs);
    void setAccelSensitivity();
    void getRawAccel();

    void enableDLPF(bool enable);
    void setAccelDLPF(icm20948_dlpf_t dlpf_accel);
    void setGyroDLPF(icm20948_dlpf_t dlpf_gyro);

    icm20948_raw_accel_value_t curr_raw_accel_vals;
    icm20948_raw_gyro_value_t curr_raw_gyro_vals;
    icm20948_raw_mag_value_t curr_raw_mag_vals;

    float gyro_sensitivity;
    float accel_sensitivity;

    i2c_addr_bit_len_t icm20948_addr_len;
    uint16_t icm20948_addr;
    i2c_master_dev_handle_t icm20948_dev_handle;

    i2c_addr_bit_len_t ak09916_addr_len;
    uint16_t ak09916_addr;
    i2c_master_dev_handle_t ak09916_dev_handle;
};

#endif