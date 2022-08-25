#include "az_common.h"
#include "src/lib/cnc_table.h"


// remap用 キー入力テスト中フラグ
uint16_t  remap_input_test;

// キーが押された時の設定
uint16_t setting_length;
setting_key_press *setting_press;
// wifi設定
uint8_t wifi_data_length;
setting_wifi *wifi_data;
// アクセスポイントパスワード
char *ap_pass_char;
// RGBLED
int8_t rgb_pin;
int8_t matrix_row;
int8_t matrix_col;
int8_t *led_num;
int8_t *key_matrix;
uint8_t led_num_length;
uint8_t key_matrix_length;


// hid
uint16_t hid_vid;
uint16_t hid_pid;

// ステータス表示用ピン番号
int status_pin = -1;

// ステータスLED今0-9
int status_led_bit = 0;

// ステータスLED表示モード
int8_t status_led_mode;
int8_t status_led_mode_last;

// M5Stamp ステータス RGB_LED ピン、オブジェクト
int8_t status_rgb_pin;
Adafruit_NeoPixel *status_rgb;

// キーボードのステータス
int8_t keyboard_status;

// rgb_led制御用クラス
Neopixel rgb_led_cls = Neopixel();

// I2Cライブラリ用クラス
Wirelib wirelib_cls = Wirelib();

//timer オブジェクト
hw_timer_t *timer = NULL;

// WIFI接続オブジェクト
WiFiMulti wifiMulti;

// WIFI接続フラグ
int wifi_conn_flag;

// http用のバッファ
char webhook_buf[WEBFOOK_BUF_SIZE];

// 入力キーの数
int key_input_length;

// キーボードの名前
char keyboard_name_str[32];

// キーボードの言語(日本語=0/ US=1 / 日本語(US記号) = 2)
uint8_t keyboard_language;

// デフォルトのレイヤー番号と、今選択しているレイヤー番号と、最後に押されたレイヤーボタン
int default_layer_no;
int select_layer_no;
int last_select_layer_key;


// 押している最中のキーデータ
press_key_data press_key_list[PRESS_KEY_MAX];

// 押している最中のマウス移動
press_mouse_data press_mouse_list[PRESS_MOUSE_MAX];

// マウスのスクロールボタンが押されているか
bool mouse_scroll_flag;

// aztoolで設定中かどうか
uint8_t aztool_mode_flag;

// オールクリア送信フラグ
int press_key_all_clear;

// eepromのデータ
mrom_data_set eep_data;

// 起動回数
uint32_t boot_count;

// 打鍵数を自動保存するかどうか
uint8_t key_count_auto_save;

// IOエキスパンダオブジェクト
Adafruit_MCP23X17 *ioxp_obj[8];

// 入力用ピン情報
short col_len;
short row_len;
short direct_len;
short touch_len;
short *col_list;
short *row_list;
short *direct_list;
short *touch_list;

short ioxp_len;
short *ioxp_list;
short ioxp_sda;
short ioxp_scl;
int ioxp_hz;
short ioxp_status[8];
int ioxp_hash[8];

// I2Cオプションの設定
i2c_option *i2copt;
short i2copt_len;

// 動作電圧チェック用ピン
int8_t power_read_pin; // 電圧を読み込むピン


// ステータス用LED点滅
void IRAM_ATTR status_led_write() {
    int set_bit;
    status_led_bit++;
    if (status_led_bit >= 20) status_led_bit = 0;
    if (status_led_mode == 0) {
        set_bit = 0; // 消灯

    } else if (status_led_mode == 1) {
        set_bit = 1; // 点灯
      
    } else if (status_led_mode == 2) {
        // 設定モード
        if (status_led_bit < 5 || (status_led_bit >= 10 && status_led_bit < 15)) {
            set_bit = 1;
        } else {
            set_bit = 0;
        }
      
    } else if (status_led_mode == 3) {
        // ファームウェア更新中
        if (status_led_bit % 2) {
            set_bit = 1;
        } else {
            set_bit = 0;
        }

    } else if (status_led_mode == 4) {
        // wifi接続中
        if (status_led_bit == 0 || status_led_bit == 2 || status_led_bit == 10 || status_led_bit == 12) {
            set_bit = 1;
        } else {
            set_bit = 0;
        }
    }
    if (status_pin >= 0) digitalWrite(status_pin, set_bit);
}

static void status_rgb_loop(void* arg) {
    while (true) {
        if (status_led_mode == status_led_mode_last) {
            vTaskDelay(100);
            continue;
        }
        if (status_led_mode == 0) {
            status_rgb->setPixelColor(0, status_rgb->Color(0, 0, 0));
        } else if (status_led_mode == 1) {
            status_rgb->setPixelColor(0, status_rgb->Color(0, 10, 0));
        } else if (status_led_mode == 2) {
            status_rgb->setPixelColor(0, status_rgb->Color(0, 0, 10));
        } else if (status_led_mode == 3) {
            status_rgb->setPixelColor(0, status_rgb->Color(0, 10, 10));
        } else if (status_led_mode == 4) {
            status_rgb->setPixelColor(0, status_rgb->Color(0, 0, 10));
        }
        status_rgb->show();
        status_led_mode_last = status_led_mode;
        vTaskDelay(100);
    }
}

// ランダムな文字生成(1文字)
char getRandomCharLower(void) {
    const char CHARS[] = "abcdefghijklmnopqrstuvwxyz";
    int index = random(0, (strlen(CHARS) - 1));
    char c = CHARS[index];
    return c;
}

// ランダムな文字生成(1文字)
char getRandomNumber(void) {
    const char CHARS[] = "0123456789";
    int index = random(0, (strlen(CHARS) - 1));
    char c = CHARS[index];
    return c;
}

// ランダムな文字列生成(文字数指定)
void getRandomCharsLower(int le, char *cbuf) {
    int i;
    for(i=0; i<le; i++){
        cbuf[i]  = getRandomCharLower();
    }
    cbuf[i] = '\0';
}

// ランダムな文字列生成(数字)
void getRandomNumbers(int le, char *cbuf) {
    int i;
    for(i=0; i<le; i++){
        cbuf[i]  = getRandomNumber();
    }
    cbuf[i] = '\0';
}

// crc32のハッシュ値を計算
int azcrc32(uint8_t* d, int len) {
	int i;
    uint32_t r = 0 ^ (-1);
    for (i=0; i<len; i++) {
        r = (r >> 8) ^ crc_table_crc32[(r ^ d[i]) & 0xFF];
    }
    return (r ^ (-1));
};

// コンストラクタ
AzCommon::AzCommon() {
}

// 共通処理の初期化
void AzCommon::common_start() {
    // 乱数初期化
    randomSeed(millis());
    // ファイルシステム初期化
    if(!SPIFFS.begin(true)){
        ESP_LOGD(LOG_TAG, "SPIFFS begin error\n");
        return;
    }
    // WIFI 接続フラグ
    wifi_conn_flag = 0;
    // 押している最中のキーデータ初期化
    int i;
    for (i=0; i<PRESS_KEY_MAX; i++) {
        press_key_list[i].action_type = -1;
        press_key_list[i].key_num = -1;
        press_key_list[i].key_id = -1;
        press_key_list[i].layer_id = -1;
        press_key_list[i].unpress_time = -1;
    }
    // ioエキスパンダピン
    ioxp_sda = -1;
    ioxp_scl = -1;
    ioxp_hz = 400000;
    // ioエキスパンダフラグ
    for (i=0; i<8; i++) {
      ioxp_status[i] = -1;
      ioxp_hash[i] = 0;
    }
    // マウスのスクロールボタンが押されているか
    mouse_scroll_flag = false;
    if (AZ_DEBUG_MODE) Serial.begin(115200);
    // aztoolで作業中かどうか
    aztool_mode_flag = 0;
    // remap用 キー入力テスト中フラグ
    remap_input_test = 0;
    // キーボードのステータス
    keyboard_status = 0;
    // RGBLEDのステータス
    status_led_mode_last = -1;
}


// ステータスLEDチカ用タイマー登録
void AzCommon::set_status_led_timer() {
    timer = timerBegin(0, 80, true); //timer=1us
    timerAttachInterrupt(timer, &status_led_write, true);
    timerAlarmWrite(timer, 100000, true); // 100ms
    timerAlarmEnable(timer);
}

