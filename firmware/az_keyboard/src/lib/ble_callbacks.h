#ifndef BleCallbacks_h
#define BleCallbacks_h
#include "sdkconfig.h"


#if defined(CONFIG_BT_ENABLED)


#include "hid_common.h"
#include "../../az_common.h"



// HidrawCallback
void HidrawCallbackExec(int data_length);

#endif // CONFIG_BT_ENABLED
#endif
