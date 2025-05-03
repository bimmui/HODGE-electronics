use std::{collections::HashMap, error::Error};

use csv::Reader;
use ffi::RustSensorDataSnapshot;
use serde::Deserialize;

#[cxx::bridge]
mod ffi {
    #[derive(Debug)]
    struct RustSensorDataSnapshot {
        baro_altitude: f64,
        baro_temp: f32,
        baro_pressure: f32,

        gps_lat: f64,
        gps_lon: f64,
        gps_alt: f64,
        gps_speed: f32,
        gps_cog: f32,
        gps_mag_vari: f32,
        gps_num_sats: i32,
        gps_fix_status: i32,
        gps_year: i32,
        gps_month: i32,
        gps_day: i32,
        gps_hour: i32,
        gps_minute: i32,
        gps_second: i32,
        gps_fix_valid: bool,

        imu_accel_x: f32,
        imu_accel_y: f32,
        imu_accel_z: f32,
        imu_gyro_x: f32,
        imu_gyro_y: f32,
        imu_gyro_z: f32,
        imu_mag_x: f32,
        imu_mag_y: f32,
        imu_mag_z: f32,

        hg_accel_x: f32,
        hg_accel_y: f32,
        hg_accel_z: f32,

        ekf_yaw: f32,
        ekf_pitch: f32,
        ekf_roll: f32,
        ekf_vertical_velocity: f32,
        ekf_altitude: f32,
        efk_vertical_accel: f32,
        temp_temp_c: f32,
        timestamp: u64,
    }

}

struct QuantizeCompressor {
    
}