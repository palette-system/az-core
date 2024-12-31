#ifndef AzConfig_h
#define AzConfig_h

// キーボードのタイプ ( 0=BLE 1=USB )
#define KEYBOARD_TYPE 1

// ESP32 のタイプ ( 0 = ノーマル ESP32 / 9 = ESP32S3 )
#define CPUTYPE_ESP32 9

// デフォルトのJSON
//   0 = AZ-CORE
//   1 = AtomS3
//   2 = AZ-Nubkey
//   3 = AZ-Magicpad
//   4 = AZSENSOR
#define SETTING_JSON_DEFAULT_TYPE 4

#endif // AzConfig_h
