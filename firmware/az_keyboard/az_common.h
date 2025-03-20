#include "az_config.h"

#ifndef AzCommon_h
#define AzCommon_h

#include "Arduino.h"
#include "FS.h"
#include "SPIFFS.h"
#include "driver/adc.h"

#include <esp_task_wdt.h>
#include <ArduinoJson.h>

#if WIFI_FLAG == 1
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include "src/lib/HTTPClient_my.h"
#endif

#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h> 
#include <Adafruit_NeoPixel.h>
#include <Adafruit_MCP23X17.h>

#include "src/lib/oled.h"
#include "src/lib/neopixel.h"
#include "src/lib/wirelib.h"

// キーボード
#include "src/lib/setting_json_default.h"


#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "AZM";
#endif

// レイヤー切り替え同時押し許容数
#define PRESS_KEY_MAX 8

// マウス移動ボタン同時押し許容数
#define PRESS_MOUSE_MAX 4

// WEBフック用のバッファサイズ
#define WEBFOOK_BUF_SIZE 512

// JSONバッファにPSRAMを使うかのフラグ
#define SETTING_JSON_BUF_PSRAM 0

// 設定JSONのバッファサイズ
#define SETTING_JSON_BUF_SIZE 51200

// 暗記ボタンで暗記できる数
#define ANKEY_DATA_MAX_LENGTH  32

// Neopixデータ送信周波数(400 or 800)
#define AZ_NEO_KHZ 400

// remap用 デフォルトの vid  pid
#define BLE_HID_VID  0xE502
#define BLE_HID_PID  0x0200

// キースキャンループの待ち時間デフォルト(ms)
#define LOOP_DELAY_DEFAULT  5

// ファームウェアのバージョン文字
#define FIRMWARE_VERSION   "000122"

// EEPROMに保存しているデータのバージョン文字列
#define EEP_DATA_VERSION    "AZC003"

// JSON のファイルパス
#define SETTING_JSON_PATH "/setting.json"

// 起動回数を保存するファイルのパス
#define  BOOT_COUNT_PATH  "/boot_count"

// システム情報を保存するファイルのパス
#define  AZ_SYSTEM_FILE_PATH  "/sys_data"

// 打鍵数を自動保存するかどうかの設定を保存するファイルパス
#define  KEY_COUNT_AUTO_SAVE_PATH  "/key_count_auto_save"

// デバッグモード 0=OFF / 1=ON
#define  AZ_DEBUG_MODE 0

// アクチュエーションタイプデフォルト
#define  ACTUATION_TYPE_DEFAULT  0

// アクチュエーションポイントデフォルト
#define  ACTUATION_POINT_DEFAULT  160

// ラピットトリガーデフォルト
#define  RAPID_TRIGGER_DEFAULT  120

// ホールセンサーのアナログ値読み取り範囲デフォルト
#define  HALL_RANGE_MIN_DEFAULT -50
#define  HALL_RANGE_MAX_DEFAULT 1200

// シリアル通信で入力されたキーのステータス(16ビット×16＝0～255キー)
#define  SERIAL_INPUT_MAX  16


// 今押されているボタンの情報
struct press_key_data {
    short action_type; // キーの動作タイプ 0=設定なし / 1=通常入力 / 2=テキスト入力 / 3=レイヤー変更 / 4=WEBフック
    short layer_id; // キーを押した時のレイヤーID
    short key_num; // キー番号
    short press_type; // 入力タイプ(0=press / 1=sub_press)
    short key_id; // 送信した文字
    short press_time; // キーを押してからどれくらい経ったか
    short unpress_time; // キーを離してからどれくらい経ったか
    short repeat_interval; // 連打の間隔
    short repeat_index; // 現在の連打カウント
};


// 今押されているマウスボタン情報
struct press_mouse_data {
    short key_num; // キー番号
    short action_type; // 動作のタイプ(5=マウス移動 / 10=アナログマウス移動)
    short move_x; // X座標
    short move_y; // Y座標
    short move_wheel; // 縦ホイール
    short move_hWheel; // 横ホイール
    short move_speed; // 移動速度
    short move_index; // 移動index
};

// EEPROMに保存するデータ
struct mrom_data_set {
    char check[10];
    char keyboard_type[16];
    int boot_mode; // 起動モード 0=キーボード / 1=設定モード
    char uid[12];
};

// 通常キー入力
struct setting_normal_input {
    uint8_t key_length;
    uint16_t *key; // 通常入力
    uint16_t hold; // ホールド(長押し)
    short repeat_interval; // 連打設定
};

// マウス移動
struct setting_mouse_move {
    int16_t x;
    int16_t y;
    int16_t wheel;
    int16_t hWheel;
    int16_t speed;
};

