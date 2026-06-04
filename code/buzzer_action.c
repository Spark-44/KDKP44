#include "buzzer_action.h"
#include "offline_voice.h"
#include "peripheral.h"
#include "zf_common_headfile.h"

#define BUZZER_ACTION_ONE_SECOND_MS       (1000u)
#define BUZZER_ACTION_THREE_SECONDS_MS    (3000u)
#define BUZZER_ACTION_REPEAT_GAP_MS       (1000u)
#define BUZZER_ACTION_RAPID_ON_MS         (500u)
#define BUZZER_ACTION_RAPID_OFF_MS        (500u)
#define BUZZER_ACTION_RAPID_TIMES         (6u)
#define BUZZER_ACTION_ALARM_TIMES         (6u)
#define BUZZER_ACTION_NORMAL_FREQ_HZ      (2000u)
#define BUZZER_ACTION_ALARM_LOW_FREQ_HZ   (500u)
#define BUZZER_ACTION_ALARM_HIGH_FREQ_HZ  (1000u)

void buzzer_action_once(uint16 on_ms)
{
    Buzzer_tone((int)on_ms, BUZZER_ACTION_NORMAL_FREQ_HZ);
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
    buzzer_action_once(BUZZER_ACTION_ONE_SECOND_MS);
    system_delay_ms(BUZZER_ACTION_REPEAT_GAP_MS);
    buzzer_action_once(BUZZER_ACTION_THREE_SECONDS_MS);
}

void buzzer_action_rapid(void)
{
    buzzer_action_repeat(BUZZER_ACTION_RAPID_TIMES, BUZZER_ACTION_RAPID_ON_MS, BUZZER_ACTION_RAPID_OFF_MS);
}

void buzzer_action_alarm(void)
{
    uint8 i;

    for(i = 0; i < BUZZER_ACTION_ALARM_TIMES; i++)
    {
        uint16 freq_hz = ((i & 0x01u) == 0u) ? BUZZER_ACTION_ALARM_LOW_FREQ_HZ : BUZZER_ACTION_ALARM_HIGH_FREQ_HZ;
        Buzzer_tone((int)BUZZER_ACTION_ONE_SECOND_MS, freq_hz);
    }
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
            buzzer_action_repeat(2, BUZZER_ACTION_ONE_SECOND_MS, BUZZER_ACTION_REPEAT_GAP_MS);
            break;

        case OFFLINE_VOICE_CMD_HORN_3X:
            buzzer_action_repeat(3, BUZZER_ACTION_ONE_SECOND_MS, BUZZER_ACTION_REPEAT_GAP_MS);
            break;

        case OFFLINE_VOICE_CMD_HORN_4X:
            buzzer_action_repeat(4, BUZZER_ACTION_ONE_SECOND_MS, BUZZER_ACTION_REPEAT_GAP_MS);
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
