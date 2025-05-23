#include "Arduino.h"
#include "az_keyboard.h"
#include "az_common.h"
#include "src/lib/ankey.h"
#include "src/lib/dakey.h"

#include "esp_bt_main.h"
#include "esp_bt.h"
#include "esp_wifi.h"
#include "driver/rtc_io.h"

#if KEYBOARD_TYPE == 2
// 有線キーボード
#include "src/lib/usb_keyboard.h"
// 有線キーボードクラス
CustomHIDDevice bleKeyboard = CustomHIDDevice();

#elif KEYBOARD_TYPE == 1
// BLEキーボード
#include "src/lib/c6_keyboard_jis.h"
// BLEキーボードクラス
BleKeyboardC6 bleKeyboard = BleKeyboardC6();

#else
// BLEキーボード
#include "src/lib/ble_keyboard_jis.h"
// BLEキーボードクラス
BleKeyboardJIS bleKeyboard = BleKeyboardJIS();
#endif

// 暗記ボタンクラス
Ankey ankeycls = Ankey();

// 打鍵カウントクラス
Dakey dakeycls = Dakey();


// コンストラクタ
AzKeyboard::AzKeyboard() {
}

// キーボード初期化処理
void AzKeyboard::begin_keyboard() {
// bluetoothキーボード開始
#if KEYBOARD_TYPE == 1
    bleKeyboard.set_vendor_id(hid_vid);
    bleKeyboard.set_product_id(hid_pid);
    bleKeyboard.setName(keyboard_name_str);
    bleKeyboard.begin();
#else
    bleKeyboard.begin(keyboard_name_str);
#endif
}

