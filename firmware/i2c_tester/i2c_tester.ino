#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>


// I2Cのピン
#define IOMCP_SDA  18
#define IOMCP_SCL  26


// ステータスLED
Adafruit_NeoPixel *status_rgb;

// 押されているかどうかのフラグ
uint8_t input_key[16];

// 最後のLEDの状態
int last_status;
int wire_err;

// 初期化
void setup() {

    // 入力ピン初期化
    pinMode(21, INPUT_PULLUP);
    pinMode(19, INPUT_PULLUP);
    pinMode(22, INPUT_PULLUP);
    pinMode(25, INPUT_PULLUP);

    // ステータスLED初期化
    status_rgb =  new Adafruit_NeoPixel(1, 27, NEO_GRB + NEO_KHZ400);
    status_rgb->setPixelColor(0, status_rgb->Color(0, 0, 0));
    status_rgb->show();
    last_status = 0;

    // I2C初期化
    if (Wire.begin(IOMCP_SDA, IOMCP_SCL)) {
        Wire.setClock(400000);
    }

    status_rgb->setPixelColor(0, status_rgb->Color(10, 10, 0));
    status_rgb->show();
    last_status = 0;

    delay(200);
}

// キーの入力状態を取得
void key_read() {
    int i;

    // 入力データクリア
    for (i=0; i<16; i++) {
      input_key[i] = 0;
    }


    // ダイレクトキー入力取得

    if (!digitalRead(21)) input_key[1] = 1;
    if (!digitalRead(19)) input_key[2] = 1;
    if (!digitalRead(22)) input_key[2] = 1;
    if (!digitalRead(25)) input_key[3] = 1;

    Wire.beginTransmission(0x40);
    Wire.write(0x10);
    wire_err = Wire.endTransmission();
    Wire.requestFrom(0x40, 1);
    // while (Wire.available()) 
    input_key[0] = Wire.read(); // データ全部受け取る

}

// 動作ループ
void loop() {
    int i;
    bool change_flag = false;

    // キーの入力情報取得
    key_read();

    // 前回の入力から変更があったかチェック
    for (i=0; i<4; i++) {
        if (input_key[i]) change_flag = true;
    }

    // 変更が無ければ何もしない
    if (!change_flag) {
        if (last_status > 0) {
            status_rgb->setPixelColor(0, status_rgb->Color(10, 10, 0));
            status_rgb->show();
            /*
            Wire.beginTransmission(0x40);
            Wire.write(0x00);
            wire_err = Wire.endTransmission();
            */
            last_status = 0;
        }
        delay(20);
        return;
    }

    if (last_status != input_key[0]) {
        status_rgb->setPixelColor(0, status_rgb->Color(0, (input_key[0] & 0x01) * 10, (input_key[0] & 0x02) * 5));
        status_rgb->show();
        /*
        Wire.beginTransmission(0x40);
        Wire.write(0x01);
        wire_err = Wire.endTransmission();
        */
        last_status = input_key[0];
    }

    delay(50);
}
