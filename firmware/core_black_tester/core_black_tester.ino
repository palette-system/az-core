#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>

// I/Oエキスパンダの数
#define  IOMCP_LENGTH  4

// チェックするキーの数
#define  INPUT_KEY_LENGTH  (16 * IOMCP_LENGTH)


// I2Cのピン
#define IOMCP_SDA  18
#define IOMCP_SCL  26

// I/Oエキスパンダ用
Adafruit_MCP23X17 *iomcp;

// ステータスLED
Adafruit_NeoPixel *status_rgb;

// 押されているかどうかのフラグ
bool input_key[INPUT_KEY_LENGTH];

// 最後のLEDの状態
int last_status;

// 初期化
void setup() {

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

    // MCP23017 初期化
    int i, j;
    iomcp = new Adafruit_MCP23X17[2];
    if (iomcp[0].begin_I2C(0x26, &Wire)) {
        for (i=0; i<2; i++) iomcp[0].pinMode(i, OUTPUT);
        for (i=2; i<16; i++) iomcp[0].pinMode(i, INPUT_PULLUP);
    }
    if (iomcp[1].begin_I2C(0x24, &Wire)) {
        for (i=0; i<2; i++) iomcp[1].pinMode(i, OUTPUT);
        for (i=2; i<16; i++) iomcp[1].pinMode(i, INPUT_PULLUP);
    }

    // キー入力データ初期化
    for (i=0; i<INPUT_KEY_LENGTH; i++) {
        input_key[i] = false;
    }

    status_rgb->setPixelColor(0, status_rgb->Color(10, 10, 0));
    status_rgb->show();
    last_status = 0;

    delay(200);
}

// キーの入力状態を取得
void key_read() {
    int c, i, j, k, n;
    int m[4];

    // 入力データクリア
    for (i=0; i<INPUT_KEY_LENGTH; i++) {
      input_key[i] = 0;
    }

    for (k=0; k<2; k++) {
        // IOエキスパンダ入力取得
        iomcp[k].writeGPIO(0xfe, 0);
        delay(3);
        m[0] = iomcp[k].readGPIOAB();
        iomcp[k].writeGPIO(0xfd, 0);
        delay(3);
        m[1] = iomcp[k].readGPIOAB();
        
        // 現在の入力情報取得
        for (i=0; i<16; i++) {
            c = 1 << i;
            n = 0;
            if (i == 0 || i == 1) continue;
            for (j=0; j<2; j++) {
                input_key[(k * 32) + i + n] = (!(m[j] & c));
                n += 16;
            }
        }
    }

    // ロータリエンコーダ
    Wire.requestFrom(0x45, 1);
    i = Wire.read(); // データ受け取る
    if (i) input_key[0] = 1;

    // ダイレクトキー入力取得
    if (!digitalRead(21)) input_key[0] = 1;
    if (!digitalRead(19)) input_key[0] = 1;
    if (!digitalRead(22)) input_key[0] = 1;
    if (!digitalRead(25)) input_key[0] = 1;
    if (!digitalRead(32)) input_key[0] = 1;

}

// 動作ループ
void loop() {
    int i;
    bool change_flag = false;

    // キーの入力情報取得
    key_read();

    // 前回の入力から変更があったかチェック
    for (i=0; i<INPUT_KEY_LENGTH; i++) {
        if (input_key[i]) change_flag = true;
    }

    // 変更が無ければ何もしない
    if (!change_flag) {
        if (last_status > 0) {
            status_rgb->setPixelColor(0, status_rgb->Color(10, 10, 0));
            status_rgb->show();
            last_status = 0;
        }
        delay(20);
        return;
    }

    if (last_status == 0) {
        status_rgb->setPixelColor(0, status_rgb->Color(0, 0, 10));
        status_rgb->show();
        last_status = 1;
    }

    delay(20);
}