// キーボードとして処理開始
void AzKeyboard::start_keyboard() {
    // ステータスLED wifi接続中
    status_led_mode = 4;

    // Wifi 接続
#if WIFI_FLAG == 1
    ESP_LOGD(LOG_TAG, "mmm: %D %D\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    common_cls.wifi_connect();
    ESP_LOGD(LOG_TAG, "mmm: %D %D\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) );
#else
    esp_wifi_stop();
#endif // #if WIFI_FLAG == 1
    
    // ステータスLED点灯
    status_led_mode = 1;

    // レイヤーをデフォルトに
    common_cls.layer_set(default_layer_no);

    // キーの入力状態初期化
    common_cls.key_read();
    common_cls.key_old_copy();

    // マウス移動リスト初期化
    common_cls.press_mouse_list_clean();

    // バッテリーレベル
    // bleKeyboard.setBatteryLevel(100);

    // 暗記ボタン初期化
    ankeycls.begin(this);

    // 打鍵クラス初期化
    dakeycls.begin();

    press_key_all_clear = -1;

     // setCpuFrequencyMhz(80);

  
}


// 前回のキーのステータスと比較して変更があった物だけ処理を実行する
void AzKeyboard::key_action_exec() {
    if (remap_input_test) {
      // Serial.printf("remap_input_test: %d\n", remap_input_test);
      remap_input_test--;
      return;
    }
    // 暗記データのキー入力中は何もしない
    if (ankeycls.ankey_flag == 2) return;
    // キー入力チェック
    int i;
    for (i=0; i<key_input_length; i++) {
        if (common_cls.input_key_last[i] != common_cls.input_key[i]) {
            if (common_cls.input_key[i] == 1) {
                // キーが押された
                key_down_action(i, 0); // 押された時の動作
                rgb_led_cls.set_led_buf(i, 1); // LED に押したよを送る
                ankeycls.key_down(i); // 暗記クラスに押したよを送る(暗記用)
            } else if (common_cls.input_key[i] == 2) {
                // 2段入力の 1段目が押された
                if (common_cls.input_key_last[i] == 3) {
                    // 前が2段目の入力だった場合は2段目のキーを離す
                    key_up_action(i, 0); // 離された時の動作
                    // 1段目のキーを押す
                    key_down_action(i, 1); // 押された時の動作
                } else {
                    // 前が未入力の場合はそのまま1段目のキーを押す
                    key_down_action(i, 1); // 押された時の動作
                    rgb_led_cls.set_led_buf(i, 1); // LED に押したよを送る
                    ankeycls.key_down(i); // 暗記クラスに押したよを送る(暗記用)
                }
            } else if (common_cls.input_key[i] == 3) {
                // 2段入力の 2段目が押された
                if (common_cls.input_key_last[i] == 2) {
                    // 前が1段目入力だったら1段目を離す
                    key_up_action(i, 1); // 離された時の動作
                    // 2段目のキーを押す
                    key_down_action(i, 0); // 押された時の動作
                } else {
                    // 前が未入力の場合はそのまま2段目を押す
                    key_down_action(i, 0); // 押された時の動作
                    rgb_led_cls.set_led_buf(i, 1); // LED に押したよを送る
                    ankeycls.key_down(i); // 暗記クラスに押したよを送る(暗記用)
                }

            } else {
                // キーは離された
                if (common_cls.input_key_last[i] == 2) {
                    // 前が2段入力の 1段目入力だった場合1段目を離す
                    key_up_action(i, 1); // 離された時の動作
                } else {
                    // それ以外は通常のキーを離す
                    key_up_action(i, 0); // 離された時の動作
                }
                rgb_led_cls.set_led_buf(i, 0); // LED に離したよを送る
                ankeycls.key_up(i); // 暗記クラスに離したよを送る(暗記用)
            }
        }
    }
}

// キー連打処理
void AzKeyboard::key_repeat_exec() {
    int i, k;
    for (i=0; i<PRESS_KEY_MAX; i++) {
        if (press_key_list[i].key_num < 0) continue;
        if (press_key_list[i].action_type == 1 && 
            press_key_list[i].repeat_interval >= 0 &&
            press_key_list[i].repeat_interval <= 50) { // 通常キー入力で連打設定がある
            if (press_key_list[i].repeat_index >= press_key_list[i].repeat_interval) {
                k = press_key_list[i].key_id;
                if (k & MOUSE_CODE) {
                    // マウスボタンだった場合
                    bleKeyboard.mouse_press(k - MOUSE_CODE); // マウスボタンを押す
                    delay(10);
                } else {
                    // キーコードだった場合
                    bleKeyboard.press_raw(k); // キーを押す
                    delay(10);
                }
                press_key_list[i].repeat_index = 0;
            }
            if (press_key_list[i].repeat_index == 0) {
                // 押してスグキーを離す
                if (k & MOUSE_CODE) {
                    // マウスボタンだった場合
                    bleKeyboard.mouse_release(k - MOUSE_CODE); // マウスボタンを離す
                } else {
                    // キーコードだった場合
                    bleKeyboard.release_raw(k); // キーを離す
                }
            }
            press_key_list[i].repeat_index++;
        }
    }
}

// キーを押しましたリストに追加
// 
void AzKeyboard::press_key_list_push(int action_type, int key_num, int key_id, int layer_id, int repeat_interval, short press_type) {
    int i, k = -1;
    // 既にリストに自分がいないかチェック(離してスグ押した時とかに自分がいる可能性がある)
    for (i=0; i<PRESS_KEY_MAX; i++) {
        if (press_key_list[i].key_num == key_num && press_key_list[i].key_id == key_id) {
            k = i;
            break;
        }
    }
    // 空いている所を探す
    if (k < 0) {
        for (i=0; i<PRESS_KEY_MAX; i++) {
            if (press_key_list[i].key_num < 0) {
                k = i;
                break;
            }
        }
    }
    // キーデータを入れる(入れる場所が無ければ何もしない)
    if (k >= 0) {
        press_key_list[k].action_type = action_type;
        press_key_list[k].key_num = key_num;
        press_key_list[k].key_id = key_id;
        press_key_list[k].layer_id = layer_id;
        press_key_list[k].press_type = press_type;
        press_key_list[k].press_time = 0;
        press_key_list[k].unpress_time = 0;
        press_key_list[k].repeat_interval = repeat_interval;
        press_key_list[k].repeat_index = 0;
    }
}


// マウス移動処理
void AzKeyboard::move_mouse_loop() {
    int a, i, k;
    int mx, my;
    int wx, wy;
    for (i=0; i<PRESS_MOUSE_MAX; i++) {
        // 入力無しならば何もしない
        if (press_mouse_list[i].key_num < 0) continue;
        if (press_mouse_list[i].action_type == 5) { // 5.マウス移動
            if (press_mouse_list[i].move_speed == 0 && press_mouse_list[i].move_index == 0) {
                // スピード0なら最初の1回だけ移動
                bleKeyboard.mouse_move(
                    press_mouse_list[i].move_x,
                    press_mouse_list[i].move_y,
                    press_mouse_list[i].move_wheel,
                    press_mouse_list[i].move_hWheel);
            } else {
                // スピードで割った分だけ移動
                mx = ((press_mouse_list[i].move_x * press_mouse_list[i].move_speed) / 100);
                my = ((press_mouse_list[i].move_y * press_mouse_list[i].move_speed) / 100);
                wx = ((press_mouse_list[i].move_wheel * press_mouse_list[i].move_speed) / 100);
                wy = ((press_mouse_list[i].move_hWheel * press_mouse_list[i].move_speed) / 100);
                bleKeyboard.mouse_move(mx, my, wx, wy);
                delay(5);
            }

        } else if (press_mouse_list[i].action_type == 10) { // 10.アナログマウス移動
            // 磁気スイッチの押された分だけ移動
            k = press_mouse_list[i].key_num - direct_len - touch_len; // hall キーのID
            a = common_cls.input_key_analog[k]; // キーの現在のアナログ値(0 - 255)
            mx = press_mouse_list[i].move_x * a / 50;
            my = press_mouse_list[i].move_y * a / 50;
            wx = press_mouse_list[i].move_wheel * a / 100;
            wy = press_mouse_list[i].move_hWheel * a / 100;
            bleKeyboard.mouse_move(mx, my, wx, wy);
            delay(5);
        }
        // index をカウント
        if (press_mouse_list[i].move_index < 1000) press_mouse_list[i].move_index++;
        // トラックボールからの入力は1度送ったらすぐ削除
        if (press_mouse_list[i].key_num == 0x2000) common_cls.press_mouse_list_remove(0x2000);
    }
}

// 消費電力用ループ
void AzKeyboard::power_saving_loop() {
    // 省電力状態に戻す
  // Serial.printf("power_saving_loop: %d, %d, %d\n", hid_power_saving_mode, hid_power_saving_state, hid_state_change_time);
    if (hid_power_saving_mode == 1  // 省電力モードON
         // && aztool_mode_flag == 0 // AZTOOLモード中ではない
         && hid_power_saving_state == 0 // 現在の動作モードが通常モード
         && hid_state_change_time < millis()) { // 省電力モードに入るまでの時間が経過した
        bleKeyboard.setConnInterval(1);
    }

    // 省電力状態を解除する(AZTOOLと通信が始まった時用)
    if (hid_power_saving_state == 2) {
        bleKeyboard.setConnInterval(0);
    }
}


#if WIFI_FLAG == 1

// WEBフックを送信する
void AzKeyboard::send_webhook(char *jstr) {
    if (!wifi_conn_flag) {
        send_string("wifi not connected.");
        return;
    }
    char res_char[1024];
    // httpリクエスト送信
    String r = common_cls.send_webhook(jstr);
    r.toCharArray(res_char, 1024);
    ESP_LOGD(LOG_TAG, "mmm: %D %D\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    ESP_LOGD(LOG_TAG, "http res: %S\n", res_char);
    send_string(res_char);
    ESP_LOGD(LOG_TAG, "mmm: %D %D\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) );
}

#endif // #if WIFI_FLAG == 1


// テキスト入力
void AzKeyboard::send_string(char *send_char) {
    ESP_LOGD(LOG_TAG, "send_string: %S\n", send_char );
    // Bluetooth接続してなければ何もしない
    if (!bleKeyboard.isConnected()) return;
    int i = 0;
    // 全て離す
    bleKeyboard.releaseAll();
    while (send_char[i] > 0) {
        // 指定したキーだけ押す
        bleKeyboard.press_set(send_char[i]);
        delay(50);
        bleKeyboard.releaseAll();
        delay(50);
        i++;
    }
}

void AzKeyboard::hold_press(int hold, int key_num) {
    int k = hold >> 4;
    if (k == 0x06) { // 左モデファイア
        if (hold & 0x01) bleKeyboard.press_raw(0xE0); // 左Ctrl
        if (hold & 0x02) bleKeyboard.press_raw(0xE1); // 左Ctrl
        if (hold & 0x04) bleKeyboard.press_raw(0xE2); // 左Ctrl
        if (hold & 0x08) bleKeyboard.press_raw(0xE3); // 左Ctrl
    } else if (k == 0x07) { // 右モデファイア
        if (hold & 0x01) bleKeyboard.press_raw(0xE4); // 右Ctrl
        if (hold & 0x02) bleKeyboard.press_raw(0xE5); // 右Ctrl
        if (hold & 0x04) bleKeyboard.press_raw(0xE6); // 右Ctrl
        if (hold & 0x08) bleKeyboard.press_raw(0xE7); // 右Ctrl
    } else if (k == 0x04) { // レイヤー
        common_cls.layer_set(hold & 0x0F);
        last_select_layer_key = key_num; // 最後に押されたレイヤーボタン設定
    }
}

// キーが押された時の処理
void AzKeyboard::key_down_action(int key_num, short press_type) {
    int i, m, k, r;
    // 打鍵数カウントアップ
    common_cls.key_count[key_num]++;
    common_cls.key_count_total++;
    // キーの設定取得
    setting_key_press key_set = common_cls.get_key_setting(select_layer_no, key_num, press_type);
    // キーの押された時の設定があるか確認
    if (key_set.layer < 0 || key_set.key_num < 0 || key_set.action_type < 0) {
        // 設定が無ければ何もしない
        return;
    }
    // tap / hold を押している最中だったら tap を無効化する
    tap_key_disable_all();
    // キーが押された時の動作タイプ取得
    int action_type = key_set.action_type;
    // キーボードの接続が無ければ何もしない(レイヤー切り替え、WEBフック以外)
    if (action_type != 3 && action_type != 4 && action_type != 6 && action_type != 7 && action_type != 8 && !bleKeyboard.isConnected()) {
        // 押したよリストに追加だけする
        press_key_list_push(-1, key_num, -1, select_layer_no, -1, press_type);
        return;
    }
    // 動作タイプ別の動作
    if (action_type == 1) {
        // 通常キー入力
        setting_normal_input normal_input;
        memcpy(&normal_input, key_set.data, sizeof(setting_normal_input));
        if (normal_input.hold) {
            m = select_layer_no;
            // hold の場合押した時にhold押したよを送信
            if (hold_type == 1) { // すぐにholdを送信
                hold_press(normal_input.hold, key_num);
            }
            press_key_list_push(9, key_num, normal_input.hold, m, -1, press_type); // アクションタイプは9:holdにする
        } else {
            // hold が無ければ通常のキー入力
            for (i=0; i<normal_input.key_length; i++) {
                if (normal_input.repeat_interval < 0 || normal_input.repeat_interval > 50) {
                    if (normal_input.key[i] == 0x4005) {
                        // マウススクロールボタン
                        mouse_scroll_flag = true;
                    } else if (normal_input.key[i] & MOUSE_CODE) {
                        // マウスボタンだった場合
                        bleKeyboard.mouse_press(normal_input.key[i] - MOUSE_CODE); // マウスボタンを押す
                    } else {
                        // キーコードだった場合
                        bleKeyboard.press_raw(normal_input.key[i]); // キーを押す
                    }
                }
                // キー押したよリストに追加
                press_key_list_push(action_type, key_num, normal_input.key[i], select_layer_no, normal_input.repeat_interval, press_type);
                ESP_LOGD(LOG_TAG, "key press : %D %D\r\n", key_num, normal_input.key[i]);
            }
        }

    } else if (action_type == 2) {
        // 固定テキストの入力
        send_string(key_set.data); // 特定の文章を送る
        // キー押したよリストに追加
        press_key_list_push(action_type, key_num, -1, select_layer_no, -1, press_type);

    } else if (action_type == 3) {
        setting_layer_move layer_move_input;
        // マウス移動リストクリア(移動中にレイヤーが切り替わると移動したままになってしまうので)
        common_cls.press_mouse_list_clean();
        // レイヤーの切り替え
        memcpy(&layer_move_input, key_set.data, sizeof(setting_layer_move));
        // Serial.printf("dw: %d %d %02x %d\n", select_layer_no, key_num, layer_move_input.layer_type, layer_move_input.layer_id);
        m = select_layer_no; // 元のレイヤー番号保持
        common_cls.layer_set(layer_move_input.layer_id);
        last_select_layer_key = key_num; // 最後に押されたレイヤーボタン設定
        if (layer_move_input.layer_type == 0x50 || layer_move_input.layer_type == 0x52) {
            // TO、DF はデフォルトレイヤーも切り替える
            default_layer_no = layer_move_input.layer_id;
        }
        // キー押したよリストに追加
        press_key_list_push(action_type, key_num, -1, m, -1, press_type);
        ESP_LOGD(LOG_TAG, "key press layer : %D %02x %D\r\n", key_num, layer_move_input.layer_type, layer_move_input.layer_id);

    } else if (action_type == 4) {
        // webフック
#if WIFI_FLAG == 1
        send_webhook(key_set.data);
#endif

    } else if (action_type == 5 || action_type == 10) {
        // 5.マウス移動  10.アナログマウス移動
        setting_mouse_move mouse_move_input;
        memcpy(&mouse_move_input, key_set.data, sizeof(setting_mouse_move));
        common_cls.press_mouse_list_push(key_num,
            action_type,
            mouse_move_input.x,
            mouse_move_input.y,
            mouse_move_input.wheel,
            mouse_move_input.hWheel,
            mouse_move_input.speed);
        // キー押したよリストに追加
        press_key_list_push(action_type, key_num, -1, select_layer_no, -1, press_type);

    } else if (action_type == 6) {
        // 暗記ボタン
        ankeycls.ankey_down(select_layer_no, key_num);
        // キー押したよリストに追加
        press_key_list_push(action_type, key_num, -1, select_layer_no, -1, press_type);

    } else if (action_type == 7 && ankeycls.ankey_flag == 0) {
        // LED設定ボタン(暗記処理中は動作無視)
        m = *key_set.data;
        if (m == 0) {
            // ON / OFF
            rgb_led_cls.setting_status();
        } else if (m == 1) {
            // 明るさアップ
            rgb_led_cls.setting_bright_up();
        } else if (m == 2) {
            // 明るさダウン
            rgb_led_cls.setting_bright_down();
        } else if (m == 3) {
            // 色変更
            rgb_led_cls.setting_color_type();
        } else if (m == 4) {
            // 光らせ方変更
            rgb_led_cls.setting_shine_type();
        }

    } else if (action_type == 8 && ankeycls.ankey_flag == 0) {
        // 打鍵設定ボタン(暗記処理中は動作無視)
        m = *key_set.data;
        if (m == 0) {
            // サーモグラフ表示
        } else if (m == 1) {
            // 打鍵数をファイルに保存
            dakeycls.save_dakey(0);
        } else if (m == 2) {
            // 自動保存設定を変更
            dakeycls.set_auto_save_change();
        } else if (m == 3) {
            // 打鍵数をファイルに保存
            dakeycls.save_dakey(1);
        }

    } else if (action_type == 11) {
        // Nubkey 位置調整ボタン
        nubkey_status = 1; // 位置調整中に変更
        // Nubkey の位置取得を空に
        common_cls.nubkey_position_init();
        // キー押したよリストに追加
        press_key_list_push(action_type, key_num, -1, select_layer_no, -1, press_type);

    }
}

// キーが離された時の処理
void AzKeyboard::key_up_action(int key_num, short press_type) {
    int i, j, k, m, action_type;
    setting_key_press key_set;
    setting_normal_input normal_input;
    for (i=0; i<PRESS_KEY_MAX; i++) {
        if (press_key_list[i].key_num != key_num) continue;
        if (press_key_list[i].press_type != press_type) continue;
        ESP_LOGD(LOG_TAG, "key release action_type: %D - %D - %D\r\n", key_num, i, press_key_list[i].action_type);
        action_type = press_key_list[i].action_type;
        // キーボードの接続が無ければ何もしない(レイヤー切り替え、WEBフック以外)
        if (action_type != 3 && action_type != 4 && action_type != 6 && action_type != 7 && action_type != 8 && !bleKeyboard.isConnected()) {
            // 離したよだけやる、離したよカウンターカウント開始
            press_key_list[i].unpress_time = 1;
            continue;
        }
        if (action_type == 1) {
            // 通常入力
            if (press_key_list[i].key_id == 0x4005) {
                // マウススクロールボタン
                mouse_scroll_flag = false;
            } else if (press_key_list[i].key_id & MOUSE_CODE) {
                // マウスボタンだった場合
                bleKeyboard.mouse_release(press_key_list[i].key_id - MOUSE_CODE); // マウスボタンを離す
            } else {
                // キーコードだった場合
                bleKeyboard.release_raw(press_key_list[i].key_id); // キーを離す
            }
            ESP_LOGD(LOG_TAG, "key release : %D\r\n", press_key_list[i].key_id);
        } else if (action_type == 3) {
            // レイヤー選択
            key_set = common_cls.get_key_setting(press_key_list[i].layer_id, key_num, press_type);
            setting_layer_move layer_move_input;
            memcpy(&layer_move_input, key_set.data, sizeof(setting_layer_move));
            // Serial.printf("up: %d %d %02x %d\n", press_key_list[i].layer_id, key_num, layer_move_input.layer_type, layer_move_input.layer_id);
            if (layer_move_input.layer_type == 0x51 || layer_move_input.layer_type == 0x58) {
                // MO(押している間)で最後に押されたレイヤーボタンならばレイヤーをデフォルトに戻す
                if (last_select_layer_key == key_num) {
                    common_cls.layer_set(default_layer_no);
                    last_select_layer_key = -1;
                }
            }
        } else if (action_type == 5 || action_type == 10) {
            // 5.マウス移動ボタン 10.アナログマウス移動
            common_cls.press_mouse_list_remove(key_num); // 移動中リストから削除
        } else if (action_type == 6) {
            // 暗記ボタン
            ankeycls.ankey_up(press_key_list[i].layer_id, key_num);
        } else if (action_type == 9) {
            // hold ボタン
            // まずは長押し用に押されたボタンを離す
            if (hold_type == 1 || // 押された時点でhold押されるモード
                    (hold_type == 0 && press_key_list[i].press_time >= hold_time)) { // 時間が経ってからhold押されるモードで、hold押された後
                k = press_key_list[i].key_id >> 4;
                m = press_key_list[i].key_id;
                if (k == 0x06) { // 左モデファイア
                    if (m & 0x01) bleKeyboard.release_raw(0xE0); // 左Ctrl
                    if (m & 0x02) bleKeyboard.release_raw(0xE1); // 左Ctrl
                    if (m & 0x04) bleKeyboard.release_raw(0xE2); // 左Ctrl
                    if (m & 0x08) bleKeyboard.release_raw(0xE3); // 左Ctrl
                    delay(20);
                } else if (k == 0x07) { // 右モデファイア
                    if (m & 0x01) bleKeyboard.release_raw(0xE4); // 右Ctrl
                    if (m & 0x02) bleKeyboard.release_raw(0xE5); // 右Ctrl
                    if (m & 0x04) bleKeyboard.release_raw(0xE6); // 右Ctrl
                    if (m & 0x08) bleKeyboard.release_raw(0xE7); // 右Ctrl
                    delay(20);
                } else if (k == 0x04) { // レイヤー
                    // 最後に押されたレイヤーボタンだったらデフォルトに戻す
                    if (last_select_layer_key == key_num) {
                        common_cls.layer_set(default_layer_no);
                        last_select_layer_key = -1;
                    }
                }
            }
            // 押していた時間が短ければ単押しのキーを送信
            if (press_key_list[i].press_time < hold_time) {
                // キーの設定取得
                key_set = common_cls.get_key_setting(press_key_list[i].layer_id, key_num, press_type);
                memcpy(&normal_input, key_set.data, sizeof(setting_normal_input));
                // 設定されている単押しキーを押す
                for (j=0; j<normal_input.key_length; j++) {
                    if (normal_input.key[j] & MOUSE_CODE) {
                        // マウスボタンだった場合
                        bleKeyboard.mouse_press(normal_input.key[j] - MOUSE_CODE); // マウスボタンを押す
                        delay(20);
                        bleKeyboard.mouse_release(normal_input.key[j] - MOUSE_CODE); // マウスボタンを離す
                    } else {
                        // キーコードだった場合
                        bleKeyboard.press_raw(normal_input.key[j]); // キーを押す
                        delay(20);
                        bleKeyboard.release_raw(normal_input.key[j]); // キーを離す
                    }
                }
            }

        } else if (action_type == 11) {
            // Nubkey 位置調整ボタン
            nubkey_status = 0; // 動作中に戻す
            // Nubkey の位置調整を反映
            common_cls.nubkey_position_set();
            

        }
        // スグクリアしない。離したよカウンターカウント開始
        press_key_list[i].unpress_time = 1;
    }
}

// ボタンが押された時 tap / hold 押してる最中だった時の処理
void AzKeyboard::tap_key_disable_all() {
    int i;
    for (i=0; i<PRESS_KEY_MAX; i++) {
        if (press_key_list[i].key_num < 0) continue; // データが無い分は無視
        if (press_key_list[i].action_type != 9) continue; // hold 以外は無視
        if (press_key_list[i].unpress_time > 0) continue; // もう離されてるキーは無視
        if (hold_type == 1) { // すぐに hold を送信する設定
            press_key_list[i].press_time = hold_time + 1; // 押してる時間をtap時間より長くして、離した時にtapが送られないようにする
        } else if (hold_type == 0) { // hold_time 後に hold を送信の設定
            if (press_key_list[i].press_time < hold_time) { // まだ hold を送信してない場合は先にholdを送る
                press_key_list[i].press_time = hold_time + 1; // 押してる時間をtap時間より長くして、離した時にtapが送られないようにする
                hold_press(press_key_list[i].key_id , press_key_list[i].key_num); // hold 押す
            }
        }
    }
}

// 押されているキーの情報を全てリセットする
void AzKeyboard::press_data_reset() {
    int i;
    common_cls.layer_set(default_layer_no);
    last_select_layer_key = -1;
    for (i=0; i<PRESS_KEY_MAX; i++) {
        press_key_list[i].action_type = -1;
        press_key_list[i].key_num = -1;
        press_key_list[i].key_id = -1;
        press_key_list[i].layer_id = -1;
        press_key_list[i].press_type = -1;
        press_key_list[i].press_time = 0;
        press_key_list[i].unpress_time = -1;
        press_key_list[i].repeat_interval = -1;
        press_key_list[i].repeat_index = -1;
    }
    for (i=0; i<key_input_length; i++) {
        common_cls.input_key_last[i] = 0;
        common_cls.input_key[i] = 0;
    }
    bleKeyboard.releaseAll();
    press_key_all_clear = -1;
    common_cls.press_mouse_list_clean();
}

// 押されたキーの情報クリア
void AzKeyboard::press_data_clear() {
    int i;
    int press_count_start = 0, press_count_end = 0;
    for (i=0; i<PRESS_KEY_MAX; i++) {
        if (press_key_list[i].key_num < 0) continue; // データが無い分は無視
        // 処理前の押されたままキー数取得
        if ((press_key_list[i].action_type == 1 && !(press_key_list[i].key_id & MOUSE_CODE)) || press_key_list[i].action_type == 9 ) {
            press_count_start++;
        }
        // 押してる時間カウントアップ
        if (press_key_list[i].press_time < 0x7FFF) press_key_list[i].press_time++;
    }
    // 離したよカウントを加算していき閾値を超えたらクリア
    for (i=0; i<PRESS_KEY_MAX; i++) {
        if (press_key_list[i].key_num < 0 || press_key_list[i].unpress_time < 1) continue;
        press_key_list[i].unpress_time++;
        if (press_key_list[i].unpress_time > 10) {
            if (press_key_list[i].action_type == 1) {
                // 通常入力(キー離したよをもう一回送る)
                bleKeyboard.release_raw(press_key_list[i].key_id); // キーを離す
                ESP_LOGD(LOG_TAG, "key release : %D\r\n", press_key_list[i].key_id);
            }
            // 押したよデータクリア
            press_key_list[i].action_type = -1;
            press_key_list[i].key_num = -1;
            press_key_list[i].key_id = -1;
            press_key_list[i].layer_id = -1;
            press_key_list[i].press_type = -1;
            press_key_list[i].press_time = -1;
            press_key_list[i].unpress_time = -1;
            press_key_list[i].repeat_interval = -1;
            press_key_list[i].repeat_index = -1;
        }
    }
    // tap / hold の hold 時間に達成したキーがあれば hold を送信
    if (hold_type == 0) { // hold_time になったら holdを送信する設定
        for (i=0; i<PRESS_KEY_MAX; i++) {
            if (press_key_list[i].action_type != 9) continue; // tap / hold 以外は無視
            if (press_key_list[i].unpress_time > 0) continue; // 既に離されていれば無視
            if (press_key_list[i].press_time == hold_time) { // hold_time になったら hold を押す
                hold_press(press_key_list[i].key_id , press_key_list[i].key_num); // hold を押す
                delay(20);
            }
        }
    }
    // 処理後の押されたままキー数取得
    for (i=0; i<PRESS_KEY_MAX; i++) {
        if (press_key_list[i].key_num < 0) continue; // データが無い分は無視
        if ((press_key_list[i].action_type == 1 && !(press_key_list[i].key_id & MOUSE_CODE)) || press_key_list[i].action_type == 9 ) {
            press_count_end++;
        }
    }
    // キーの入力があったならオールクリアカウントリセット
    if (press_count_start > 0) {
        press_key_all_clear = -1;
    }
    // 押してるキー数が0になったらオールクリアカウント開始
    if (press_count_start > 0 && press_count_end == 0) {
        press_key_all_clear = 1;
    }
    // オールクリアカウントが閾値を超えたらオールクリア送信
    if (press_key_all_clear > 0) {
        press_key_all_clear++;
        if (press_key_all_clear > 10) {
            bleKeyboard.releaseAll();
            press_key_all_clear = -1;
        }
    }
    
}

void IRAM_ATTR wake_gpio_isr_handler(void* arg) {
    gpio_intr_disable(GPIO_NUM_16);
  
  }

// 電源スイッチ用ループ
void AzKeyboard::power_sleep_loop() {
    // 電源スイッチが設定されていなければ何もしない
    if (power_pin < 0) return;

    if (digitalRead(power_pin)) {
        // 電源スイッチがOFFならばスリープに入る
        delay(50);
        // wifiとBLEを止める
        esp_bluedroid_disable();
        esp_bt_controller_disable();
        // esp_wifi_stop();
        /*
        setCpuFrequencyMhz(10);
        status_led_mode = 0;
        while (!digitalRead(power_pin)) {
            delay(100);
        }
        ESP.restart();
        */
        /*
        setCpuFrequencyMhz(80);
        esp_bluedroid_enable();
        esp_bt_controller_enable(ESP_BT_MODE_BLE);
        begin_keyboard();
        start_keyboard();
        bleKeyboard.press_set(6);
        delay(50);
        bleKeyboard.releaseAll();
        delay(50);
        */
       delay(100);
       esp_light_sleep_start();
       ESP.restart();

       // esp_bluedroid_enable();
       // esp_bt_controller_enable(ESP_BT_MODE_BLE);
       // delay(100);

    }

}

// 定期実行の処理
void AzKeyboard::loop_exec(void) {


    // 現在のキーの状態を前回部分にコピー
    common_cls.key_old_copy();

    // 現在のキーの状態を取得
    common_cls.key_read();

    // キー入力のアクション実行
    key_action_exec();

    // 暗記ボタン定期処理
    ankeycls.loop_exec();

    // キー連打処理
    key_repeat_exec();

    // マウス移動処理
    move_mouse_loop();

    // キー入力クリア処理
    press_data_clear();

    // 打鍵数定期処理(自動保存など)
    dakeycls.loop_exec();

    // RGB_LEDを制御する定期処理
    rgb_led_cls.rgb_led_loop_exec();

    // 省電力モード用ループ処理
    power_saving_loop();

    // 電源スイッチ用ループ処理
    power_sleep_loop();

    // eztoolツールI2Cオプション設定中はループ処理をしない(I2Cの読み込みが走っちゃうと落ちるから)
    while (aztool_mode_flag == 1) {
        delay(100);
    }

    // aztool入力テスト中はキー入力の読み込みだけループ
    while (aztool_mode_flag == 2) {
        // 現在のキーの状態を取得
        common_cls.key_read();
        // 現在のキーの状態を前回部分にコピー
        common_cls.key_old_copy();
        delay(5);
    }

    // キーボードリスタート要求を受け取った
    if (aztool_mode_flag == 3) {
        common_cls.esp_restart(); // ESP32 再起動
    }

    delay(5);

}
