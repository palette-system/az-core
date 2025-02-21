#include "../../az_config.h"

#if KEYBOARD_TYPE == 1
// 1 = Normal BLE

#ifndef ESP32_BLE_KEYBOARD_H
#define ESP32_BLE_KEYBOARD_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "BLEHIDDevice.h"
#include "BLECharacteristic.h"

#include "Print.h"
#include "hid_common.h"
#include "./ble_callbacks.h"
#include "../../az_common.h"





#define BLE_KEYBOARD_VERSION "0.0.4"
#define BLE_KEYBOARD_VERSION_MAJOR 0
#define BLE_KEYBOARD_VERSION_MINOR 0
#define BLE_KEYBOARD_VERSION_REVISION 4

#define INPUT_REPORT_RAW_MAX_LEN_C6 32
#define OUTPUT_REPORT_RAW_MAX_LEN_C6 32


typedef uint8_t MediaKeyReport[2];

const MediaKeyReport KEY_MEDIA_NEXT_TRACK = {1, 0};
const MediaKeyReport KEY_MEDIA_PREVIOUS_TRACK = {2, 0};
const MediaKeyReport KEY_MEDIA_STOP = {4, 0};
const MediaKeyReport KEY_MEDIA_PLAY_PAUSE = {8, 0};
const MediaKeyReport KEY_MEDIA_MUTE = {16, 0};
const MediaKeyReport KEY_MEDIA_VOLUME_UP = {32, 0};
const MediaKeyReport KEY_MEDIA_VOLUME_DOWN = {64, 0};
const MediaKeyReport KEY_MEDIA_WWW_HOME = {128, 0};
const MediaKeyReport KEY_MEDIA_LOCAL_MACHINE_BROWSER = {0, 1}; // Opens "My Computer" on Windows
const MediaKeyReport KEY_MEDIA_CALCULATOR = {0, 2};
const MediaKeyReport KEY_MEDIA_WWW_BOOKMARKS = {0, 4};
const MediaKeyReport KEY_MEDIA_WWW_SEARCH = {0, 8};
const MediaKeyReport KEY_MEDIA_WWW_STOP = {0, 16};
const MediaKeyReport KEY_MEDIA_WWW_BACK = {0, 32};
const MediaKeyReport KEY_MEDIA_CONSUMER_CONTROL_CONFIGURATION = {0, 64}; // Media Selection
const MediaKeyReport KEY_MEDIA_EMAIL_READER = {0, 128};


// REMAP Output コールバック クラス
class AztoolOutputCallbacks : public BLECharacteristicCallbacks {
  public:
	BLECharacteristic* pInputCharacteristic; // AZTOOL 送信用
    AztoolOutputCallbacks(void);
    void onWrite(BLECharacteristic* me);
	void sendRawData(uint8_t *data, uint8_t data_length); // Remapにデータを返す
};

class BleKeyboardC6 : public Print, public BLEServerCallbacks, public BLECharacteristicCallbacks
{
private:
  BLEHIDDevice* hid;
  BLECharacteristic* inputKeyboard;
  BLECharacteristic* outputKeyboard;
  BLECharacteristic* inputMediaKeys;
  BLECharacteristic* inputMouse;
  BLECharacteristic* inputAztool;
  BLECharacteristic* outputAztool;
  AztoolOutputCallbacks* aztoolCallbacks;
  BLEAdvertising*    advertising;
  KeyReport          _keyReport;
  MediaKeyReport     _mediaKeyReport;
  uint8_t            _MouseButtons; // マウスボタン情報
  std::string        deviceName;
  std::string        deviceManufacturer;
  uint8_t            batteryLevel;
  bool               connected = false;
  uint32_t           _delay_ms = 7;
  void delay_ms(uint64_t ms);

  uint16_t vid       = 0x05ac;
  uint16_t pid       = 0x820a;
  uint16_t version   = 0x0210;

public:

  BleKeyboardC6(std::string deviceName = "ESP32 Keyboard", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  void begin();
  void end(void);
  void sendReport(KeyReport* keys);
  void sendReport(MediaKeyReport* keys);
  size_t press(uint8_t k);
  size_t press(const MediaKeyReport k);
  size_t release(uint8_t k);
  size_t release(const MediaKeyReport k);
  size_t write(uint8_t c);
  size_t write(const MediaKeyReport c);
  size_t write(const uint8_t *buffer, size_t size);
  void releaseAll(void);
  bool isConnected(void);
  void setBatteryLevel(uint8_t level);
  void setName(std::string deviceName);  
  void setDelay(uint32_t ms);

  void set_vendor_id(uint16_t vid);
  void set_product_id(uint16_t pid);
  void set_version(uint16_t version);


    unsigned short modifiers_press(unsigned short k);
    unsigned short modifiers_release(unsigned short k);
    unsigned short modifiers_media_press(unsigned short k);
    unsigned short modifiers_media_release(unsigned short k);

    void mouse_click(uint8_t b);
    void mouse_press(uint8_t b);
    void mouse_release(uint8_t b);
    void mouse_move(signed char x, signed char y, signed char wheel, signed char hWheel);
    size_t press_set(uint8_t k); // 指定したキーだけ押す
    size_t press_raw(unsigned short k);
    size_t release_raw(unsigned short k);
    void setConnInterval(int interval_type);


protected:
  virtual void onStarted(BLEServer *pServer) { };
  virtual void onConnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) ;
  virtual void onDisconnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) ;
  virtual void onWrite(BLECharacteristic* me) ;

};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_KEYBOARD_H
#endif // KEYBOARD_TYPE == 1 // Normal BLE
