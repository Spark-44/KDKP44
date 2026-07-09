#ifndef CODE_REMOTE_CONTROL_H_
#define CODE_REMOTE_CONTROL_H_

#include "zf_common_headfile.h"

void remote_control_init(void);
uint8 remote_control_task(void);
void remote_control_stop(void);
uint8 remote_control_is_active(void);

#endif
