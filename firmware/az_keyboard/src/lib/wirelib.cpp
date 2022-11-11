#include "Arduino.h"
#include "wirelib.h"


// コンストラクタ
Wirelib::Wirelib() {
}

// ロータリエンコーダの入力取得
uint8_t Wirelib::read_rotary(int addr) {
    Wire.requestFrom(addr, 1); // 指定したアドレスのTinyにデータ取得要求
    return Wire.read(); // データ受け取る
};

void Wirelib::set_az1uball_read_type(int addr, int set_mode) {
    Wire.beginTransmission(addr);
    if (set_mode) {
        Wire.write(0x91);
    } else {
        Wire.write(0x90);
    }
    Wire.endTransmission();
};

// 1Uトラックボール PIM447 から入力を取得する
tracktall_pim447_data Wirelib::read_trackball_pim447(int addr) {
    // int wire_err;
    tracktall_pim447_data r;
    // Wire.beginTransmission(addr);
    // Wire.write(0x04);
    // wire_err = Wire.endTransmission();
    Wire.requestFrom(addr, 5);
    r.left = Wire.read();  // 左回転
    r.right = Wire.read(); // 右回転
    r.up = Wire.read();    // 上回転
    r.down = Wire.read();  // 下回転
    r.click = Wire.read(); // スイッチ
    return r;
};
