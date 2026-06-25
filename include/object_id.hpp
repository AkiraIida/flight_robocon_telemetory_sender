#ifndef SHIZU_OBJECT_ID_HPP
#define SHIZU_OBJECT_ID_HPP

#include <cstdint>

enum struct object_ids : uint32_t {
  KERNEL_OBJECT = 0,
  DRIVE_CONTROLLER = 1,
  MOTOR_DRIVER_FL=2,
  MOTOR_DRIVER_FR=3,
  MOTOR_DRIVER_RL=4,
  MOTOR_DRIVER_RR=5,
  IO_CONTROLLER=6,
  STDIO_DRIVER=7,
  LINE_SENSOR_DRIVER=8,
  CYW43_BL_DRIVER=9,
  WS2812_DRIVER=10,
  BLE_UART_DRIVER=11,
  HELLO_WORLD=12,
  BME280_DRIVER=13,     // 気圧/温度センサ (I2C 0x76)。高度を算出して公開する。
  BNO055_DRIVER=14,     // 9 軸 IMU (I2C 0x28)。重力/線形加速度/姿勢を公開する。
  TELEMETRY_SENDER=15,  // 上記 2 ドライバを購読し融合 → BLE UART で母艦へ送信する。
};

#endif // SHIZU_OBJECT_ID_HPP