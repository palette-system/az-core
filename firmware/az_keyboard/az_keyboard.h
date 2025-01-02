#ifndef AzKeyboard_h
#define AzKeyboard_h

#include "az_common.h"


// クラスの定義
class AzKeyboard
{
    public:
        AzKeyboard();   // コンストラクタ
        void begin_keyboard(); // キーボード初期処理
        void start_keyboard(); // キーボードとして処理開始
        void loop_exec();         // キーボード定期処理
        void key_action_exec(); // ステータスが変更されたキーのアクションを実行する
        void key_repeat_exec(); // キー連打処理
        void send_webhook(char *jstr); // WEBフックを送信する
        void send_string(char *send_char); // テキストを送信する
        void hold_press(int hold, int key_num); // hold キーを押す
        void key_down_action(int key_num, short press_type); // キーが押された時のアクション
        void key_up_action(int key_num, short press_type); // キーが離された時のアクション
        void tap_key_disable_all(); // tap / hold の単押しを無効化する
        void press_data_reset(); // 入力状態を全てクリアする
        void press_key_list_push(int action_type, int key_num, int key_id, int layer_id, int repeat_interval, short press_type); // キーを押しましたリストに追加
        void move_mouse_loop(); // マウス移動処理
        void power_saving_loop(); // 省電力モード用ループ
        void press_data_clear(); // キーを押したままリストをクリア
    
    private:
};

#endif
