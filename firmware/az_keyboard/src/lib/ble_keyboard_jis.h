#ifndef BleKeyboardJIS_h
#define BleKeyboardJIS_h
#if CONFIG_IDF_FIRMWARE_CHIP_ID == 0x00
// 0x00 = ノーマルESP32

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "nimconfig.h"
#if defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL)

#include "Arduino.h"
#include "NimBLEHIDDevice.h"
#include "NimBLECharacteristic.h"

#include "ble_callbacks.h"
#include "../../az_common.h"







// BLEキーボードクラス
class BleKeyboardJIS
{
  public:
    /* プロパティ */
    BleConnectionStatus* connectionStatus;
    NimBLEServer* pServer; // ble サービス
    NimBLEService* pDeviceInfoService; // bleサービス INFO
    NimBLECharacteristic* pPnpCharacteristic; // bleサービス pnp
    NimBLECharacteristic* pManufacturerCharacteristic; // bleサービス Manufacturer
    NimBLEService* pHidService; // HID サービス
    NimBLECharacteristic* pHidInfoCharacteristic; // HID サービスINFO
    NimBLECharacteristic* pReportMapCharacteristic; // HID レポートマップ
    NimBLECharacteristic* pHidControlCharacteristic; // HID コントローラー
    NimBLECharacteristic* pProtocolModeCharacteristic; // HID モード
    NimBLECharacteristic* pInputCharacteristic; // HID input 1 (キーコードの送信)
    NimBLEDescriptor* pDesc1; // HID input 1 (キーコードの送信)
    NimBLECharacteristic* pOutputCharacteristic; // HID output 1 (capslockとかの情報取得)
    NimBLEDescriptor* pDesc2; //  HID output 1 (capslockとかの情報取得)
    NimBLECharacteristic* pInputCharacteristic2; // HID input 2 (メディアキーコード送信)
    NimBLEDescriptor* pDesc3; // HID input 2 (メディアキーコード送信)
    NimBLECharacteristic* pInputCharacteristic3; // HID input 3 (マウス送信)
    NimBLEDescriptor* pDesc4; // HID input 3 (マウス送信)
    NimBLECharacteristic* pInputCharacteristic4; // HID input 4 (REMAP 送信)
    NimBLEDescriptor* pDesc5; // HID input 4 (REMAP 送信)
    NimBLECharacteristic* pOutputCharacteristic2; // HID output 4 (REMAP 情報取得)
    NimBLEDescriptor* pDesc6; // HID input 4 (REMAP 情報取得)
    NimBLEService* pBatteryService; // バッテリーサービス
    NimBLECharacteristic* pBatteryLevelCharacteristic; // バッテリーサービス レベル
    NimBLE2904* pBatteryLevelDescriptor; // バッテリーサービス レベル
    NimBLEAdvertising* pAdvertising; // アドバタイズ (ペアリングを待つ情報を送信)
    uint8_t *_hidReportDescriptor; // HID レポート設定
    unsigned short _hidReportDescriptorSize; // HID レポートのサイズ
    uint8_t batteryLevel; // バッテリーレベル 0-100
    KeyReport _keyReport;
    MediaKeyReport _mediaKeyReport;
    uint8_t _MouseButtons; // マウスボタン情報
    std::string deviceManufacturer; // 会社名
    std::string deviceName; // デバイス名
    static void taskServer(void* pvParameter);

    /* メソッド */
    BleKeyboardJIS(void); // コンストラクタ
    void set_report_map(uint8_t * report_map, unsigned short report_size);
    void begin(std::string deviceName = "az_keyboard", std::string deviceManufacturer = "PaletteSystem");
    void end(void);
    bool isConnected(void);
    unsigned short modifiers_press(unsigned short k);
    unsigned short modifiers_release(unsigned short k);
    void shift_release(); // Shiftを離す
    unsigned short modifiers_media_press(unsigned short k);
    unsigned short modifiers_media_release(unsigned short k);
    void sendReport(KeyReport* keys);
    void sendReport(MediaKeyReport* keys);
    void mouse_click(uint8_t b);
    void mouse_press(uint8_t b);
    void mouse_release(uint8_t b);
    void mouse_move(signed char x, signed char y, signed char wheel, signed char hWheel);
    size_t press_set(uint8_t k); // 指定したキーだけ押す
    size_t press_raw(unsigned short k);
    size_t release_raw(unsigned short k);
    void releaseAll(void);
    bool onShift(); // Shiftが押されている状態かどうか(物理的に)
    void setConnInterval(int interval_type);
};



#endif // CONFIG_BT_NIMBLE_ROLE_PERIPHERAL
#endif // CONFIG_BT_ENABLED
#endif // CONFIG_IDF_FIRMWARE_CHIP_ID == 0x00
#endif // BleKeyboardJIS_h
