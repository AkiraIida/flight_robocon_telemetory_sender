#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

#ifdef ENABLE_CLASSIC
#undef ENABLE_CLASSIC
#endif
#define ENABLE_CLASSIC 1
#define ENABLE_PRINTF_HEXDUMP
#define ENABLE_HID_HOST
#define ENABLE_SDP
#define ENABLE_L2CAP
#define ENABLE_SDP_QUERIES

// BLE_UART_DRIVER (Nordic UART Service ペリフェラル) 用。
// CYW43 無線は排他利用だが、btstack はデュアルモードでビルドしておく。
#define ENABLE_LE_PERIPHERAL
#define ENABLE_LE_DATA_LENGTH_EXTENSION
// Numeric Comparison は LE Secure Connections (LESC) 専用の方式。これを定義
// しないと BTstack は Legacy ペアリングに落ち、MITM 必須のため Passkey Entry に
// なってしまう。CYW43 のコントローラ側 P-256 を使って LESC を行う。
#define ENABLE_LE_SECURE_CONNECTIONS
#define MAX_NR_GATT_CLIENTS 0
#define MAX_NR_LE_DEVICE_DB_ENTRIES 1
#define NVM_NUM_DEVICE_DB_ENTRIES 1

#define HCI_ACL_PAYLOAD_SIZE 1021
#define HCI_OUTGOING_PRE_BUFFER_SIZE 64
#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 64

#define MAX_NR_HCI_CONNECTIONS 2
#define MAX_NR_L2CAP_SERVICES 4
#define MAX_NR_L2CAP_CHANNELS 4
#define MAX_NR_SDP_CLIENTS 2
#define MAX_NR_HID_HOST_CONNECTIONS 2

// HID通信（Control / Interrupt）用のチャンネルとサービスが
// 足りなくなるのを防ぐため、これも少し余裕を持たせておきます
#define MAX_NR_L2CAP_CHANNELS 4
#define MAX_NR_HCI_CONNECTIONS 2

#define NVM_NUM_LINK_KEYS 2
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES 2

#define HAVE_MALLOC
#define HAVE_ASSERT
#define HAVE_EMBEDDED_TIME_MS

#endif // BTSTACK_CONFIG_H
