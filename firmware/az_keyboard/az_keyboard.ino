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
Serial.printf("a");
    // 設定jsonの読み込み
    common_cls.load_setting_json();
Serial.printf("b");
    // ステータス表示用のLED初期化
    if (status_pin >= 0) {
        pinMode(status_pin, OUTPUT);
        digitalWrite(status_pin, 0);
        status_led_mode = 0;
        common_cls.set_status_led_timer();
    }
Serial.printf("c");
    // ステータスRGBLEDのクラス初期化
    if (status_rgb_pin >= 0) {
        status_rgb =  new Adafruit_NeoPixel(1, status_rgb_pin, NEO_GRB + NEO_KHZ400);
        status_led_mode = 0;
        common_cls.set_status_rgb_loop();
    }
Serial.printf("d");
    // RGB_LEDクラス初期化
    ESP_LOGD(LOG_TAG, "rgb_led_cls.begin %D %D %D", rgb_pin, matrix_row, matrix_col);
    if (rgb_pin > 0 && matrix_row > 0 && matrix_col > 0) {
        rgb_led_cls.begin( rgb_pin, matrix_row, matrix_col, &select_layer_no, led_num, key_matrix, AZ_NEO_KHZ);
    }
Serial.printf("e");
    // キーの入力ピンの初期化
    common_cls.pin_setup();
Serial.printf("f");
    // 起動回数を読み込み
    common_cls.load_boot_count();
Serial.printf("g");
    // 打鍵数を自動保存するかどうかの設定を読み込み
    key_count_auto_save = 0;
    common_cls.load_file_data(KEY_COUNT_AUTO_SAVE_PATH, (uint8_t *)&key_count_auto_save, 1);
Serial.printf("h");
    // eepromからデータ読み込み
    common_cls.load_data();
Serial.printf("i");
    ESP_LOGD(LOG_TAG, "boot_mode = %D\r\n", eep_data.boot_mode);
    // キーボードとして起動
    azkb.start_keyboard();
}


// ループ処理本体
void loop() {
    // キーボードモード用ループ
    azkb.loop_exec();
}
