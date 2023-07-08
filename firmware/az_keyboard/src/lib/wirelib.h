#ifndef wirelib_h
#define wirelib_h


#include <Wire.h>


// 1U トラックボール PIM447 から取得したデータ
struct tracktall_pim447_data {
    uint8_t left;  // 左移動
    uint8_t right; // 右移動
    uint8_t up;    // 上移動
    uint8_t down;  // 下移動
    uint8_t click; // スイッチ
};

// AZエクスパンダの入力データ
struct azexpanda_data {
    uint8_t key_input[16];
};


// クラスの定義
class Wirelib
{
	public:
		Wirelib();   // コンストラクタ
        int write(int addr, uint8_t *send_data, int send_len); // I2Cへデータ送信
        int read(int addr, uint8_t *read_data, int read_len); // I2Cからデータ読み込み
		uint8_t read_rotary(int addr); // ロータリエンコーダの入力取得
        void set_az1uball_read_type(int addr, int set_mode); // AZ1UBALLのデータ取得タイプを設定
		tracktall_pim447_data read_trackball_pim447(int addr); // 1U トラックボール PIM447 の入力取得
        azexpanda_data read_azexpanda_key(int addr); // AZエクスパンダのキー入力状態を取得
		
};

#endif
