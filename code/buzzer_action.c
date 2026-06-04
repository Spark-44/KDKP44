#include "buzzer_action.h"
#include "offline_voice.h"
#include "peripheral.h"
#include "zf_common_headfile.h"

#define BUZZER_ACTION_SHORT_MS        (200u)
#define BUZZER_ACTION_MEDIUM_MS       (300u)
#define BUZZER_ACTION_LONG_MS         (800u)
#define BUZZER_ACTION_GAP_MS          (150u)
#define BUZZER_ACTION_RAPID_ON_MS     (80u)
#define BUZZER_ACTION_RAPID_OFF_MS    (80u)
#define BUZZER_ACTION_ALARM_ON_MS     (400u)
#define BUZZER_ACTION_ALARM_OFF_MS    (200u)

void buzzer_action_once(uint16 on_ms)
{
    Buzzer_check((int)on_ms);
}

void buzzer_action_repeat(uint8 times, uint16 on_ms, uint16 off_ms)
{
    uint8 i;

    for(i = 0; i < times; i++)
    {
        Buzzer_check((int)on_ms);
        if(i + 1u < times)
        {
            system_delay_ms(off_ms);
        }
    }
}

void buzzer_action_long_short(void)
{
    Buzzer_check((int)BUZZER_ACTION_LONG_MS);
    system_delay_ms(BUZZER_ACTION_GAP_MS);
    Buzzer_check((int)BUZZER_ACTION_SHORT_MS);
}

void buzzer_action_rapid(void)
{
    buzzer_action_repeat(6, BUZZER_ACTION_RAPID_ON_MS, BUZZER_ACTION_RAPID_OFF_MS);
}

void buzzer_action_alarm(void)
{
    buzzer_action_repeat(3, BUZZER_ACTION_ALARM_ON_MS, BUZZER_ACTION_ALARM_OFF_MS);
}

void buzzer_action_from_voice_cmd(uint8 cmd_id)
{
    switch(cmd_id)
    {
        case OFFLINE_VOICE_CMD_HORN_1S:
            buzzer_action_once(1000u);
            break;

        case OFFLINE_VOICE_CMD_HORN_2S:
            buzzer_action_once(2000u);
            break;

        case OFFLINE_VOICE_CMD_HORN_3S:
            buzzer_action_once(3000u);
            break;

        case OFFLINE_VOICE_CMD_HORN_2X:
            buzzer_action_repeat(2, BUZZER_ACTION_MEDIUM_MS, BUZZER_ACTION_GAP_MS);
            break;

        case OFFLINE_VOICE_CMD_HORN_3X:
            buzzer_action_repeat(3, BUZZER_ACTION_MEDIUM_MS, BUZZER_ACTION_GAP_MS);
            break;

        case OFFLINE_VOICE_CMD_HORN_4X:
            buzzer_action_repeat(4, BUZZER_ACTION_MEDIUM_MS, BUZZER_ACTION_GAP_MS);
            break;

        case OFFLINE_VOICE_CMD_HORN_LONG_SHORT:
            buzzer_action_long_short();
            break;

        case OFFLINE_VOICE_CMD_HORN_RAPID:
            buzzer_action_rapid();
            break;

        case OFFLINE_VOICE_CMD_HORN_ALARM:
            buzzer_action_alarm();
            break;

        default:
            break;
    }
}
