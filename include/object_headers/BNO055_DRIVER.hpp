#ifndef SHIZU_OBJECT_HEADERS_BNO055_DRIVER_HPP
#define SHIZU_OBJECT_HEADERS_BNO055_DRIVER_HPP
#include <cstdint>

namespace shizu {

// BNO055 ドライバが公開する最新サンプル。read_latest にこの構造体へのポインタを
// 渡すと、ドライバが内部キャッシュをコピーする (単一アドレス空間でのポインタ渡し)。
struct bno055_sample_t {
  uint32_t seq;               // 読み出しごとに +1
  float gx, gy, gz;           // 重力ベクトル [m/s^2]
  float lax, lay, laz;        // 線形加速度 (重力除去済) [m/s^2]
  float heading, roll, pitch; // オイラー角 [deg]
  uint8_t calib;              // キャリブレーション状態 (SYS<<6|GYR<<4|ACC<<2|MAG)
  bool valid;                 // 直近の I2C 読み出しが成功したか
};

// BNO055 (9 軸 IMU、NDOF フュージョン) を Shizuku オブジェクト化したドライバ。
// 専用スレッドが I2C で姿勢/加速度を周期サンプリングし内部にキャッシュする。
class BNO055_DRIVER {
public:
  BNO055_DRIVER() {};
  ~BNO055_DRIVER() {};
  static void init(); // オブジェクトスレッドのエントリ

  enum METHOD_IDs : uint32_t {
    // arg0 = bno055_sample_t* (呼び出し元のバッファ)。最新値をコピーする。
    read_latest = 0,
  };
};
} // namespace shizu

#endif // SHIZU_OBJECT_HEADERS_BNO055_DRIVER_HPP