// RGBステータスループ開始
void AzCommon::set_status_rgb_loop() {
    xTaskCreatePinnedToCore(status_rgb_loop, "rgbloop", 2048, NULL, 20, NULL, 1);
}

// WIFI 接続
void AzCommon::wifi_connect() {
    // WIFI 接続
    int i;
    if (wifi_data_length <= 0) {
        ESP_LOGD(LOG_TAG, "wifi : not setting\r\n");
        wifi_conn_flag = 0;
        return;
    }
    // WiFiに接続(一番電波が強いAPへ接続)
    for (i=0; i<wifi_data_length; i++) {
        wifiMulti.addAP(wifi_data[i].ssid, wifi_data[i].pass);
        ESP_LOGD(LOG_TAG, "wifi : [%S] [%S]", wifi_data[i].ssid, wifi_data[i].pass);
    }
    ESP_LOGD(LOG_TAG, "wifi : connect start\r\n");
    i = 0;
    wifi_conn_flag = 1;
    // while (WiFi.status() != WL_CONNECTED) {
    while (wifiMulti.run() != WL_CONNECTED) {
        ESP_LOGD(LOG_TAG, "wifi : try %D\r\n", i);
        delay(1000);
        i++;
        if (i > 20) {
            ESP_LOGD(LOG_TAG, "wifi : connect error\r\n");
            wifi_conn_flag = 0;
            break;
        }
    }
    if (wifi_conn_flag) {
        ESP_LOGD(LOG_TAG, "wifi : connect OK!\r\n");
    }
}

// wifiアクセスポイントのリストをJSONで取得
String AzCommon::get_wifi_ap_list_json() {
    String res = "{\"list\": [";
    int ssid_num;
    String auth_open;
    ssid_num = WiFi.scanNetworks();
    if (ssid_num == 0) {
        ESP_LOGD(LOG_TAG, "get_wifi_ap_list: no networks");
    } else {
        ESP_LOGD(LOG_TAG, "get_wifi_ap_list: %d\r\n", ssid_num);
        for (int i = 0; i < ssid_num; ++i) {
            auth_open = ((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)? "true": "false");
            if (i > 0) res += ",";
            res += "{\"ssid\": \"" + WiFi.SSID(i) + "\", \"rssi\": \"" + WiFi.RSSI(i) + "\", \"auth_open\": " + auth_open + "}";
            delay(10);
        }
    }
    res += "]}";
    ESP_LOGD(LOG_TAG, "%S", res);
    return res;
}

// URLからドメイン名だけ抜き出す
void AzCommon::get_domain(char *url, char *domain_name) {
    int i = 0, j = 0, s = 0;
    while (url[i] > 0) {
        if (s == 0 && url[i] == '/') {
            s = 1;
        } else if (s == 1 && url[i] == '/') {
            s = 2;
        } else if (s == 2) {
            if (url[i] == '/' || url[i] == ':') {
                s = 3;
                domain_name[j] = 0x00;
                break;
            } else if (url[i] == '@') {
                j = 0;
                domain_name[j] = 0x00;
            } else {
                domain_name[j] = url[i];
                j++;
            }
        } else {
            s = 0;
        }
        i++;
    }
    domain_name[j] = 0x00;
}

// webリクエストを送信する(シンプルなGETリクエストのみ)
String AzCommon::send_webhook_simple(char *url) {
    int status_code;
    String res;
    // httpリクエスト用オブジェクト
    HTTPClient http;
    http.begin(url);
    // GET
    status_code = http.GET();
    ESP_LOGD(LOG_TAG, "http status: %D\r\n", status_code);
    if (status_code == HTTP_CODE_OK) {
        res = http.getString();
    } else {
        res = "";
    }
    http.end();
    return res;
}


// POSTでファイルの内容を送信する
String AzCommon::send_webhook_post_file(char *url, char *file_path) {
    if (!wifi_conn_flag) return "";
    status_led_mode = 3;
    // httpリクエスト用オブジェクト
    int status_code;
    HTTPClient_my http;
    String res_body;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    status_code = http.sendPostFile(file_path);
    if (status_code == HTTP_CODE_OK) {
        res_body = http.getString();
    } else {
        res_body = "";
    }
    http.end();
    return res_body;
}

// webリクエストを送信する
String AzCommon::send_webhook(char *setting_data) {
    ESP_LOGD(LOG_TAG, "send_webhook start: %S %D\r\n", setting_data, strlen(setting_data));
    int n = strlen(setting_data) + 1;
    char jchar[n];
    memcpy(jchar, setting_data, n);
    DynamicJsonDocument setting_doc(n + 512);
    deserializeJson(setting_doc, jchar);
    JsonObject prm = setting_doc.as<JsonObject>();
    String url = prm["url"].as<String>();
    int m = url.length() + 1;
    char url_char[m];
    url += "\0";
    url.toCharArray(url_char, m);
    if (url.startsWith("http://") || url.startsWith("https://")) {
        return http_request(url_char, prm);        
    } else {
        return String("url error");
    }
}


// HTTPリクエストを送信する
String AzCommon::http_request(char *url, const JsonObject &prm) {
    if (!wifi_conn_flag) return "";
    status_led_mode = 3;
    ESP_LOGD(LOG_TAG, "http : %S\r\n", url);
    char hkey[64];
    int i;
    int header_len = prm["header"].size();
    // httpリクエスト用オブジェクト
    HTTPClient http;
    http.begin(url);
    // ヘッダー送信
    for (i=0; i<header_len; i++) {
        String hk = prm["header"][i]["key"].as<String>();
        String hv = prm["header"][i]["value"].as<String>();
        hk.toCharArray(hkey, 64);
        hv.toCharArray(webhook_buf, WEBFOOK_BUF_SIZE);
        http.addHeader(hkey, webhook_buf);
    }
    // POSTデータがあればPOST
    String pd = prm["post"].as<String>();
    int status_code;
    if (pd.length() > 0) {
        // POST
        pd.toCharArray(webhook_buf, WEBFOOK_BUF_SIZE);
        status_code = http.POST(webhook_buf);
    } else {
        // GET
        status_code = http.GET();
    }
    ESP_LOGD(LOG_TAG, "http status: %D\r\n", status_code);
    int ot = prm["keyoutput"].as<signed int>();
    if (status_code == HTTP_CODE_OK) {
        String res_body = http.getString();
        http.end();
        status_led_mode = 1;
        if (ot == 0) {
            return "";
        } else if (ot == 1) {
            return String(status_code);
        } else if (ot == 2) {
            return res_body;
        }
        return "";
    } else {
        http.end();
        status_led_mode = 1;
        if (ot == 1) {
            return String(status_code);
        }
        String res_buf = "status code:" + String(status_code);
        return res_buf;
    }
}



// 区切り文字で分割する
int AzCommon::split(String data, char delimiter, String *dst){
    int index = 0;
    int arraySize = (sizeof(data)/sizeof((data)[0]));  
    int datalength = data.length();
    for (int i = 0; i < datalength; i++) {
        char tmp = data.charAt(i);
        if ( tmp == delimiter ) {
            index++;
            if ( index > (arraySize - 1)) return -1;
        }
        else dst[index] += tmp;
    }
    return (index + 1);
}

// レイヤー名、キー名から番号を抜き出す
int split_num(char *c) {
    // _の文字まで進める
    while (c[0] != 0x5F && c[0] != 0x00) {
        c++;
    }
    if (c[0] == 0x5F) c++;
    return String(c).toInt();
}


