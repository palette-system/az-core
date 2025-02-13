#ifndef hid_common_h
#define hid_common_h

#include "Arduino.h"

#include <stdint.h>

/* */
#define AZ_HID_VERSION_1_11    (0x0111)

/* HID Class */
#define AZ_HID_CLASS           (3)
#define AZ_HID_SUBCLASS_NONE   (0)
#define AZ_HID_PROTOCOL_NONE   (0)

/* Descriptors */
#define AZ_HID_DESCRIPTOR          (33)
#define AZ_HID_DESCRIPTOR_LENGTH   (0x09)
#define AZ_REPORT_DESCRIPTOR       (34)

/* Class requests */
#define AZ_GET_REPORT (0x1)
#define AZ_GET_IDLE   (0x2)
#define AZ_SET_REPORT (0x9)
#define AZ_SET_IDLE   (0xa)

/* Main items */
#define AZ_HIDINPUT(size)             (0x80 | size)
#define AZ_HIDOUTPUT(size)            (0x90 | size)
#define AZ_FEATURE(size)           (0xb0 | size)
#define AZ_COLLECTION(size)        (0xa0 | size)
#define AZ_END_COLLECTION(size)    (0xc0 | size)

/* Global items */
#define AZ_USAGE_PAGE(size)        (0x04 | size)
#define AZ_LOGICAL_MINIMUM(size)   (0x14 | size)
#define AZ_LOGICAL_MAXIMUM(size)   (0x24 | size)
#define AZ_PHYSICAL_MINIMUM(size)  (0x34 | size)
#define AZ_PHYSICAL_MAXIMUM(size)  (0x44 | size)
#define AZ_UNIT_EXPONENT(size)     (0x54 | size)
#define AZ_UNIT(size)              (0x64 | size)
#define AZ_REPORT_SIZE(size)       (0x74 | size)  //bits
#define AZ_REPORT_ID(size)         (0x84 | size)
#define AZ_REPORT_COUNT(size)      (0x94 | size)  //bytes
#define AZ_PUSH(size)              (0xa4 | size)
#define AZ_POP(size)               (0xb4 | size)

/* Local items */
#define AZ_USAGE(size)                 (0x08 | size)
#define AZ_USAGE_MINIMUM(size)         (0x18 | size)
#define AZ_USAGE_MAXIMUM(size)         (0x28 | size)
#define AZ_DESIGNATOR_INDEX(size)      (0x38 | size)
#define AZ_DESIGNATOR_MINIMUM(size)    (0x48 | size)
#define AZ_DESIGNATOR_MAXIMUM(size)    (0x58 | size)
#define AZ_STRING_INDEX(size)          (0x78 | size)
#define AZ_STRING_MINIMUM(size)        (0x88 | size)
#define AZ_STRING_MAXIMUM(size)        (0x98 | size)
#define AZ_DELIMITER(size)             (0xa8 | size)


enum via_command_id {
    // remap用
    id_get_protocol_version                 = 0x01,  // always 0x01
    id_get_keyboard_value                   = 0x02,
    id_set_keyboard_value                   = 0x03,
    id_dynamic_keymap_get_keycode           = 0x04,
    id_dynamic_keymap_set_keycode           = 0x05,
    id_dynamic_keymap_reset                 = 0x06,
    id_lighting_set_value                   = 0x07,
    id_lighting_get_value                   = 0x08,
    id_lighting_save                        = 0x09,
    id_eeprom_reset                         = 0x0A,
    id_bootloader_jump                      = 0x0B,
    id_dynamic_keymap_macro_get_count       = 0x0C,
    id_dynamic_keymap_macro_get_buffer_size = 0x0D,
    id_dynamic_keymap_macro_get_buffer      = 0x0E,
    id_dynamic_keymap_macro_set_buffer      = 0x0F,
    id_dynamic_keymap_macro_reset           = 0x10,
    id_dynamic_keymap_get_layer_count       = 0x11,
    id_dynamic_keymap_get_buffer            = 0x12,
    id_dynamic_keymap_set_buffer            = 0x13,

