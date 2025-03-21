#include "az_common.h"
#include "az_keyboard.h"


// 共通クラス
AzCommon common_cls = AzCommon();

// キーボードモードクラス
AzKeyboard azkb = AzKeyboard();

void setup() {
    int i, j, s;
    // 共通処理の初期化
    common_cls.common_start();
    // 設定jsonの読み込み
    common_cls.load_setting_json();
    // キーボード初期処理
    azkb.begin_keyboard();
    // 電源ピンの設定
    if (power_pin >= 0) {
        pinMode(power_pin, INPUT_PULLUP); // ピンの設定
        gpio_wakeup_enable((gpio_num_t)power_pin, GPIO_INTR_LOW_LEVEL); // スリープから復帰する条件設定
        esp_sleep_enable_gpio_wakeup(); // ピンのIOで復帰をONにする
    }
    // ステータス表示用のLED初期化
    if (status_pin >= 0) {
        pinMode(status_pin, OUTPUT);
        digitalWrite(status_pin, 1);
        status_led_mode = 0;
        common_cls.set_status_led_timer();
    }
    // ステータスRGBLEDのクラス初期化
    if (status_rgb_pin >= 0) {
        status_rgb =  new Adafruit_NeoPixel(1, status_rgb_pin, NEO_GRB + NEO_KHZ400);
        status_led_mode = 0;
        common_cls.set_status_rgb_loop();
    }
    // RGB_LEDクラス初期化
    ESP_LOGD(LOG_TAG, "rgb_led_cls.begin %D %D %D", rgb_pin, matrix_row, matrix_col);
    if (rgb_pin > 0 && matrix_row > 0 && matrix_col > 0) {
        rgb_led_cls.begin( rgb_pin, matrix_row, matrix_col, &select_layer_no, led_num, key_matrix, AZ_NEO_KHZ);
    }
    // キーの入力ピンの初期化
    common_cls.pin_setup();
    // 起動回数を読み込み
    common_cls.load_boot_count();
    // 打鍵数を自動保存するかどうかの設定を読み込み
    key_count_auto_save = 0;
    common_cls.load_file_data(KEY_COUNT_AUTO_SAVE_PATH, (uint8_t *)&key_count_auto_save, 1);
    // eepromからデータ読み込み
    common_cls.load_data();
    ESP_LOGD(LOG_TAG, "boot_mode = %D\r\n", eep_data.boot_mode);
    // キーボードとして起動
    azkb.start_keyboard();
}


// ループ処理本体
void loop() {
    // キーボードモード用ループ
    azkb.loop_exec();
}
