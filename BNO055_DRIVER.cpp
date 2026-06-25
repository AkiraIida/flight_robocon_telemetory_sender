// ===========================================================================
//  BNO055_DRIVER — 9 軸 IMU を Shizuku オブジェクト化したドライバ
// ===========================================================================
//  元実装 test_firmware/altitude_fusion_wifi.c の BNO055 部分を移植。NDOF モード
//  でセンサ内蔵フュージョンを使い、重力ベクトル・線形加速度・オイラー角・較正状態を
//  周期サンプリングしてキャッシュ、read_latest で他オブジェクトへ渡す。
// ===========================================================================
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <export_method.hpp>
#include <i2c_bus.hpp>
#include <obj_api.hpp>
#include <object_headers/BNO055_DRIVER.hpp>
#include <pico/time.h>

namespace shizu {

// ---- レジスタ定義 ----------------------------------------------------------
static constexpr uint8_t BNO055_ADDR = 0x28;
static constexpr uint8_t REG_CHIP_ID = 0x00;
static constexpr uint8_t REG_GRV_DATA_X_LSB = 0x2E; // gravity vector
static constexpr uint8_t REG_LIA_DATA_X_LSB = 0x28; // linear accel
static constexpr uint8_t REG_EUL_HEAD_LSB = 0x1A;   // euler heading/roll/pitch
static constexpr uint8_t REG_CALIB_STAT = 0x35;
static constexpr uint8_t REG_OPR_MODE = 0x3D;
static constexpr uint8_t REG_SYS_TRIGGER = 0x3F;
static constexpr uint8_t REG_UNIT_SEL = 0x3B;

static constexpr uint8_t OPMODE_CONFIG = 0x00;
static constexpr uint8_t OPMODE_NDOF = 0x0C;
static constexpr uint8_t USE_MODE = OPMODE_NDOF;

// ---- 最新サンプル ----------------------------------------------------------
static bno055_sample_t latest = {};

// ===========================================================================
//  BNO055 低レベル
// ===========================================================================
static void set_mode(uint8_t mode) {
  i2c_bus::write_reg(BNO055_ADDR, REG_OPR_MODE, OPMODE_CONFIG);
  obj_api::yield_us(30000);
  if (mode != OPMODE_CONFIG) {
    i2c_bus::write_reg(BNO055_ADDR, REG_OPR_MODE, mode);
    obj_api::yield_us(30000);
  }
}

static bool bno_init_sensor() {
  uint8_t id;
  if (i2c_bus::read_regs(BNO055_ADDR, REG_CHIP_ID, &id, 1) < 0) {
    printf("[BNO055] I2C read failed at ID\n");
    return false;
  }
  if (id != 0xA0) {
    printf("[BNO055] wrong ID 0x%02X (expected 0xA0)\n", id);
    return false;
  }

  // POR リセット → 起動待ち (~650ms)。yield で他スレッドを止めない。
  i2c_bus::write_reg(BNO055_ADDR, REG_SYS_TRIGGER, 0x20);
  obj_api::yield_us(700000);
  i2c_bus::write_reg(BNO055_ADDR, REG_UNIT_SEL, 0x00); // m/s^2, deg
  obj_api::yield_us(10000);
  set_mode(USE_MODE);
  return true;
}

// 重力/線形加速度/オイラー角/較正状態を 1 サンプル読む。失敗時 false。
static bool read_motion(bno055_sample_t *s) {
  uint8_t buf[6];

  if (i2c_bus::read_regs(BNO055_ADDR, REG_GRV_DATA_X_LSB, buf, 6) < 0)
    return false;
  s->gx = (int16_t)((buf[1] << 8) | buf[0]) / 100.0f;
  s->gy = (int16_t)((buf[3] << 8) | buf[2]) / 100.0f;
  s->gz = (int16_t)((buf[5] << 8) | buf[4]) / 100.0f;

  if (i2c_bus::read_regs(BNO055_ADDR, REG_LIA_DATA_X_LSB, buf, 6) < 0)
    return false;
  s->lax = (int16_t)((buf[1] << 8) | buf[0]) / 100.0f;
  s->lay = (int16_t)((buf[3] << 8) | buf[2]) / 100.0f;
  s->laz = (int16_t)((buf[5] << 8) | buf[4]) / 100.0f;

  if (i2c_bus::read_regs(BNO055_ADDR, REG_EUL_HEAD_LSB, buf, 6) < 0)
    return false;
  s->heading = (int16_t)((buf[1] << 8) | buf[0]) / 16.0f;
  s->roll = (int16_t)((buf[3] << 8) | buf[2]) / 16.0f;
  s->pitch = (int16_t)((buf[5] << 8) | buf[4]) / 16.0f;

  if (i2c_bus::read_regs(BNO055_ADDR, REG_CALIB_STAT, &s->calib, 1) < 0)
    return false;
  return true;
}

// ===========================================================================
//  公開メソッド
// ===========================================================================
static void method_read_latest(uint32_t _caller_obj_id,
                               uint32_t _caller_thread_id, uint32_t out_ptr,
                               uint32_t _arg1) {
  (void)_caller_obj_id;
  (void)_caller_thread_id;
  (void)_arg1;
  if (out_ptr == 0)
    return;
  memcpy((void *)(uintptr_t)out_ptr, (const void *)&latest, sizeof(latest));
}

// ===========================================================================
//  オブジェクトエントリ
// ===========================================================================
void BNO055_DRIVER::init() {
  printf("[BNO055] init\n");
  export_method<method_read_latest>(BNO055_DRIVER::METHOD_IDs::read_latest);

  i2c_bus::init();
  if (!bno_init_sensor()) {
    printf("[BNO055] init failed — driver idles (valid=false)\n");
    while (true) {
      latest.valid = false;
      obj_api::yield_us(500000);
    }
  }
  printf("[BNO055] sensor OK (mode=0x%02X)\n", USE_MODE);

  // NDOF フュージョン出力はハード的に 100Hz 固定なので、その限界で読み続ける
  // (これより速く読んでも同じ値しか出ない)。
  while (true) {
    bno055_sample_t s = {};
    if (read_motion(&s)) {
      s.seq = latest.seq + 1;
      s.valid = true;
      latest = s;
    } else {
      latest.valid = false;
    }
    obj_api::yield_us(10000); // 100Hz (NDOF フュージョン出力レート)
  }
}

} // namespace shizu