    // aztool用
    id_get_file_start                       = 0x30,
    id_get_file_data                        = 0x31,
    id_save_file_start                      = 0x32,
    id_save_file_data                       = 0x33,
    id_save_file_complate                   = 0x34,
    id_remove_file                          = 0x35,
    id_remove_all                           = 0x36,
    id_move_file                            = 0x37,
    id_get_file_list                        = 0x38,
    id_get_disk_info                        = 0x39,
    id_restart                              = 0x3A,
    id_get_ioxp_key                         = 0x3B,
    id_set_mode_flag                        = 0x3C,
    id_get_ap_list                          = 0x3D,
    id_read_key                             = 0x3E,
    id_get_rotary_key                       = 0x3F,
    id_get_pim447                           = 0x40,
    id_set_pin_set                          = 0x41,
    id_i2c_read                             = 0x42,
    id_i2c_write                            = 0x43,
    id_get_analog_switch                    = 0x44,
    id_set_analog_switch                    = 0x45,
    id_get_serial_input                     = 0x46,
    id_get_serial_setting                   = 0x47,

    // ステータス取得
    id_get_firmware_status                  = 0x60,

    // システム用
    id_unhandled                            = 0xFF,
};

enum via_keyboard_value_id {
    id_uptime              = 0x01,  //
    id_layout_options      = 0x02,
    id_switch_matrix_state = 0x03
};

// remapへ返事を返す用のバッファ
extern uint8_t remap_buf[36];

// ファイル送受信用バッファ
extern uint8_t send_buf[36];
extern char target_file_path[36];
extern char second_file_path[36];

// ファイル保存用バッファ
extern uint8_t *save_file_data;
extern int save_file_length;
extern uint8_t save_file_step;
extern uint8_t save_file_index;
extern bool save_step_flag[8];

// remapで設定変更があったかどうかのフラグ
extern uint8_t remap_change_flag;


#define JIS_SHIFT 0x1000
#define MOUSE_CODE 0x2000
#define MOUSE_CODE 0x4000

// マウスボタン
/*
#define MOUSE_BUTTON_LEFT  0x01
#define MOUSE_BUTTON_RIGHT  0x02
#define MOUSE_BUTTON_MIDDLE 0x04
#define MOUSE_BUTTON_BACK   0x08
*/

// 通常キー構造
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

// メディアキー構造
typedef uint8_t MediaKeyReport[2];


// HIDのデバイスID
#define REPORT_KEYBOARD_ID 0x01
#define REPORT_MEDIA_KEYS_ID 0x02
#define REPORT_MOUSE_ID 0x03
#define REPORT_AZTOOL_ID 0x04

#define INPUT_REPORT_RAW_MAX_LEN 32
#define OUTPUT_REPORT_RAW_MAX_LEN 32



