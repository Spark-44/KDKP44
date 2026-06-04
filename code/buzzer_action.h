#ifndef CODE_BUZZER_ACTION_H_
#define CODE_BUZZER_ACTION_H_

#include "zf_common_typedef.h"

void buzzer_action_once(uint16 on_ms);
void buzzer_action_repeat(uint8 times, uint16 on_ms, uint16 off_ms);
void buzzer_action_long_short(void);
void buzzer_action_rapid(void);
void buzzer_action_alarm(void);
void buzzer_action_from_voice_cmd(uint8 cmd_id);

#endif
