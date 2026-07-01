#ifndef SHIZU_SOFT_MATH_HPP
#define SHIZU_SOFT_MATH_HPP
// ===========================================================================
//  ソフトフロート専用 数学関数 (FPU を一切使わない実装)
// ===========================================================================
//  【なぜ必要か】
//  本プロジェクトの main ターゲットは -mfloat-abi=soft でビルドされるが、Pico SDK
//  本体とその libm はハードフロート (FPv5) でビルドされている。そのため標準の
//  sqrtf/powf/expf/logf を呼ぶと「FPU 命令を含む libm 実装」へリンクされ、実行時に
//  vsqrt/vcmp 等が走って CPU の CONTROL.FPCA ビットが立つ。
//
//  Shizuku カーネルは例外フレームを基本 8 ワード (exception_frame_t) と仮定しており、
//  FPCA=1 のスレッドが次に SVC を発行すると HW が「拡張 (FP) 例外フレーム」を積むため
//  カーネルのフレーム操作が破綻し HardFault する。
//
//  → 基本の float 加減乗除/比較は __aeabi_* (ソフトウェア。FPU 不使用) に落ちるので
//    安全。これらだけを使って sqrt/pow 等を自前実装し、FPU 命令を 1 つも発行しない
//    ことで FPCA を立てない。精度はテレメトリ用途に十分なレベル。
//
//  すべて static inline。各 TU に取り込まれ、-mfloat-abi=soft でコンパイルされる
//  限り FPU 命令を含まない (objdump で確認済み)。
// ===========================================================================
#include <cstdint>

namespace shizu {
namespace soft_math {

// 符号ビットを落とすだけ (FPU 不使用)。
static inline float fabsf(float x) {
  union {
    float f;
    uint32_t i;
  } u;
  u.f = x;
  u.i &= 0x7FFFFFFFu;
  return u.f;
}

// Newton 法による平方根。初期値をビット演算で作り 3 回反復する。
static inline float sqrtf(float x) {
  if (x <= 0.0f)
    return 0.0f;
  union {
    float f;
    uint32_t i;
  } u;
  u.f = x;
  u.i = (u.i >> 1) + 0x1FBD1DF5u; // おおよその平方根 (有名なビックリ初期値)
  float y = u.f;
  y = 0.5f * (y + x / y);
  y = 0.5f * (y + x / y);
  y = 0.5f * (y + x / y);
  return y;
}

// 自然対数。仮数部を [1,2) に正規化し、t=(m-1)/(m+1) のべき級数で評価する。
static inline float logf(float x) {
  if (x <= 0.0f)
    return -1.0e30f;
  union {
    float f;
    uint32_t i;
  } u;
  u.f = x;
  int e = (int)((u.i >> 23) & 0xFF) - 127;     // 2 の指数
  u.i = (u.i & 0x007FFFFFu) | 0x3F800000u;     // 仮数 m ∈ [1,2)
  float m = u.f;
  float t = (m - 1.0f) / (m + 1.0f);
  float t2 = t * t;
  // ln(m) = 2t (1 + t^2/3 + t^4/5 + t^6/7)
  float s = t * (2.0f + t2 * (2.0f / 3.0f + t2 * (2.0f / 5.0f + t2 * (2.0f / 7.0f))));
  return s + (float)e * 0.69314718056f; // + e*ln2
}

// 指数関数。2 冪部を指数ビット直書きで作り、残差を Taylor 展開する。
static inline float expf(float x) {
  float t = x * 1.44269504089f; // x / ln2
  int n = (int)(t >= 0.0f ? t + 0.5f : t - 0.5f);
  float f = (t - (float)n) * 0.69314718056f; // 残差 (|f| <= ~0.35)
  float ef =
      1.0f +
      f * (1.0f +
           f * (0.5f + f * (1.0f / 6.0f +
                            f * (1.0f / 24.0f + f * (1.0f / 120.0f)))));
  int e = n + 127;
  if (e <= 0)
    return 0.0f;
  if (e >= 255)
    return 1.0e30f;
  union {
    float f;
    uint32_t i;
  } u;
  u.i = (uint32_t)e << 23; // 2^n
  return ef * u.f;
}

// base^exp (base > 0)。pow(b,e) = exp(e * log(b))。
static inline float powf(float base, float exp_) {
  if (base <= 0.0f)
    return 0.0f;
  return expf(exp_ * logf(base));
}

// 床関数 (整数化キャストのみ。FPU 不使用)。
static inline float floorf(float x) {
  int i = (int)x;
  float fi = (float)i;
  return (x < fi) ? (fi - 1.0f) : fi;
}

// [-pi/2, pi/2] 上の sin テイラー近似 (x^7 まで、最大誤差 ~1.7e-4)。
static inline float _sin_poly(float x) {
  float x2 = x * x;
  return x * (1.0f + x2 * (-1.0f / 6.0f +
                          x2 * (1.0f / 120.0f + x2 * (-1.0f / 5040.0f))));
}

// 正弦。任意入力を [-pi, pi] に畳んでから [-pi/2, pi/2] に折り返して評価する。
static inline float sinf(float x) {
  const float PI = 3.14159265358979f;
  const float TWO_PI = 6.28318530717959f;
  const float HALF_PI = 1.57079632679490f;
  const float INV_TWO_PI = 0.159154943091895f;
  x = x - TWO_PI * floorf((x + PI) * INV_TWO_PI); // → [-pi, pi)
  if (x > HALF_PI)
    x = PI - x;
  else if (x < -HALF_PI)
    x = -PI - x;
  return _sin_poly(x);
}

// 余弦。cos(x) = sin(x + pi/2)。
static inline float cosf(float x) { return sinf(x + 1.57079632679490f); }

} // namespace soft_math
} // namespace shizu

#endif // SHIZU_SOFT_MATH_HPP