// JSONデータを読み込む
void AzCommon::load_setting_json() {
    // セッティングJSONを保持する領域
    SpiRamJsonDocument setting_doc(SETTING_JSON_BUF_SIZE);
    JsonObject setting_obj;

    // ファイルが無い場合はデフォルトファイル作成
    if (!SPIFFS.exists(SETTING_JSON_PATH)) {
        if (!create_setting_json()) return;
    }
    // ファイルオープン
    File json_file = SPIFFS.open(SETTING_JSON_PATH);
    if(!json_file){
        // ファイルが開けなかった場合はデフォルトファイル作り直して再起動
        ESP_LOGD(LOG_TAG, "json file open error\n");
        create_setting_json();
        delay(1000);
        ESP.restart(); // ESP32再起動
        return;
    }
    // 読み込み＆パース
    DeserializationError err = deserializeJson(setting_doc, json_file);
    if (err) {
        ESP_LOGD(LOG_TAG, "load_setting_json deserializeJson error\n");
        create_setting_json();
        delay(1000);
        ESP.restart(); // ESP32再起動
        return;
    }
    json_file.close();
    // オブジェクトを保持
    setting_obj = setting_doc.as<JsonObject>();

    // キーボードタイプは必須なので項目が無ければ設定ファイル作り直し(設定ファイル壊れた時用)
    if (!setting_obj.containsKey("keyboard_type")) {
        ESP_LOGD(LOG_TAG, "json not keyboard_type error\n");
        create_setting_json();
        delay(1000);
        ESP.restart(); // ESP32再起動
        return;
    }

    // キーボードの名前を取得する
    String keynamestr;
    if (setting_obj.containsKey("keyboard_name")) {
        keynamestr = setting_obj["keyboard_name"].as<String>();
        keynamestr.toCharArray(keyboard_name_str, 31);
    } else {
        // 設定が無い場合はデフォルト
        sprintf(keyboard_name_str, "az_keyboard");
    }

    // HID 設定
    String hidstr;
    if (setting_obj.containsKey("vendorId")) {
        hidstr = setting_obj["vendorId"].as<String>();
        hid_vid = (uint16_t) strtol(&hidstr[2], NULL, 16);
    } else {
        hid_vid = BLE_HID_VID;
    }
    if (setting_obj.containsKey("productId")) {
        hidstr = setting_obj["productId"].as<String>();
        hid_pid = (uint16_t) strtol(&hidstr[2], NULL, 16);
    } else {
        hid_pid = BLE_HID_PID;
    }
    // デフォルトのレイヤー番号設定
    default_layer_no = setting_obj["default_layer"].as<signed int>();
    // 今選択してるレイヤーをデフォルトに
    select_layer_no = default_layer_no;
    // 入力ピン情報取得
    int i, j, k, m, n, o, p, r;
    col_len = setting_obj["keyboard_pin"]["col"].size();
    row_len = setting_obj["keyboard_pin"]["row"].size();
    direct_len = setting_obj["keyboard_pin"]["direct"].size();
    touch_len = setting_obj["keyboard_pin"]["touch"].size();
    col_list = new short[col_len];
    for (i=0; i<col_len; i++) {
        col_list[i] = setting_obj["keyboard_pin"]["col"][i].as<signed int>();
    }
    row_list = new short[row_len];
    for (i=0; i<row_len; i++) {
        row_list[i] = setting_obj["keyboard_pin"]["row"][i].as<signed int>();
    }
    direct_list = new short[direct_len];
    for (i=0; i<direct_len; i++) {
        direct_list[i] = setting_obj["keyboard_pin"]["direct"][i].as<signed int>();
    }
    touch_list = new short[touch_len];
    for (i=0; i<touch_len; i++) {
        touch_list[i] = setting_obj["keyboard_pin"]["touch"][i].as<signed int>();
    }

    // 動作電圧チェック用ピン
    power_read_pin = -1;
    if (setting_obj.containsKey("power_read")) {
        power_read_pin = setting_obj["power_read"].as<signed int>();
    }

    // IOエキスパンダピン
    if (setting_obj.containsKey("i2c_set") && setting_obj["i2c_set"].size() == 3) {
        ioxp_sda = setting_obj["i2c_set"][0].as<signed int>();
        ioxp_scl = setting_obj["i2c_set"][1].as<signed int>();
        ioxp_hz = setting_obj["i2c_set"][2].as<signed int>();
    } else {
        ioxp_sda = -1;
        ioxp_scl = -1;
        ioxp_hz = 400000;
    }

    // I2cオプションの設定
    i2c_map i2cmap_obj;
    i2c_ioxp i2cioxp_obj;
    i2c_rotary i2crotary_obj;
    i2c_pim447 i2cpim447_obj;
    int opt_type;
    if (setting_obj.containsKey("i2c_option") && setting_obj["i2c_option"].size()) {
        // 有効になっているオプションの数を数える
        k = setting_obj["i2c_option"].size();
        // Serial.printf("i2c_option size: %D\n", k);
        i2copt_len = 0;
        for (i=0; i<k; i++) {
            // Serial.printf("i2c_option: check %D\n", i);
            if (setting_obj["i2c_option"][i].containsKey("enable") &&
                    setting_obj["i2c_option"][i]["enable"].as<signed int>() == 1) {
                // Serial.printf("i2c_option: enable %D\n", i);
                i2copt_len++;
            }
        }
        // Serial.printf("i2c_option: enable total %D\n", i2copt_len);
        i2copt = new i2c_option[i2copt_len];
        // オプションのデータを取得
        j = 0;
        for (i=0; i<k; i++) {
            // 有効になっていないオプションは無視
            if (!setting_obj["i2c_option"][i].containsKey("enable") ||
                    setting_obj["i2c_option"][i]["enable"].as<signed int>() != 1) continue;
            // オプションのタイプ
            if (setting_obj["i2c_option"][i].containsKey("type")) {
                opt_type = setting_obj["i2c_option"][i]["type"].as<signed int>();
            } else {
                opt_type = 0;
            }
            i2copt[j].opt_type = opt_type & 0xff;
            // Serial.printf("i2c_option: load %D type %D - %D\n", i, opt_type, i2copt[j].opt_type);
            // マッピング情報の読み込み
            if (opt_type == 1 || opt_type == 2 || opt_type == 3) { // 1 = IOエキスパンダ（MCP23017）/ 2 = Tiny202 ロータリーエンコーダ
                // キーマッピング設定
                if (setting_obj["i2c_option"][i].containsKey("map") &&
                        setting_obj["i2c_option"][i]["map"].size() ) {
                    i2cmap_obj.map_len = setting_obj["i2c_option"][i]["map"].size();
                    i2cmap_obj.map = new short[i2cmap_obj.map_len];
                    for (p=0; p<i2cmap_obj.map_len; p++) {
                        i2cmap_obj.map[p] = setting_obj["i2c_option"][i]["map"][p].as<signed int>();
                    }
                } else {
                    i2cmap_obj.map_len = 0;
                }
                // キー設定の開始番号
                if (setting_obj["i2c_option"][i].containsKey("map_start")) {
                    i2cmap_obj.map_start = setting_obj["i2c_option"][i]["map_start"].as<signed int>();
                } else {
                    i2cmap_obj.map_start = 0;
                }
                i2copt[j].i2cmap = new i2c_map;
                memcpy(i2copt[j].i2cmap, &i2cmap_obj, sizeof(i2c_map));
            }
            // オプション別のデータ読み込み
            if (opt_type == 1) { // 1 = IOエキスパンダ（MCP23017）
                // 使用するIOエキスパンダの情報
                if (setting_obj["i2c_option"][i].containsKey("ioxp") && setting_obj["i2c_option"][i]["ioxp"].size()) {
                    i2cioxp_obj.ioxp_len = setting_obj["i2c_option"][i]["ioxp"].size();
                    i2cioxp_obj.ioxp = new ioxp_option[i2cioxp_obj.ioxp_len];
                    for (n=0; n<i2cioxp_obj.ioxp_len; n++) {
                        // IOエキスパンダのアドレス
                        i2cioxp_obj.ioxp[n].addr = setting_obj["i2c_option"][i]["ioxp"][n]["addr"];
                        // row に設定しているピン
                        if (setting_obj["i2c_option"][i]["ioxp"][n].containsKey("row") &&
                                setting_obj["i2c_option"][i]["ioxp"][n]["row"].size() ) {
                            // row に設定されている配列を取得(0～7 以外を無視する)
                            i2cioxp_obj.ioxp[n].row_len = setting_obj["i2c_option"][i]["ioxp"][n]["row"].size();
                            i2cioxp_obj.ioxp[n].row = new uint8_t[i2cioxp_obj.ioxp[n].row_len]; // row のピン番号
                            i2cioxp_obj.ioxp[n].row_output = new uint16_t[i2cioxp_obj.ioxp[n].row_len]; // マトリックスでrow write するデータ
                            i2cioxp_obj.ioxp[n].row_mask = 0x00; // row write する時用のマスク
                            o = 0;
                            for (p=0; p<i2cioxp_obj.ioxp[n].row_len; p++) {
                                r = setting_obj["i2c_option"][i]["ioxp"][n]["row"][p].as<signed int>();
                                // if (r < 0 || r >= 8) continue; // ポートA以外の場合は取得しない
                                i2cioxp_obj.ioxp[n].row[o] = r;
                                o++;
                                i2cioxp_obj.ioxp[n].row_mask |= 0x01 << r;
                            }
                            i2cioxp_obj.ioxp[n].row_len = o;
                            // マトリックス出力する時用のデータ作成
                            for (p=0; p<o; p++) {
                                i2cioxp_obj.ioxp[n].row_output[p] = i2cioxp_obj.ioxp[n].row_mask & ~(0x01 << i2cioxp_obj.ioxp[n].row[p]);
                            }
                        } else {
                            i2cioxp_obj.ioxp[n].row_len = 0;
                        }
                        // col に設定しているピン
                        if (setting_obj["i2c_option"][i]["ioxp"][n].containsKey("col") &&
                                setting_obj["i2c_option"][i]["ioxp"][n]["col"].size() ) {
                            i2cioxp_obj.ioxp[n].col_len = setting_obj["i2c_option"][i]["ioxp"][n]["col"].size();
                            i2cioxp_obj.ioxp[n].col = new uint8_t[i2cioxp_obj.ioxp[n].col_len];
                            for (p=0; p<i2cioxp_obj.ioxp[n].col_len; p++) {
                                i2cioxp_obj.ioxp[n].col[p] = setting_obj["i2c_option"][i]["ioxp"][n]["col"][p].as<signed int>();
                            }
                        } else {
                            i2cioxp_obj.ioxp[n].col_len = 0;
                        }
                        // direct に設定しているピン
                        if (setting_obj["i2c_option"][i]["ioxp"][n].containsKey("direct") &&
                                setting_obj["i2c_option"][i]["ioxp"][n]["direct"].size() ) {
                            i2cioxp_obj.ioxp[n].direct_len = setting_obj["i2c_option"][i]["ioxp"][n]["direct"].size();
                            i2cioxp_obj.ioxp[n].direct = new uint8_t[i2cioxp_obj.ioxp[n].direct_len];
                            for (p=0; p<i2cioxp_obj.ioxp[n].direct_len; p++) {
                                i2cioxp_obj.ioxp[n].direct[p] = setting_obj["i2c_option"][i]["ioxp"][n]["direct"][p].as<signed int>();
                            }
                        } else {
                            i2cioxp_obj.ioxp[n].direct_len = 0;
                        }
                    }
                } else {
                    i2cioxp_obj.ioxp_len = 0;
                }
                i2copt[j].data = (uint8_t *)new i2c_ioxp;
                memcpy(i2copt[j].data, &i2cioxp_obj, sizeof(i2c_ioxp));
                j++;

            } else if (opt_type == 2) { // 2 = Tiny202 ロータリーエンコーダ
                
                // 使用するIOエキスパンダの情報
                if (setting_obj["i2c_option"][i].containsKey("rotary") && setting_obj["i2c_option"][i]["rotary"].size()) {
                    i2crotary_obj.rotary_len = setting_obj["i2c_option"][i]["rotary"].size();
                    i2crotary_obj.rotary = new uint8_t[i2crotary_obj.rotary_len];
                    for (n=0; n<i2crotary_obj.rotary_len; n++) {
                        i2crotary_obj.rotary[n] = setting_obj["i2c_option"][i]["rotary"][n].as<signed int>();
                    }
                } else {
                    i2crotary_obj.rotary_len = 0;
                }
                i2copt[j].data = (uint8_t *)new i2c_rotary;
                memcpy(i2copt[j].data, &i2crotary_obj, sizeof(i2c_rotary));
                j++;

            } else if (opt_type == 3) { // 3 = 1U トラックボール PIM447
                // アドレス
                if (setting_obj["i2c_option"][i].containsKey("addr")) {
                    i2cpim447_obj.addr = setting_obj["i2c_option"][i]["addr"].as<signed int>();
                } else {
                    i2cpim447_obj.addr = 0;
                }
                // マウスの移動速度
                if (setting_obj["i2c_option"][i].containsKey("speed")) {
                    i2cpim447_obj.speed = setting_obj["i2c_option"][i]["speed"].as<signed int>();
                } else {
                    i2cpim447_obj.speed = 0;
                }
                i2copt[j].data = (uint8_t *)new i2c_pim447;
                memcpy(i2copt[j].data, &i2cpim447_obj, sizeof(i2c_pim447));
                j++;
            


            }

        }


    } else {
        i2copt_len = 0;
    }

    // key_input_length = 16 * ioxp_len;
    // キーの設定を取得
    // まずは設定の数を取得
    this->get_keymap(setting_obj);



    // wifiの設定読み出し
    String ssid, pass;
    wifi_data_length = setting_obj["wifi"].size(); // wifiの設定数
    wifi_data = new setting_wifi[wifi_data_length];
    for (i=0; i<wifi_data_length; i++) {
        ssid = setting_obj["wifi"][i]["ssid"].as<String>();
        pass = setting_obj["wifi"][i]["pass"].as<String>();
        m = ssid.length() + 1;
        wifi_data[i].ssid = new char[m];
        ssid.toCharArray(wifi_data[i].ssid, m);
        m = pass.length() + 1;
        wifi_data[i].pass = new char[m];
        pass.toCharArray(wifi_data[i].pass, m);
    }
    for (i=0; i<wifi_data_length; i++) {
        ESP_LOGD(LOG_TAG, "wifi setting [ %S , %S ]\n", wifi_data[i].ssid, wifi_data[i].pass );
    }

    // アクセスポイントのパスワード
    String ap_pass = setting_obj["ap"]["pass"].as<String>();
    m = ap_pass.length() + 1;
    ap_pass_char = new char[m];
    ap_pass.toCharArray(ap_pass_char, m);

    // ステータス表示用ピン番号取得
    if (setting_obj.containsKey("status_pin")) {
        status_pin = setting_obj["status_pin"].as<signed int>();
    } else {
        status_pin = -1;
    }

    if (setting_obj.containsKey("status_rgb_pin")) {
        status_rgb_pin = setting_obj["status_rgb_pin"].as<signed int>();
    } else {
        status_rgb_pin = -1;
    }
    
    // 設定されているデフォルトレイヤー取得
    default_layer_no = setting_obj["default_layer"].as<signed int>();

    // キーボードの言語取得
    if (setting_obj.containsKey("keyboard_language")) {
        keyboard_language = setting_obj["keyboard_language"].as<signed int>();
    } else {
        keyboard_language = 0;
    }

    // RGBLED設定の取得
    rgb_pin = -1;
    if (setting_obj.containsKey("rgb_pin")) rgb_pin = setting_obj["rgb_pin"].as<signed int>();
    matrix_row = -1;
    if (setting_obj.containsKey("matrix_row")) matrix_row = setting_obj["matrix_row"].as<signed int>();
    matrix_col = -1;
    if (setting_obj.containsKey("matrix_col")) matrix_col = setting_obj["matrix_col"].as<signed int>();

    led_num_length = setting_obj["led_num"].size();
    key_matrix_length = setting_obj["key_matrix"].size();
    if (led_num_length > 0) {
        led_num = new int8_t[led_num_length];
        for (i=0; i<led_num_length; i++) {
            led_num[i] = setting_obj["led_num"][i].as<signed int>();
        }
    }
    if (key_matrix_length > 0) {
        key_matrix = new int8_t[key_matrix_length];
        for (i=0; i<key_matrix_length; i++) {
            key_matrix[i] = setting_obj["key_matrix"][i].as<signed int>();
        }
    }

}

