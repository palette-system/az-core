
#include "hid_common.h"

// remapへ返事を返す用のバッファ
uint8_t remap_buf[36];

// ファイル送受信用バッファ
uint8_t send_buf[36];
char target_file_path[36];
char second_file_path[36];

// ファイル保存用バッファ
uint8_t *save_file_data;
int save_file_length;
uint8_t save_file_step;
uint8_t save_file_index;
bool save_step_flag[8];

// remapで設定変更があったかどうかのフラグ
uint8_t remap_change_flag;