// HID レポートデフォルト
const uint8_t _hidReportDescriptorDefault[] PROGMEM = {
  AZ_USAGE_PAGE(1),      0x01,          // USAGE_PAGE (Generic Desktop Ctrls)
  AZ_USAGE(1),           0x06,          // USAGE (Keyboard)
  AZ_COLLECTION(1),      0x01,          // COLLECTION (Application)
  // ------------------------------------------------- Keyboard
  AZ_REPORT_ID(1),       REPORT_KEYBOARD_ID,   //   REPORT_ID (1)
  AZ_USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)

	// モデファイヤキー(修飾キー)
	AZ_USAGE_MINIMUM(1),   0xE0,          //   USAGE_MINIMUM (0xE0)(左CTRLが0xe0)
  AZ_USAGE_MAXIMUM(1),   0xE7,          //   USAGE_MAXIMUM (0xE7)(右GUIが0xe7)
  AZ_LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  AZ_LOGICAL_MAXIMUM(1), 0x01,          //   Logical Maximum (1)
  AZ_REPORT_COUNT(1),    0x08,          //   REPORT_COUNT (8)全部で8つ(左右4つずつ)。
	AZ_REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  AZ_HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)

	// 予約フィールド
	AZ_REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 1 byte (Reserved)
  AZ_REPORT_SIZE(1),     0x08,          //   REPORT_SIZE (8)1ビットが8つ。
  AZ_HIDINPUT(1),        0x01,          //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

	// LED状態のアウトプット
	AZ_REPORT_COUNT(1),    0x05,          //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)全部で5つ。
  AZ_REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)LEDにつき1ビット
  AZ_USAGE_PAGE(1),      0x08,          //   USAGE_PAGE (LEDs)
  AZ_USAGE_MINIMUM(1),   0x01,          //   USAGE_MINIMUM (0x01) ; Num Lock(NumLock LEDが1)
  AZ_USAGE_MAXIMUM(1),   0x05,          //   USAGE_MAXIMUM (0x05) ; Kana(KANA LEDが5)
  AZ_HIDOUTPUT(1),       0x02,          //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)LED report

	// LEDレポートのパディング
	AZ_REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 3 bits (Padding)
  AZ_REPORT_SIZE(1),     0x03,          //   REPORT_SIZE (3)残りの3ビットを埋める。
  AZ_HIDOUTPUT(1),       0x03,          //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)

	// 入力キーのインプット
	AZ_REPORT_COUNT(1),    0x06,          //   REPORT_COUNT (6) ; 6 bytes (Keys)全部で6つ。
  AZ_REPORT_SIZE(1),     0x08,          //   REPORT_SIZE(8)おのおの8ビットで表現
  AZ_LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM(0)キーコードの範囲 開始
  AZ_LOGICAL_MAXIMUM(1), 0x65,          //   LOGICAL_MAXIMUM(0x65) ; 101 keys キーコードの範囲 終了

	AZ_USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  AZ_USAGE_MINIMUM(1),   0x00,          //   USAGE_MINIMUM (0)
  AZ_USAGE_MAXIMUM(1),   0xEF,          //   USAGE_MAXIMUM (0x65)
  AZ_HIDINPUT(1),        0x00,          //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)

	AZ_END_COLLECTION(0),                 // END_COLLECTION
  // ------------------------------------------------- Media Keys
  AZ_USAGE_PAGE(1),      0x0C,          // USAGE_PAGE (Consumer)
  AZ_USAGE(1),           0x01,          // USAGE (Consumer Control)
  AZ_COLLECTION(1),      0x01,          // COLLECTION (Application)
  AZ_REPORT_ID(1),       REPORT_MEDIA_KEYS_ID, //   REPORT_ID (3)
  AZ_USAGE_PAGE(1),      0x0C,          //   USAGE_PAGE (Consumer)
  AZ_LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  AZ_LOGICAL_MAXIMUM(1), 0x01,          //   LOGICAL_MAXIMUM (1)
  AZ_REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  AZ_REPORT_COUNT(1),    0x10,          //   REPORT_COUNT (16)
  AZ_USAGE(1),           0xB8,          //   USAGE (Eject)     ; bit 0: 1
  AZ_USAGE(1),           0xB5,          //   USAGE (Scan Next Track)     ; bit 0: 2
  AZ_USAGE(1),           0xB6,          //   USAGE (Scan Previous Track) ; bit 1: 4
  AZ_USAGE(1),           0xB7,          //   USAGE (Stop)                ; bit 2: 8
  AZ_USAGE(1),           0xCD,          //   USAGE (Play/Pause)          ; bit 3: 16
  AZ_USAGE(1),           0xE2,          //   USAGE (Mute)                ; bit 4: 32
  AZ_USAGE(1),           0xE9,          //   USAGE (Volume Increment)    ; bit 5: 64
  AZ_USAGE(1),           0xEA,          //   USAGE (Volume Decrement)    ; bit 6: 128
  AZ_USAGE(2),           0x94, 0x01,    //   Usage (My Computer) ; bit 0: 1
  AZ_USAGE(2),           0x92, 0x01,    //   Usage (Calculator)  ; bit 1: 2
  AZ_USAGE(2),           0x2A, 0x02,    //   Usage (WWW fav)     ; bit 2: 4
  AZ_USAGE(2),           0x21, 0x02,    //   Usage (WWW search)  ; bit 3: 8
  AZ_USAGE(2),           0x26, 0x02,    //   Usage (WWW stop)    ; bit 4: 16
  AZ_USAGE(2),           0x24, 0x02,    //   Usage (WWW back)    ; bit 5: 32
  AZ_USAGE(2),           0x83, 0x01,    //   Usage (Media sel)   ; bit 6: 64
  AZ_USAGE(2),           0x8A, 0x01,    //   Usage (Mail)        ; bit 7: 128
  AZ_HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  AZ_END_COLLECTION(0),                 // END_COLLECTION

  // ------------------------------------------------- Mouse
  AZ_USAGE_PAGE(1),       0x01, // USAGE_PAGE (Generic Desktop)
  AZ_USAGE(1),            0x02, // USAGE (Mouse)
  AZ_COLLECTION(1),       0x01, // COLLECTION (Application)
  AZ_USAGE(1),            0x01, //   USAGE (Pointer)
  AZ_COLLECTION(1),       0x00, //   COLLECTION (Physical)
  AZ_REPORT_ID(1),        REPORT_MOUSE_ID, //     REPORT_ID (1)
  // ------------------------------------------------- Buttons (Left, Right, Middle, Back, Forward)
  AZ_USAGE_PAGE(1),       0x09, //     USAGE_PAGE (Button)
  AZ_USAGE_MINIMUM(1),    0x01, //     USAGE_MINIMUM (Button 1)
  AZ_USAGE_MAXIMUM(1),    0x05, //     USAGE_MAXIMUM (Button 5)
  AZ_LOGICAL_MINIMUM(1),  0x00, //     LOGICAL_MINIMUM (0)
  AZ_LOGICAL_MAXIMUM(1),  0x01, //     LOGICAL_MAXIMUM (1)
  AZ_REPORT_SIZE(1),      0x01, //     REPORT_SIZE (1)
  AZ_REPORT_COUNT(1),     0x05, //     REPORT_COUNT (5)
  AZ_HIDINPUT(1),         0x02, //     INPUT (Data, Variable, Absolute) ;5 button bits
  // ------------------------------------------------- Padding
  AZ_REPORT_SIZE(1),      0x03, //     REPORT_SIZE (3)
  AZ_REPORT_COUNT(1),     0x01, //     REPORT_COUNT (1)
  AZ_HIDINPUT(1),         0x03, //     INPUT (Constant, Variable, Absolute) ;3 bit padding
  // ------------------------------------------------- X/Y position, Wheel
  AZ_USAGE_PAGE(1),       0x01, //     USAGE_PAGE (Generic Desktop)
  AZ_USAGE(1),            0x30, //     USAGE (X)
  AZ_USAGE(1),            0x31, //     USAGE (Y)
  AZ_USAGE(1),            0x38, //     USAGE (Wheel)
  AZ_LOGICAL_MINIMUM(1),  0x81, //     LOGICAL_MINIMUM (-127)
  AZ_LOGICAL_MAXIMUM(1),  0x7f, //     LOGICAL_MAXIMUM (127)
  AZ_REPORT_SIZE(1),      0x08, //     REPORT_SIZE (8)
  AZ_REPORT_COUNT(1),     0x03, //     REPORT_COUNT (3)
  AZ_HIDINPUT(1),         0x06, //     INPUT (Data, Variable, Relative) ;3 bytes (X,Y,Wheel)
  // ------------------------------------------------- Horizontal wheel
  AZ_USAGE_PAGE(1),       0x0c, //     USAGE PAGE (Consumer Devices)
  AZ_USAGE(2),      0x38, 0x02, //     USAGE (AC Pan)
  AZ_LOGICAL_MINIMUM(1),  0x81, //     LOGICAL_MINIMUM (-127)
  AZ_LOGICAL_MAXIMUM(1),  0x7f, //     LOGICAL_MAXIMUM (127)
  AZ_REPORT_SIZE(1),      0x08, //     REPORT_SIZE (8)
  AZ_REPORT_COUNT(1),     0x01, //     REPORT_COUNT (1)
  AZ_HIDINPUT(1),         0x06, //     INPUT (Data, Var, Rel)
  AZ_END_COLLECTION(0),         //   END_COLLECTION
  AZ_END_COLLECTION(0)          // END_COLLECTION

  // ------------------------------------------------- remap
        ,
        0x06, 0x60, 0xFF,
        0x09, 0x61,
        0xa1, 0x01,
        0x85, REPORT_AZTOOL_ID,
        
        0x09, 0x62, 
        0x15, 0x00, 
        0x26, 0xFF, 0x00, 
        0x95, INPUT_REPORT_RAW_MAX_LEN,
        0x75, 0x08, 
        0x81, 0x06, 
      
        0x09, 0x63, 
        0x15, 0x00, 
        0x26, 0xFF, 0x00, 
        0x95, OUTPUT_REPORT_RAW_MAX_LEN, //REPORT_COUNT(32)
        0x75, 0x08, //REPORT_SIZE(8)
        0x91, 0x83, 
        0xC0             // End Collection (Application)
};


