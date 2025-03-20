#ifndef AzConfig_h
#define AzConfig_h

// キーボードの接続ライブラリ ( 0=NimBLE / 1=NomalBLE / 2=USB )
#define KEYBOARD_TYPE 1

// ESP32 のタイプ ( 0 = ノーマル ESP32 / 8 = ESP32C6 / 9 = ESP32S3 )
#define CPUTYPE_ESP32 8

// WIFI機能（ 0 = WIFI機能なし / 1 = WIFI機能あり ）
#define WIFI_FLAG 0

// デフォルトのJSON
//   0 = AZ-CORE(M5Stamp)
//   1 = AtomS3
//   2 = AZ-Nubkey
//   3 = AZ-Magicpad
//   4 = AZSENSOR(XIAO ESP32S3)
//   5 = XIAO ESP32C6
//   6 = AZCARD(XIAO ESP32S3)
//   7 = AZPOCKET(XIAO ESP32C6)
#define SETTING_JSON_DEFAULT_TYPE 5

#endif // AzConfig_h