// キーマップ用に確保しているメモリを解放
void AzCommon::clear_keymap() {
    int i;
    setting_normal_input normal_input;
    for (i=0; i<setting_length; i++) {
        if (setting_press[i].action_type == 1) { // 通常キー
            memcpy(&normal_input, setting_press[i].data, sizeof(setting_normal_input));
            delete[] normal_input.key;
            delete setting_press[i].data;
        } else if (setting_press[i].action_type == 2) { // テキスト入力
            delete[] setting_press[i].data;
        } else if (setting_press[i].action_type == 3) { // レイヤー切り替え
            delete setting_press[i].data;
        } else if (setting_press[i].action_type == 4) { // WEBフック
            delete[] setting_press[i].data;
        } else if (setting_press[i].action_type == 5) { // マウス移動
            delete setting_press[i].data;
        } else if (setting_press[i].action_type == 7) { // LED設定ボタン
            delete setting_press[i].data;
        } else if (setting_press[i].action_type == 8) { // 打鍵設定ボタン
            delete setting_press[i].data;
        }
    }
    delete[] setting_press;
}

// JSONデータからキーマップの情報を読み込む
void AzCommon::get_keymap(JsonObject setting_obj) {
    int i, j, k, m, at, s;
    char lkey[16];
    char kkey[16];
    uint16_t lnum, knum;
    JsonObject::iterator it_l;
    JsonObject::iterator it_k;
    JsonObject layers, keys;
    JsonObject press_obj;
    String text_str;
    setting_normal_input normal_input;
    setting_layer_move layer_move_input;
    setting_mouse_move mouse_move_input;
    // まずはキー設定されている数を取得
    layers = setting_obj["layers"].as<JsonObject>();
    setting_length = 0;
    for (it_l=layers.begin(); it_l!=layers.end(); ++it_l) {
        setting_length += setting_obj["layers"][it_l->key().c_str()]["keys"].size();
    }
    // Serial.printf("setting total %D\n", setting_length);
    // 設定数分メモリ確保
    // Serial.printf("setting_key_press:start: %D %D\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    setting_press = new setting_key_press[setting_length];
    // Serial.printf("setting_key_press:end: %D %D\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) );
    // キー設定読み込み
    i = 0;
    for (it_l=layers.begin(); it_l!=layers.end(); ++it_l) {
        sprintf(lkey, "%S", it_l->key().c_str());
        lnum = split_num(lkey);
        keys = setting_obj["layers"][lkey]["keys"].as<JsonObject>();
        for (it_k=keys.begin(); it_k!=keys.end(); ++it_k) {
            sprintf(kkey, "%S", it_k->key().c_str());
            knum = split_num(kkey);
            // Serial.printf("get_keymap: %S %S [ %D %D ]\n", lkey, kkey, lnum, knum);
            // Serial.printf("mem: %D %D\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) );
            press_obj = setting_obj["layers"][lkey]["keys"][kkey]["press"].as<JsonObject>();
            setting_press[i].layer = lnum;
            setting_press[i].key_num = knum;
            setting_press[i].action_type = press_obj["action_type"].as<signed int>();
            if (setting_press[i].action_type == 1) {
                // 通常入力
                normal_input.key_length = press_obj["key"].size();
                normal_input.key = new uint16_t[normal_input.key_length];
                for (j=0; j<normal_input.key_length; j++) {
                      normal_input.key[j] = press_obj["key"][j].as<signed int>();
                }
                if (press_obj.containsKey("repeat_interval")) {
                    normal_input.repeat_interval = press_obj["repeat_interval"].as<signed int>();
                } else {
                    normal_input.repeat_interval = 51;
                }
                if (press_obj.containsKey("hold")) {
                    normal_input.hold = press_obj["hold"].as<signed int>();
                } else {
                    normal_input.hold = 0;
                }
                setting_press[i].data = (char *)new setting_normal_input;
                memcpy(setting_press[i].data, &normal_input, sizeof(setting_normal_input));
            } else if (setting_press[i].action_type == 2) {
                // テキスト入力
                text_str = press_obj["text"].as<String>();
                m = text_str.length() + 1;
                setting_press[i].data = new char[m];
                text_str.toCharArray(setting_press[i].data, m);
            } else if (setting_press[i].action_type == 3) {
                // レイヤー切り替え
                layer_move_input.layer_id = press_obj["layer"].as<signed int>();
                layer_move_input.layer_type = press_obj["layer_type"].as<signed int>();
                if (layer_move_input.layer_type == 0) layer_move_input.layer_type = 0x51; // 切り替え方法の指定が無かった場合はMO(押している間切り替わる)
                setting_press[i].data = (char *)new setting_layer_move;
                memcpy(setting_press[i].data, &layer_move_input, sizeof(setting_layer_move));
            } else if (setting_press[i].action_type == 4) {
                // WEBフック
                text_str = "";
                serializeJson(press_obj["webhook"], text_str);
                m = text_str.length() + 1;
                setting_press[i].data = new char[m];
                text_str.toCharArray(setting_press[i].data, m);
                
            } else if (setting_press[i].action_type == 5) {
                // マウス移動
                mouse_move_input.x = press_obj["move"]["x"].as<signed int>();
                mouse_move_input.y = press_obj["move"]["y"].as<signed int>();
                mouse_move_input.speed = press_obj["move"]["speed"].as<signed int>();
                setting_press[i].data = (char *)new setting_mouse_move;
                memcpy(setting_press[i].data, &mouse_move_input, sizeof(setting_mouse_move));

            } else if (setting_press[i].action_type == 6) {
                // 暗記ボタン

            } else if (setting_press[i].action_type == 7) {
                // LED設定ボタン
                setting_press[i].data = new char;
                *setting_press[i].data = press_obj["led_setting_type"].as<signed int>();
                
            } else if (setting_press[i].action_type == 8) {
                // 打鍵設定ボタン
                setting_press[i].data = new char;
                *setting_press[i].data = press_obj["dakagi_settype"].as<signed int>();

            }
            i++;
        }
    }
  
    // ログに出力して確認
    for (i=0; i<setting_length; i++) {
        if (setting_press[i].action_type == 1) {
            memcpy(&normal_input, setting_press[i].data, sizeof(setting_normal_input));
            ESP_LOGD(LOG_TAG, "setting_press %D %D %D %D [%D, %D]", i, setting_press[i].layer, setting_press[i].key_num, setting_press[i].action_type, normal_input.key_length, normal_input.repeat_interval);
            for (j=0; j<normal_input.key_length; j++) {
                ESP_LOGD(LOG_TAG, "setting_press %D ", normal_input.key[j]);
            }
        } else if (setting_press[i].action_type == 2) {
            ESP_LOGD(LOG_TAG, "setting_press %D %D %D %D [ %S ]\n", i, setting_press[i].layer, setting_press[i].key_num, setting_press[i].action_type, setting_press[i].data);
        } else if (setting_press[i].action_type == 3) {
            ESP_LOGD(LOG_TAG, "setting_press %D %D %D %D [ %D ]\n", i, setting_press[i].layer, setting_press[i].key_num, setting_press[i].action_type, *setting_press[i].data);
        } else if (setting_press[i].action_type == 4) {
            ESP_LOGD(LOG_TAG, "setting_press %D %D %D %D [ %S ]\n", i, setting_press[i].layer, setting_press[i].key_num, setting_press[i].action_type, setting_press[i].data);
        } else if (setting_press[i].action_type == 5) {
            memcpy(&mouse_move_input, setting_press[i].data, sizeof(setting_mouse_move));
            ESP_LOGD(LOG_TAG, "setting_press %D %D %D %D [ %D, %D, %D ]\n", i, setting_press[i].layer, setting_press[i].key_num, setting_press[i].action_type, mouse_move_input.x, mouse_move_input.y, mouse_move_input.speed);
        } else {
            ESP_LOGD(LOG_TAG, "setting_press %D %D %D %D\n", i, setting_press[i].layer, setting_press[i].key_num, setting_press[i].action_type);
        }
    }
    ESP_LOGD(LOG_TAG, "mmm: %D %D\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT) );

}


// ファイルを開いてテキストをロードする
int AzCommon::read_file(char *file_path, String &read_data) {
    ESP_LOGD(LOG_TAG, "read file: %S\n", file_path);
    // ファイルが無ければエラー
    if (!SPIFFS.exists(file_path)) {
        ESP_LOGD(LOG_TAG, "read_file not file: %S\n", file_path);
        return 0;
    }
    // ファイルオープン
    File fp = SPIFFS.open(file_path, "r");
    if(!fp){
        ESP_LOGD(LOG_TAG, "read_file open error: %S\n", file_path);
        return 0;
    }
    // 読み込み
    read_data = fp.readString();
    fp.close();
    return 1;
}

// テキストをファイルに書き込む
int AzCommon::write_file(char *file_path, String &write_data) {
    // 書込みモードでファイルオープン
      ESP_LOGD(LOG_TAG, "write_file open path: %S\n", file_path);
    File fp = SPIFFS.open(file_path, FILE_WRITE);
    if(!fp){
        ESP_LOGD(LOG_TAG, "write_file open error: %S\n", file_path);
        return 0;
    }
    // 書込み
    ESP_LOGD(LOG_TAG, "save");
    if(!fp.print(write_data)){
        ESP_LOGD(LOG_TAG, "write_file print error");
        fp.close();
        return 0;
    }
    fp.close();
    ESP_LOGD(LOG_TAG, "write_file print ok");
    return 1;
}

// ファイルを削除
int AzCommon::remove_file(char *file_path) {
    if (SPIFFS.remove(file_path)) {
        return 1;
    } else {
        return 0;
    }
}


// デフォルトのsetting.jsonを生成する
bool AzCommon::create_setting_json() {
    // 書込みモードでファイルオープン
    File json_file = SPIFFS.open(SETTING_JSON_PATH, FILE_WRITE);
    if(!json_file){
        ESP_LOGD(LOG_TAG, "create_setting_json open error");
        return false;
    }
    // 書込み
    if(!json_file.print(setting_json_default_bin)){
        ESP_LOGD(LOG_TAG, "create_setting_json print error");
        json_file.close();
        return false;
    }
    json_file.close();
    ESP_LOGD(LOG_TAG, "create_setting_json print ok");
    return true;
}

// I2C機器の初期化(戻り値：増えるキーの数)
int AzCommon::i2c_setup(int p, i2c_option *opt) {
    int i, j, k, m, x;
    int r = 0;
    int set_type[16];
    i2c_map i2cmap_obj;
    i2c_ioxp i2cioxp_obj;
    // Serial.printf("i2c_setup: opt_type %D\n", opt->opt_type);
    if (opt->opt_type == 1) {
        // IOエキスパンダ
        memcpy(&i2cioxp_obj, opt->data, sizeof(i2c_ioxp));
        for (i=0; i<i2cioxp_obj.ioxp_len; i++) {
            x = i2cioxp_obj.ioxp[i].addr - 32; // アドレス
            // Serial.printf("i2c_setup: %D %D %D\n", i, x, ioxp_status[x]);
            // まだ初期化されていないIOエキスパンダなら初期化
            if (ioxp_status[x] < 0) {
                ioxp_obj[x] = new Adafruit_MCP23X17();
                ioxp_status[x] = 0;
            }
            if (ioxp_status[x] < 1) {
            // Serial.printf("i2c_setup: begin %D\n", i2cioxp_obj.ioxp[i].addr);
                if (ioxp_obj[x]->begin_I2C(i2cioxp_obj.ioxp[i].addr, &Wire)) {
                    ioxp_hash[x] = 1;
                    ioxp_status[x] = 1;
                } else {
                    // 初期化失敗
                    continue;
                }
            }
            // ピンのタイプデータをINPUT_PULLUPで初期化
            for (j=0; j<16; j++) {
                set_type[j] = INPUT_PULLUP;
            }
            // rowピンだけOUTPUTにする
            for (j=0; j<i2cioxp_obj.ioxp[i].row_len; j++) {
                set_type[ i2cioxp_obj.ioxp[i].row[j] ] = OUTPUT;
            }
            // ピン初期化
            for (j=0; j<16; j++) {
            // Serial.printf("i2c_setup: pinMode %D %D\n", j, set_type[j]);
                ioxp_obj[x]->pinMode(j, set_type[j]);
            }
        }
    } else if (opt->opt_type == 2) {
        // ATTiny202 ロータリーエンコーダー
        // 初期化特になし

    } else if (opt->opt_type == 3) {
        // 1U トラックボール PIM447
        // 初期化特になし

    }
    // マッピングに合わせてキー番号を付けなおす
    if (opt->opt_type == 1 || opt->opt_type == 2 || opt->opt_type == 3) {
        // キーの番号をmapデータに入れる
        // あとでキー設定の番号入れ替えをここでやる
        memcpy(&i2cmap_obj, opt->i2cmap, sizeof(i2c_map));
        k = i2cmap_obj.map_start;
        for (i=0; i<i2cmap_obj.map_len; i++) {
            // 設定内容中の番号を付け替える
            for (j=0; j<setting_length; j++) {
                if (setting_press[j].key_num == p) {
                    // 元から同じ番号の設定が入っていたら消す
                    setting_press[j].key_num = -1;
                } else if (setting_press[j].key_num == k) {
                    // Serial.printf("map: %D %D %D [ %D -> %D ]\n", i, j, k, setting_press[j].key_num, p);
                    setting_press[j].key_num = p;
                }
            }
            p++;
            k++;
        }
    }

    return p;
}

// キーの入力ピンの初期化
void AzCommon::pin_setup() {
    // output ピン設定 (colで定義されているピンを全てoutputにする)
    int i, j, m, x;

    for (i=0; i<col_len; i++) {
        if (!AZ_DEBUG_MODE || (col_list[i] != 1 && col_list[i] != 3)) pinMode(col_list[i], OUTPUT_OPEN_DRAIN);
    }
    // row で定義されているピンを全てinputにする
    for (i=0; i<row_len; i++) {
        if (!AZ_DEBUG_MODE || (row_list[i] != 1 && row_list[i] != 3)) pinMode(row_list[i], INPUT_PULLUP);
    }
    // direct(スイッチ直接続)で定義されているピンを全てinputにする
    for (i=0; i<direct_len; i++) {
        pinMode(direct_list[i], INPUT_PULLUP);
    }

    // キー数の計算
    key_input_length = (col_len * row_len) + direct_len + touch_len;

    // I2C初期化
    if (ioxp_sda >= 0 && ioxp_scl >= 0) {

        if (Wire.begin(ioxp_sda, ioxp_scl)) {
            Wire.setClock(ioxp_hz);
        } else {
            delay(1000);
        }

        // IOエキスパンダ初期化
        for (i=0; i<ioxp_len; i++) {
            x = ioxp_list[i] - 32;
            if (ioxp_status[x] < 0) {
                ioxp_obj[x] = new Adafruit_MCP23X17();
                ioxp_status[x] = 0;
            }
            if (ioxp_status[x] < 1) {
                if (ioxp_obj[x]->begin_I2C(ioxp_list[i], &Wire)) {
                    ioxp_status[x] = 1;
                    ioxp_hash[x] = 1;
                } else {
                    // 初期化失敗
                    continue;
                }
            }
            delay(10);
            for (j = 0; j < 16; j++) {
                ioxp_obj[x]->pinMode(j, INPUT_PULLUP);
            }
            delay(50);
        }
        key_input_length += (ioxp_len * 16);
        

        // I2C接続のオプション初期化
        for (i=0; i<i2copt_len; i++) {
            key_input_length = i2c_setup(key_input_length, &i2copt[i]);
        }
        
    }

    // 動作電圧チェック用ピン
    power_read_pin = 36;
    if (power_read_pin >= 0) { // 電圧を読み込むピン
        this->pinmode_analog(power_read_pin);
    }

    if (key_input_length > KEY_INPUT_MAX) key_input_length = KEY_INPUT_MAX;
    ESP_LOGD(LOG_TAG, "key length : %D\r\n", key_input_length);
    // 打鍵数リセット
    for (i=0; i<KEY_INPUT_MAX; i++) {
        this->key_count[i] = 0;
    }
    this->key_count_total = 0;
}

// アナログ入力ピン初期化
void AzCommon::pinmode_analog(int gpio_no) {
    int i;
    i = this->get_adc_num(gpio_no); // adc1か adc2か
    if (i == 1) {
        adc1_config_width(ADC_WIDTH_12Bit);
        adc1_config_channel_atten(this->get_channel_1(gpio_no), ADC_ATTEN_11db);
    } else if (i == 2) {
        // adc2_config_width(ADC_WIDTH_12Bit); // 2の方は電圧取得時にBIT数を指定する
        adc2_config_channel_atten(this->get_channel_2(gpio_no), ADC_ATTEN_11db);
    }
}

// アナログピンの入力を取得
int AzCommon::analog_read(int gpio_no) {
    int i, r = -1;
    i = this->get_adc_num(gpio_no); // adc1か adc2か
    if (i == 1) {
        r = adc1_get_voltage(this->get_channel_1(gpio_no));
    } else if (i == 2) {
        adc2_get_raw(this->get_channel_2(gpio_no), ADC_WIDTH_BIT_12, &r);
    }
    return r;
}

// GPIOの番号からADCのチャネルを取得する
adc1_channel_t AzCommon::get_channel_1(int gpio_no) {
    // 参考 通常のESP32 https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/peripherals/adc.html
    if (gpio_no == 36) return ADC1_CHANNEL_0;
    if (gpio_no == 37) return ADC1_CHANNEL_1;
    if (gpio_no == 38) return ADC1_CHANNEL_2;
    if (gpio_no == 39) return ADC1_CHANNEL_3;
    if (gpio_no == 32) return ADC1_CHANNEL_4;
    if (gpio_no == 33) return ADC1_CHANNEL_5;
    if (gpio_no == 34) return ADC1_CHANNEL_6;
    if (gpio_no == 35) return ADC1_CHANNEL_7;
    return ADC1_CHANNEL_0; // ADCが無いピンを指定されたらとりあえずgpio36を返しとく
}

// GPIOの番号からADCのチャネルを取得する
adc2_channel_t AzCommon::get_channel_2(int gpio_no) {
    // 参考 通常のESP32 https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/api-reference/peripherals/adc.html
    if (gpio_no == 4) return ADC2_CHANNEL_0;
    if (gpio_no == 0) return ADC2_CHANNEL_1;
    if (gpio_no == 2) return ADC2_CHANNEL_2;
    if (gpio_no == 15) return ADC2_CHANNEL_3;
    if (gpio_no == 13) return ADC2_CHANNEL_4;
    if (gpio_no == 12) return ADC2_CHANNEL_5;
    if (gpio_no == 14) return ADC2_CHANNEL_6;
    if (gpio_no == 27) return ADC2_CHANNEL_7;
    if (gpio_no == 25) return ADC2_CHANNEL_8;
    if (gpio_no == 26) return ADC2_CHANNEL_9;
    return ADC2_CHANNEL_0; // ADCが無いピンを指定されたらとりあえずgpio4を返しとく
}

// GPIOの番号からADC1かADC2かを返す
int AzCommon::get_adc_num(int gpio_no) {
    if (gpio_no == 36 || gpio_no == 37 || gpio_no == 38 || gpio_no == 39 
        || gpio_no == 32 || gpio_no == 33 || gpio_no == 34 || gpio_no == 35) return 1;
    if (gpio_no == 4 || gpio_no == 0 || gpio_no == 2 || gpio_no == 15 || gpio_no == 13 || gpio_no == 12 
        || gpio_no == 14 || gpio_no == 27 || gpio_no == 25 || gpio_no == 26) return 2;
    return 0;
}

// 電源電圧を取得
int AzCommon::get_power_vol() {
    // 電圧を読み込む
    return this->analog_read(power_read_pin);
}

// レイヤーが存在するか確認
bool AzCommon::layers_exists(int layer_no) {
    int i;
    for (i=0; i<setting_length; i++) {
        if (setting_press[i].layer == layer_no) return true;
    }
    return false;
}

// 指定したキーの入力設定オブジェクトを取得する
setting_key_press AzCommon::get_key_setting(int layer_id, int key_num) {
    int i;
    for (i=0; i<setting_length; i++) {
        if (setting_press[i].layer == layer_id && setting_press[i].key_num == key_num) return setting_press[i];
    }
    setting_key_press r;
    r.layer = -1;
    r.key_num = -1;
    r.action_type = -1;
    return r;
}


//データをEEPROMから読み込む。保存データが無い場合デフォルトにする。
void AzCommon::load_data() {
    // デフォルト値セット
    // 起動モード
    eep_data.boot_mode = 0;
    // データチェック文字列
    strcpy(eep_data.check, EEP_DATA_VERSION);
    // ユニークID
    getRandomNumbers(10, eep_data.uid);
    // キーボードの種類
    strcpy(eep_data.keyboard_type, "");
    File fp;
    if (!SPIFFS.exists(AZ_SYSTEM_FILE_PATH)) {
        // ファイルが無い場合デフォルト値でファイルを作成
        save_data();
        return;
    }
    // ファイルがあればデータ読み込み
    fp = SPIFFS.open(AZ_SYSTEM_FILE_PATH, "r");
    if(!fp){
        ESP_LOGD(LOG_TAG, "file open error\n");
        return;
    }
    if (fp.available()) {
        fp.read((uint8_t *)&eep_data, sizeof(mrom_data_set));
    }
    fp.close();
    // データのバージョンが変わっていたらファイルを消して再起動
    if (strcmp(eep_data.check, EEP_DATA_VERSION) != 0) {
        SPIFFS.remove(AZ_SYSTEM_FILE_PATH);
        SPIFFS.remove(SETTING_JSON_PATH);
        delay(300);
        ESP.restart(); // ESP32再起動
    }
}


// データをEEPROMに書き込む
void AzCommon::save_data() {
    //EEPROMに設定を保存する。
    File fp;
    strcpy(eep_data.check, EEP_DATA_VERSION);
    // ファイルに書き込み
    fp = SPIFFS.open(AZ_SYSTEM_FILE_PATH, "w");
    fp.write((uint8_t *)&eep_data, sizeof(mrom_data_set));
    fp.close();
    delay(200);
    ESP_LOGD(LOG_TAG, "save complete\r\n");
}

// 起動回数読み込み
void AzCommon::load_boot_count() {
    File fp;
    boot_count = 0;
    // ファイルが無い場合はデフォルトファイル作成
    if (SPIFFS.exists(BOOT_COUNT_PATH)) {
        // boot_count から読み込み
        fp = SPIFFS.open(BOOT_COUNT_PATH, "r");
        if(!fp){
            ESP_LOGD(LOG_TAG, "boot count file open error\n");
            return;
        }
        if (fp.available()) {
            fp.read((uint8_t *)&boot_count, 4);
        }
        fp.close();
    }
    // カウントアップ
    boot_count++;
    // ファイルに書き込み
    fp = SPIFFS.open(BOOT_COUNT_PATH, "w");
    fp.write((uint8_t *)&boot_count, 4);
    fp.close();
}


// ファイルから設定値を読み込み
void AzCommon::load_file_data(char *file_path, uint8_t *load_point, uint16_t load_size) {
    File fp;
    // ファイルが無い場合は何もしない
    if (!SPIFFS.exists(file_path)) return;
    // ファイル読み込み
    fp = SPIFFS.open(file_path, "r");
    if(!fp){
        ESP_LOGD(LOG_TAG, "file open error\n");
        return;
    }
    if (fp.available()) {
        fp.read(load_point, load_size);
    }
    fp.close();
}


// ファイルに設定値を書込み
void AzCommon::save_file_data(char *file_path, uint8_t *save_point, uint16_t save_size) {
    // ファイルに書き込み
    File fp;
    fp = SPIFFS.open(file_path, "w");
    fp.write(save_point, save_size);
    fp.close();
}


// 起動モードを変更してEEPROMに保存
void AzCommon::set_boot_mode(int set_mode) {
    eep_data.boot_mode = set_mode;
    save_data();
}

// モードを切り替えて再起動
void AzCommon::change_mode(int set_mode) {
    set_boot_mode(set_mode);
    ESP.restart(); // ESP32再起動
}


int AzCommon::i2c_read(int p, i2c_option *opt, char *read_data) {
    int e, i, j, k, m, n, r, x, y;
    unsigned long start_time;
    unsigned long end_time;
    uint16_t rowput_mask;
    int rowput_len;
    int read_data_bit = 8;
    uint16_t read_raw[32];
    Adafruit_MCP23X17 *ioxp;
    i2c_map i2cmap_obj;
    i2c_ioxp i2cioxp_obj;
    i2c_rotary i2crotary_obj;
    i2c_pim447 i2cpim447_obj;
    tracktall_pim447_data pim447_data_obj;
    r = 0;
    e = 0;
    if (opt->opt_type == 1) {
        // IOエキスパンダ
        read_data_bit = 16;
        memcpy(&i2cioxp_obj, opt->data, sizeof(i2c_ioxp));
        memcpy(&i2cmap_obj, opt->i2cmap, sizeof(i2c_map));
        for (i=0; i<i2cioxp_obj.ioxp_len; i++) {
            x = i2cioxp_obj.ioxp[i].addr - 32; // アドレス
            // まだ初期化されていないIOエキスパンダなら無視
            if (ioxp_status[x] < 1) continue;
            // row と col があればマトリックス入力
            if (i2cioxp_obj.ioxp[i].row_len > 0 && i2cioxp_obj.ioxp[i].col_len > 0) {
                // まずrowでoutputするデータとマスクを用意
                rowput_len = i2cioxp_obj.ioxp[i].row_len;
                rowput_mask = i2cioxp_obj.ioxp[i].row_mask;
                ioxp = ioxp_obj[x];
                // rowのoutput分ループ
                for (j=0; j<rowput_len; j++) {
                    if (rowput_mask & 0xff00) { // ポートB
                        ioxp->writeGPIO((i2cioxp_obj.ioxp[i].row_output[j] >> 8) & 0xff, 1); // ポートBに出力
                    }
                    if (rowput_mask & 0xff) { // ポートA
                        ioxp->writeGPIO(i2cioxp_obj.ioxp[i].row_output[j] & 0xff, 0); // ポートAに出力
                    }
                    read_raw[e] = ~ioxp->readGPIOAB() | rowput_mask; // ポートA,B両方のデータを取得
                    e++;
                }
            } else {
                // col と row が無い場合はダイレクトのみ
                read_raw[e] = ~ioxp_obj[x]->readGPIOAB(); // ポートA,B両方のデータを取得
                e++;
            }
        }
    } else if (opt->opt_type == 2) {
        // ATTiny202 ロータリーエンコーダー
        read_data_bit = 8;
        memcpy(&i2crotary_obj, opt->data, sizeof(i2c_rotary));
        memcpy(&i2cmap_obj, opt->i2cmap, sizeof(i2c_map));
        for (i=0; i<i2crotary_obj.rotary_len; i++) {
            read_raw[e] = wirelib_cls.read_rotary(i2crotary_obj.rotary[i]);
            e++;
        }

    } else if (opt->opt_type == 3) {
        // 1U トラックボール PIM447
        read_data_bit = 8;
        memcpy(&i2cpim447_obj, opt->data, sizeof(i2c_pim447));
        memcpy(&i2cmap_obj, opt->i2cmap, sizeof(i2c_map));
        pim447_data_obj = wirelib_cls.read_trackball_pim447(i2cpim447_obj.addr); // 入力情報取得
        // トラックボール操作があればマウス移動リストに追加
        x = (pim447_data_obj.right - pim447_data_obj.left);
        y = (pim447_data_obj.down - pim447_data_obj.up);
        if (x != 0 || y != 0) press_mouse_list_push(0x2000, x, y, i2cpim447_obj.speed);
        // キー入力(クリック)取得
        read_raw[e] = pim447_data_obj.click;
        e++;

    }
    // 読み込んだデータからキー入力を取得
    if (opt->opt_type == 1 || opt->opt_type == 2 || opt->opt_type == 3) {
        // マップデータ分入力を取得
        for (j=0; j<i2cmap_obj.map_len; j++) {
            n = i2cmap_obj.map[j];
            k = n / read_data_bit;
            m = n % read_data_bit;
            if (k >= e) k = e - 1; // マトリックスで取得した数より多い場合はdirectの指定なので一番最後に取得した情報で判定
            read_data[p] = (read_raw[k] & (0x01 << m))? 1: 0;
            p++;
            r++;
        }
    }
    return r;
}


// 現在のキーの入力状態を取得
void AzCommon::key_read(void) {
    int i, j, n, s;
    n = 0;
    // ダイレクト入力の取得
    for (i=0; i<direct_len; i++) {
        input_key[n] = !digitalRead(direct_list[i]);
        n++;
    }
    // タッチ入力の取得
    for (i=0; i<touch_len; i++) {
        if (touchRead(touch_list[i]) < 25) {
            input_key[n] = 1;
        } else {
            input_key[n] = 0;
        }
        n++;
    }
    // マトリックス入力の取得
    for (i=0; i<col_len; i++) {
        // 対象のcolピンのみ lowにする
        for (j=0; j<col_len; j++) {
            if (i == j) { s = 0; } else { s = 1; }
            if (!AZ_DEBUG_MODE || (col_list[j] != 1 && col_list[j] != 3)) digitalWrite(col_list[j], s);
        }
        delayMicroseconds(50);
        // row の分キー入力チェック
        for (j=0; j<row_len; j++) {
            input_key[n] = !digitalRead(row_list[j]);
            n++;
        }
    }
    // I2Cオプション
    for (i=0; i<i2copt_len; i++) {
        n += i2c_read(n, &i2copt[i], input_key);
    }
}


// 今回の入力状態を保持
void AzCommon::key_old_copy(void) {
    int i;
    for (i=0; i<key_input_length; i++) {
        input_key_last[i] = input_key[i];
    }
}

// 全てのファイルを削除
void AzCommon::delete_all(void) {
    File dirp = SPIFFS.open("/");
    File filep = dirp.openNextFile();
    while(filep){
        SPIFFS.remove(filep.name());
        filep = dirp.openNextFile();
    }
}

// 指定した文字から始まるファイルすべて削除
void AzCommon::delete_indexof_all(String check_str) {
    File dirp = SPIFFS.open("/");
    File filep = dirp.openNextFile();
    String file_path;
    while(filep){
        file_path = String(filep.name());
        if (file_path.indexOf("/" + check_str) == 0) {
            SPIFFS.remove(file_path);
        }
        filep = dirp.openNextFile();
    }
}

// ファイル領域合計サイズを取得
int AzCommon::spiffs_total(void) {
    return SPIFFS.totalBytes();
}

// 使用しているファイル領域サイズを取得
int AzCommon::spiffs_used(void) {
    return SPIFFS.usedBytes();
}


// マウス移動リストを空にする
void AzCommon::press_mouse_list_clean() {
    int i;
    for (i=0; i<PRESS_MOUSE_MAX; i++) {
        press_mouse_list[i].key_num = -1;
        press_mouse_list[i].move_x = 0;
        press_mouse_list[i].move_y = 0;
        press_mouse_list[i].move_speed = 0;
        press_mouse_list[i].move_index = 0;
    }
}


// マウス移動リストに追加
void AzCommon::press_mouse_list_push(int key_num, short move_x, short move_y, short move_speed) {
    int i;
    for (i=0; i<PRESS_MOUSE_MAX; i++) {
        // データが入っている or 指定されたキー番号以外は 次
        if (press_mouse_list[i].key_num >= 0 && press_mouse_list[i].key_num != key_num) continue;
        // 空いていればデータを入れる
        press_mouse_list[i].key_num = key_num;
        press_mouse_list[i].move_x = move_x;
        press_mouse_list[i].move_y = move_y;
        press_mouse_list[i].move_speed = move_speed;
        press_mouse_list[i].move_index = 0;
        break;
    }
}


// マウス移動リストから削除
void AzCommon::press_mouse_list_remove(int key_num) {
    int i;
    for (i=0; i<PRESS_MOUSE_MAX; i++) {
        // 該当のキーで無ければ何もしない
        if (press_mouse_list[i].key_num != key_num) continue;
        // 該当のキーのデータ削除
        press_mouse_list[i].key_num = -1;
        press_mouse_list[i].move_x = 0;
        press_mouse_list[i].move_y = 0;
        press_mouse_list[i].move_speed = 0;
        press_mouse_list[i].move_index = 0;
    }
}