// レイヤー切り替え
struct setting_layer_move {
    int8_t layer_id;
    int8_t layer_type;
};

// キーを押した時の設定
struct setting_key_press {
    short layer; // どのレイヤーか
    short key_num; // どのキーか
    uint8_t actuation_type; // アクチュエーションタイプ
    uint8_t actuation_point; // アクチュエーションポイント
    uint8_t rapid_trigger; // ラピットトリガー
    short action_type; // 入力するタイプ
    short data_size; // データのサイズ
    char *data; // 入力データ
    bool sub_press_flag; // サブ入力データの有無
    void *sub_press; // サブ入力用データ
};

// IOエキスパンダオプションの設定
struct ioxp_option {
    uint8_t addr; // IOエキスパンダのアドレス
    uint8_t *row; // row のピン
    uint16_t row_mask; // row output する時に使う全ROWのOR
    uint16_t *row_output; // row output する時のピンにwriteするデータ
    uint8_t row_len;
    uint8_t *col;
    uint8_t col_len;
    uint8_t *direct;
    uint8_t direct_len;
};

// I2Cオプション　マップ設定
struct i2c_map {
    short map_start; // キー設定の番号開始番号
    short *map; // キーとして読み取る番号の配列
    uint8_t map_len; // マッピング設定の数
};

// I2Cオプション　IOエキスパンダ（MCP23017
struct i2c_ioxp {
    ioxp_option *ioxp; // 使用するIOエキスパンダの設定
    uint8_t ioxp_len; // IOエキスパンダ設定の数
};

// I2Cオプション　Tiny202　ロータリエンコーダ
struct i2c_rotary {
    uint8_t *rotary; // ロータリーエンコーダのアドレス
    uint8_t rotary_len; // ロータリーエンコーダの数
};

// I2Cオプション　1Uトラックボール　PIM447
struct i2c_pim447 {
    uint8_t addr; // PIM447のアドレス
    short speed; // マウス移動速度
    uint8_t rotate; // マウスの向き
};

// I2Cオプション AZ-Expander 
struct i2c_azxp {
    uint8_t setting[18]; // AZ-Expanderに送る設定データ
    azxp_key_info key_info; // キー読み込みのバイト数とか
};

// i2cオプションの設定
struct i2c_option {
    uint8_t opt_type; // オプションのタイプ 1: ioエキスパンダキーボード
    uint8_t *data;
    i2c_map *i2cmap;
};

// Nubkey 設定
struct nubkey_option {
    // 基本設定
    uint8_t action_type; // 動作タイプ
    uint8_t up_pin; // ホールセンサー上のピン
    uint8_t down_pin; // ホールセンサー下のピン
    uint8_t left_pin; // ホールセンサー左のピン
    uint8_t right_pin; // ホールセンサー右のピン
    short start_point; // マウス移動が始まる位置
    short tap_time; // タップと判定する時間
    short enable_time; // ONになってからの時間
    // 基準値用
    short read_x_min; // X の最小
    short read_x_max; // X の最大
    short read_y_min; // Y の最小
    short read_y_max; // Y の最大
    short rang_x; // X の中心位置
    short rang_y; // Y の中心位置
    short speed_x; // X の速度調整
    short speed_y; // Y の速度調整
};

// Nubkeyの保存した調整データ
struct nubkey_setting_data {
    short read_x_min; // X の最小
    short read_x_max; // X の最大
    short read_y_min; // Y の最小
    short read_y_max; // Y の最大
};


// WIFI設定
struct setting_wifi {
    char *ssid;
    char *pass;
};

// ArduinoJSON SPRAM用の定義
struct SpiRamAllocator {
    void* allocate(size_t size) {
        if (SETTING_JSON_BUF_PSRAM) {
            return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
        } else {
            return malloc(size);
        }
    }
    void deallocate(void* pointer) {
        if (SETTING_JSON_BUF_PSRAM) {
            heap_caps_free(pointer);
        } else {
            free(pointer);
        }
    }
    void* reallocate(void* ptr, size_t new_size) {
        if (SETTING_JSON_BUF_PSRAM) {
            return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
        } else {
            return realloc(ptr, new_size);
        }
    }
};
using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;

