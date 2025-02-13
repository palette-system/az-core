#include "../../az_config.h"

#if KEYBOARD_TYPE == 2
// 0x0009 = ESP32 S3

#ifndef USBKeyboard_h
#define USBKeyboard_h

#include <USB.h>
#include <USBHID.h>
#include <HIDTypes.h>

#include "Arduino.h"
#include "hid_common.h"
#include "./ble_callbacks.h"
#include "../../az_common.h"



extern bool usbhid_connected;

extern int step_count; // HIDRAWファイル送信用ステップ数
extern int start_point; // HIDRAWファイル送信用ファイル送信ポイント
extern int raw_len; // HIDRAwファイル送信用バッファサイズ
extern int raw_file_flag; // ファイル送信状態フラグ

static void raw_file_data(void* arg);


class CustomHIDDevice: public USBHIDDevice {
    public:
        KeyReport _keyReport;
        MediaKeyReport _mediaKeyReport;
        uint8_t _MouseButtons; // マウスボタン情報
        CustomHIDDevice(void); // コンストラクタ
        void begin(std::string deviceName = "az_keyboard", std::string deviceManufacturer = "PaletteSystem");
        uint16_t _onGetDescriptor(uint8_t* buffer); // HIDからreport_mapの要求
        void _onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len); // HIDからデータを受け取る
        bool send(uint8_t * value); // キーデータ送信
        bool mouse_send(uint8_t x); // マウスデータ送信

        // void raw_file_data(void* arg); // HIDRaWファイル送信データ送信

        unsigned short modifiers_press(unsigned short k);
        unsigned short modifiers_release(unsigned short k);
        unsigned short modifiers_media_press(unsigned short k);
        unsigned short modifiers_media_release(unsigned short k);
        void sendReport(KeyReport* keys);
        void sendReport(MediaKeyReport* keys);

        bool isConnected(void);
        void mouse_move(signed char x, signed char y, signed char wheel, signed char hWheel);
        void mouse_press(uint8_t b);
        void mouse_release(uint8_t b);
        size_t press_set(uint8_t k); // 指定したキーだけ押す
        size_t press_raw(unsigned short k);
        size_t release_raw(unsigned short k);
        void releaseAll(void);
        void setConnInterval(int interval_type); // 省電力モード設定

};



#endif
#endif
