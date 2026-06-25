#ifndef SHIZU_OBJECT_HEADERS_HELLO_WORLD_HPP
#define SHIZU_OBJECT_HEADERS_HELLO_WORLD_HPP
#include <cstdint>

namespace shizu {
// BLE_UART_DRIVER (Nordic UART Service) を介して "Hello, World" + プロセッサ時間
// を周期送信するデモアプリ。さらにホストから RX 経由で Unix エポックを受け取り、
// uptime との差分 (offset) を保持してウォールクロックを再構成する (RTC 同調)。
class HELLO_WORLD {
public:
  HELLO_WORLD() {};
  ~HELLO_WORLD() {};
  static void main(); // オブジェクトスレッドのエントリ

  enum METHOD_IDs : uint32_t {
    // BLE_UART の rx_sink から 1 バイトずつ呼ばれる。行を組み立て、
    // "T<epoch_us>\n" を受けると時刻同期する。
    rx_byte = 0,
  };
};
} // namespace shizu

#endif // SHIZU_OBJECT_HEADERS_HELLO_WORLD_HPP
