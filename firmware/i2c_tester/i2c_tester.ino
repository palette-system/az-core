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

  Serial.begin(115200);

    // 入力ピン初期化
    pinMode(21, INPUT_PULLUP);
    pinMode(19, INPUT_PULLUP);
    pinMode(22, INPUT_PULLUP);
    pinMode(25, INPUT_PULLUP);
    pinMode(32, INPUT_PULLUP);

    // ステータスLED初期化
    status_rgb =  new Adafruit_NeoPixel(1, 27, NEO_GRB + NEO_KHZ400);
    status_rgb->setPixelColor(0, status_rgb->Color(0, 0, 0));
    status_rgb->show();
    last_status = 0;

    // I2C初期化
    if (Wire.begin(IOMCP_SDA, IOMCP_SCL)) {
        Wire.setClock(400000);
    }

    status_rgb->setPixelColor(0, status_rgb->Color(10, 0, 0));
    status_rgb->show();
    last_status = 0;

    delay(200);
}

// キーの入力状態を取得
void key_read() {
    int i, r, s, e;

    // 入力データクリア
    for (i=0; i<16; i++) {
      input_key[i] = 0;
    }


    // ダイレクトキー入力取得

    if (!digitalRead(21)) input_key[0] = 1;
    if (!digitalRead(19)) input_key[1] = 1;
    if (!digitalRead(22)) input_key[2] = 1;
    if (!digitalRead(25)) input_key[3] = 1;
    if (!digitalRead(32)) input_key[4] = 1;

    s = micros();
    Wire.requestFrom(0x44, 1);
    r = Wire.read(); // データ受け取る
    e = micros();
    if (r > 0) Serial.printf("R: %d %d\n", r, (e - s));
    if (r & 0x01) input_key[5] = 1;
    if (r & 0x02) input_key[6] = 1;
    if (r & 0x04) input_key[7] = 1;
    if (r & 0x08) input_key[8] = 1;

}

// 動作ループ
void loop() {
    int i, n;
    bool change_flag = false;

    // キーの入力情報取得
    key_read();

    // 今回入力があるかチェック
    for (i=0; i<9; i++) {
        if (input_key[i]) change_flag = true;
    }

    // 入力無し
    if (!change_flag) {
        if (last_status > 0) {
            status_rgb->setPixelColor(0, status_rgb->Color(10, 0, 0));
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

    n = input_key[0] + input_key[1] + input_key[2] + input_key[3] + input_key[4] + input_key[5] + input_key[6] + input_key[7] + input_key[8];

    if (last_status != n) {
        if (input_key[7] || input_key[8]) {
            status_rgb->setPixelColor(0, status_rgb->Color(0, input_key[7] * 10, input_key[8] * 10));
        } else {
            status_rgb->setPixelColor(0, status_rgb->Color(0, 10, 10));
        }
        status_rgb->show();
        /*
        Wire.beginTransmission(0x40);
        Wire.write(0x01);
        wire_err = Wire.endTransmission();
        */
        last_status = n;
    }

    delay(50);
}