const unsigned short _asciimap[] PROGMEM =
{
  // 0x00
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x28,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,

  // 0x10
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,

  // 0x20
  0x2c, // ' '
  0x1e | JIS_SHIFT, // !
  0x1f | JIS_SHIFT, // "
  0x20 | JIS_SHIFT, // #
  0x21 | JIS_SHIFT, // $
  0x22 | JIS_SHIFT, // %
  0x23 | JIS_SHIFT, // &
  0x24 | JIS_SHIFT, // '
  0x25 | JIS_SHIFT, // (
  0x26 | JIS_SHIFT, // )
  0x34 | JIS_SHIFT, // *
  0x33 | JIS_SHIFT, // +
  0x36, // ,
  0x2d, // -
  0x37, // .
  0x38, // /

  // 0x30
  0x27, // 0
  0x1e, // 1
  0x1f, // 2
  0x20, // 3
  0x21, // 4
  0x22, // 5
  0x23, // 6
  0x24, // 7
  0x25, // 8
  0x26, // 9
  0x34, // :
  0x33, // ;
  0x36 | JIS_SHIFT, // <
  0x2d | JIS_SHIFT, // =
  0x37 | JIS_SHIFT, // >
  0x38 | JIS_SHIFT, // ?

  // 0x40
  0x2f, // @
  0x04 | JIS_SHIFT, // A
  0x05 | JIS_SHIFT, // B
  0x06 | JIS_SHIFT, // C
  0x07 | JIS_SHIFT, // D
  0x08 | JIS_SHIFT, // E
  0x09 | JIS_SHIFT, // F
  0x0a | JIS_SHIFT, // G
  0x0b | JIS_SHIFT, // H
  0x0c | JIS_SHIFT, // I
  0x0d | JIS_SHIFT, // J
  0x0e | JIS_SHIFT, // K
  0x0f | JIS_SHIFT, // L
  0x10 | JIS_SHIFT, // M
  0x11 | JIS_SHIFT, // N
  0x12 | JIS_SHIFT, // O
  
  // 0x50
  0x13 | JIS_SHIFT, // P
  0x14 | JIS_SHIFT, // Q
  0x15 | JIS_SHIFT, // R
  0x16 | JIS_SHIFT, // S
  0x17 | JIS_SHIFT, // T
  0x18 | JIS_SHIFT, // U
  0x19 | JIS_SHIFT, // V
  0x1a | JIS_SHIFT, // W
  0x1b | JIS_SHIFT, // X
  0x1c | JIS_SHIFT, // Y
  0x1d | JIS_SHIFT, // Z
  0x30, // [
  0x89, // yen
  0x32, // ]
  0x2e, // ^
  0x87 | JIS_SHIFT, // _

  // 0x60
  0x2f | JIS_SHIFT, // `
  0x04, // a
  0x05, // b
  0x06, // c
  0x07, // d
  0x08, // e
  0x09, // f
  0x0a, // g
  0x0b, // h
  0x0c, // i
  0x0d, // j
  0x0e, // k
  0x0f, // l
  0x10, // m
  0x11, // n
  0x12, // o

  // 0x70
  0x13, // p
  0x14, // q
  0x15, // r
  0x16, // s
  0x17, // t
  0x18, // u
  0x19, // v
  0x1a, // w
  0x1b, // x
  0x1c, // y
  0x1d, // z
  0x30 | JIS_SHIFT, // {
  0x89 | JIS_SHIFT, // |
  0x32 | JIS_SHIFT, // }
  0x2e | JIS_SHIFT, // ~
  
  0       // DEL
};


#endif