// クラスの定義
class AzCommon
{
    public:
        AzCommon();   // コンストラクタ
        void common_start(); // 共通処理の初期処理(setup時に呼ばれる)
        void esp_restart(); // ESP32再起動
        int split(String data, char delimiter, String *dst); // 区切り文字で分割する
        void set_status_led_timer(); // ステータスLED点滅タイマー登録
        void set_status_rgb_loop(); // RGBステータスループ開始
        void wifi_connect(); // WIFI接続
        String get_wifi_ap_list_json(); // wifiアクセスポイントのリストをJSONで取得
        void get_domain(char *url, char *domain_name); // URLからドメイン名だけ取得

#if WIFI_FLAG == 1
        String send_webhook_simple(char *url); // 単純なGETリクエストのWEBフック
        String send_webhook_post_file(char *url, char *file_path); // POSTでファイルの内容を送信する
        String send_webhook(char *setting_data); // httpかhttpsか判断してリクエストを送信する
        String http_request(char *url, const JsonObject &prm); // httpリクエスト送信
#endif // WIFI_FLAG == 1

        bool create_setting_json(); // デフォルトの設定json作成
        void load_setting_json(); // jsonデータロード
        void clear_keymap(); // キーマップ用に確保しているメモリを解放
        void get_keymap(JsonObject setting_obj); // JSONデータからキーマップの情報を読み込む
        void get_keymap_one(JsonObject json_obj, setting_key_press *press_obj, uint16_t lnum, uint16_t knum); // JSONデータからキーマップの情報を読み込む(1キー分)
        int read_file(char *file_path, String &read_data); // ファイルからデータを読み出す
        int write_file(char *file_path, String &write_data); // ファイルにデータを保存する
        int remove_file(char *file_path); // ファイルを削除する
        int i2c_setup(int p, i2c_option *opt, short map_set); // IOエキスパンダの初期化(戻り値：増えるキーの数)
        void pin_setup(); // キーの入力ピンの初期化
        bool layers_exists(int layer_no); // レイヤーが存在するか確認
        void layer_set(int layer_no); // 現在のレイヤーを指定したレイヤーにする
        setting_key_press get_key_setting(int layer_id, int key_num, short press_type); // 指定したキーの入力設定を取得する
        void load_data(); // EEPROMからデータをロードする
        void save_data(); // EEPROMに保存する
        void load_boot_count(); // 起動回数を取得してカウントアップする
        void load_file_data(char *file_path, uint8_t *load_point, uint16_t load_size); // ファイルから設定値を読み込み
        void save_file_data(char *file_path, uint8_t *save_point, uint16_t save_size); // ファイルに設定値を書込み
        void set_boot_mode(int set_mode); // 起動モードを切り替えてEEPROMに保存
        void change_mode(int set_mode); // モードを切り替え
        int i2c_read(int p, i2c_option *opt, char *read_data); // I2C機器のキー状態を取得
        void serial_read(); // シリアル通信(赤外線)読み込み
        int nubkey_read(int p, nubkey_option *opt, char *read_data); // Nubkeyのキー状態を取得
        void nubkey_position_init(); // Nubkey ポジション設定情報初期化
        void nubkey_position_read(nubkey_option *opt); // Nubkey ポジション設定中動作
        void nubkey_position_set(); // Nubkey ポジション反映
        void key_read(); // 現在のキーの状態を取得
        void key_old_copy(); // 現在のキーの状態を過去用配列にコピー
        uint8_t *input_key_analog; // 今入力中のアナログ値
        char *analog_stroke_most; // 最も押し込んだ時のアナログ値
        char *input_key; // 今入力中のキー(ステータス)
        char *input_key_last; // 最後にチェックした入力中のキー(ステータス)
        short *key_point; // 入力キーに該当する設定が何番目に入っているか
        uint16_t *key_count; // キー別の打鍵した数
        uint16_t key_count_total;
        void delete_all(void); // 全てのファイルを削除
        void delete_indexof_all(String check_str); // 指定した文字から始まるファイルすべて削除
        int spiffs_total(void); // ファイル領域合計サイズを取得
        int spiffs_used(void); // 使用しているファイル領域サイズを取得
        void press_mouse_list_clean(); // マウス移動中リストを空にする
        void press_mouse_list_push(int key_num, short action_type, short move_x, short move_y, short move_wheel, short move_hWheel, short move_speed); // マウス移動中リストに追加
        void press_mouse_list_remove(int key_num); // マウス移動中リストから削除
    
    private:

};

// hid
extern uint16_t hid_vid;
extern uint16_t hid_pid;
extern uint16_t hid_conn_handle; // ペアリングしている機器のハンドルID
extern int8_t hid_power_saving_mode; // 省電力モード 0=通常 / 1=省電力モード
extern int8_t hid_power_saving_state; // インターバルステータス 0=通常 / 1=省電力
extern uint32_t hid_state_change_time; // 最後にステータスを変更した時間
extern uint16_t hid_interval_normal; // 通常時のBLEインターバル
extern uint16_t hid_interval_saving; // 省電力モード時のBLEインターバル
extern int hid_saving_time; // 省電力モードに入るまでの時間(ミリ秒)


