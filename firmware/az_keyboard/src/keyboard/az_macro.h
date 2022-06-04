// AZ-Macro �p�ݒ�

// �����N�����̃f�t�H���g�ݒ�
#include "setting_azmacro_default.h"

// �L�[�{�[�h�̒�`
#define KEYBOARD_AZMACRO

// ESP32�̎�� (0 = ESP32 / 1 = ESP32-WROVER)
#define  AZESP32_TYPE  0

// �������ɕێ�����L�[�̐�(���������m�ۂ���T�C�Y)
#define KEY_INPUT_MAX  16

// ���C���[�؂�ւ������������e��
#define PRESS_KEY_MAX 16

// �}�E�X�ړ��{�^�������������e��
#define PRESS_MOUSE_MAX 4

// WIFI�A�N�Z�X�|�C���g�̖��O
#define WIFI_AP_SSI_NAME    "AZ-Macro"

// Bluetooth �Ō����������\������閼�O
#define BLUETOOTH_SEARCH_NAME    "az-macro"

// EEPROM �ǂݍ��ݎ��̃T�C�Y
#define EEPROM_BUF_SIZE   256

// WEB�t�b�N�p�̃o�b�t�@�T�C�Y
#define WEBFOOK_BUF_SIZE 512

// JSON�o�b�t�@��PSRAM���g�����̃t���O
#define SETTING_JSON_BUF_PSRAM 0

// �ݒ�JSON�̃o�b�t�@�T�C�Y
#define SETTING_JSON_BUF_SIZE 51200

// �ËL�{�^���ňËL�ł��鐔
#define ANKEY_DATA_MAX_LENGTH  32

// Neopix�f�[�^���M���g��(400 or 800)
#define AZ_NEO_KHZ 400

// remap�p �f�t�H���g�� vid  pid
#define BLE_HID_VID  0xE502
#define BLE_HID_PID  0x0200

// remap�p �f�[�^���M���̃L�[��
#define REMAP_KEY_MAX  12

// remap�pcol,row (remap�pJSON���Œ�`����ROW��COL)
#define REMAP_ROW  1
#define REMAP_COL  12