// 電源ピン番号
extern int power_pin;

// ステータスピン番号
extern int status_pin;

// ステータスLED今0-9
extern int status_led_bit;

// ステータスLED表示モード
extern volatile int8_t status_led_mode;
extern volatile int8_t status_led_mode_last;

// M5Stamp ステータス RGB_LED ピン、オブジェクト
extern int8_t status_rgb_pin;
extern Adafruit_NeoPixel *status_rgb;

// キーボードのステータス
extern volatile int8_t keyboard_status;

// IOエキスパンダオブジェクト
extern Adafruit_MCP23X17 *ioxp_obj[8];

// 入力用ピン情報
extern short col_len;
extern short row_len;
extern short direct_len;
extern short touch_len;
extern short hall_len;
extern short *col_list;
extern short *row_list;
extern short *direct_list;
extern short *touch_list;
extern short *hall_list;
extern short *hall_offset;

// 入力ピン情報 I2C
extern short ioxp_sda;
extern short ioxp_scl;
extern int ioxp_hz;
extern short ioxp_status[8];
extern int ioxp_hash[8];

// I2Cオプションの設定
extern i2c_option *i2copt;
extern short i2copt_len;

// 入力ピン情報 シリアル通信(赤外線)
extern short seri_tx;
extern short seri_rx;
extern short seri_hz;
extern bool seri_logic;

// シリアル通信（赤外線）入力ステータス
extern uint16_t seri_input[SERIAL_INPUT_MAX];
extern uint8_t seri_cmd;
extern uint8_t seri_buf[12];
extern uint8_t seri_index;
extern uint8_t seri_setting[12];
extern uint8_t seri_up_buf[16];
extern uint16_t seri_setting_del;

// Nubkey 設定
extern nubkey_option *nubopt;
extern short nubopt_len;
extern int8_t nubkey_status;

// 動作電圧チェック用ピン
extern int8_t power_read_pin; // 電圧を読み込むピン

// rgb_led制御用クラス
extern Neopixel rgb_led_cls;

// I2Cライブラリ用クラス
extern Wirelib wirelib_cls;

//timer オブジェクト
extern hw_timer_t *timer;

// WIFI接続オブジェクト

#if WIFI_FLAG == 1
extern WiFiMulti wifiMulti;
#endif

// WIFI接続フラグ
extern int wifi_conn_flag;

// http用のバッファ
extern char webhook_buf[WEBFOOK_BUF_SIZE];

// 入力キーの数
extern int key_input_length;

// キースキャンループの待ち時間
extern short loop_delay;

// キーボードの名前
extern char keyboard_name_str[32];

// キーボードの言語(日本語=0/ US=1 / 日本語(US記号) = 2)
extern uint8_t keyboard_language;

// デフォルトのレイヤー番号と、今選択しているレイヤー番号
extern int default_layer_no;
extern int select_layer_no;
extern int last_select_layer_key; // レイヤーボタン最後に押されたボタン(これが離されたらレイヤーリセット)

// ホールセンサーの範囲
extern short hall_range_min;
extern short hall_range_max;

// holdの設定
extern uint8_t hold_type;
extern uint8_t hold_time;

// 押している最中のキーデータ
extern press_key_data press_key_list[PRESS_KEY_MAX];

// 押している最中のマウス移動
extern press_mouse_data press_mouse_list[PRESS_MOUSE_MAX];

// マウスのスクロールボタンが押されているか
extern bool mouse_scroll_flag;

// aztoolで設定中かどうか
extern uint8_t aztool_mode_flag;

// オールクリア送信フラグ
extern int press_key_all_clear;

// EEPROMデータリンク
extern mrom_data_set eep_data;

// 起動回数
extern uint32_t boot_count;

// 打鍵数を自動保存するかどうか
extern uint8_t key_count_auto_save;

// 共通クラスリンク
extern AzCommon common_cls;

// 設定JSONオブジェクト
extern JsonObject setting_obj;


// remap用 キー入力テスト中フラグ
extern uint16_t  remap_input_test;

// キーが押された時の設定
extern uint16_t setting_length;
extern setting_key_press *setting_press;
// wifi設定
extern uint8_t wifi_data_length;
extern setting_wifi *wifi_data;
// アクセスポイントパスワード
extern char *ap_pass_char;
// RGBLED
extern int8_t rgb_pin;
extern int8_t matrix_row;
extern int8_t matrix_col;
extern int8_t *led_num;
extern int8_t *key_matrix;
extern uint8_t led_num_length;
extern uint8_t key_matrix_length;

// ハッシュ値計算用
int azcrc32(uint8_t* d, int len);


#endif
