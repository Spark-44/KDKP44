#include "zf_common_headfile.h"
#include "serial_menu.h"
#include "display.h"
#include "guandao.h"
#include "peripheral.h"
#include "rear_motor/rear_motor.h"

typedef enum
{
    SERIAL_MENU_PAGE_MAIN = 0,
    SERIAL_MENU_PAGE_MODE,
    SERIAL_MENU_PAGE_RUN_ROUTE,
    SERIAL_MENU_PAGE_RECORD_CONTROL,
    SERIAL_MENU_PAGE_RECORD_ROUTE,
    SERIAL_MENU_PAGE_DRIVE_CONTROL,
} serial_menu_page_t;

static uint8 serial_menu_active = 0;
static serial_menu_page_t serial_menu_page = SERIAL_MENU_PAGE_MAIN;
static uint8 serial_menu_cursor = 0;

static void serial_menu_write(const char *text)
{
    uart_write_string(DEBUG_UART_INDEX, text);
}

static const char *serial_menu_mode_name(void)
{
    switch(main_mode)
    {
        case Guandao_Voice:
            return "RUN";
        case Guandao_Portion2_Recode:
            return "RECORD";
        case Guandao_Drive:
            return "DRIVE";
        default:
            return "UNKNOWN";
    }
}

static void serial_menu_print_item(uint8 index, const char *text)
{
    serial_menu_write(index == serial_menu_cursor ? "-> " : "   ");
    serial_menu_write(text);
    serial_menu_write("\r\n");
}

static uint8 serial_menu_item_count(void)
{
    switch(serial_menu_page)
    {
        case SERIAL_MENU_PAGE_MAIN:
            return 4;
        case SERIAL_MENU_PAGE_MODE:
            return 3;
        case SERIAL_MENU_PAGE_RUN_ROUTE:
            return PORTION2_ROUTE_COUNT + 1U;
        case SERIAL_MENU_PAGE_RECORD_CONTROL:
            return 4;
        case SERIAL_MENU_PAGE_RECORD_ROUTE:
            return PORTION2_ROUTE_COUNT;
        case SERIAL_MENU_PAGE_DRIVE_CONTROL:
            return 3;
        default:
            return 1;
    }
}

static void serial_menu_print_route_item(uint8 index, const char *prefix)
{
    char line[40];
    sprintf(line, "%s%02u", prefix, (unsigned int)(index + 1U));
    serial_menu_print_item(index, line);
}

static void serial_menu_render(void)
{
    char line[96];

    serial_menu_write("\r\n[SERIAL-MENU] M/w/s/e/b = menu/up/down/enter/back\r\n");
    sprintf(line, "[SERIAL-MENU] mode=%s route=%u control=%u\r\n",
            serial_menu_mode_name(),
            (unsigned int)(route_setting_choice),
            (unsigned int)(conrtol_mode));
    serial_menu_write(line);

    switch(serial_menu_page)
    {
        case SERIAL_MENU_PAGE_MAIN:
            serial_menu_write("[SERIAL-MENU] Menu_Main\r\n");
            serial_menu_print_item(0, "Mode_Choice");
            serial_menu_print_item(1, "Run_Route");
            serial_menu_print_item(2, "Record_Control");
            serial_menu_print_item(3, "Drive_Control");
            break;

        case SERIAL_MENU_PAGE_MODE:
            serial_menu_write("[SERIAL-MENU] Mode_Choice\r\n");
            serial_menu_print_item(0, "Run_Mode");
            serial_menu_print_item(1, "Record_Mode");
            serial_menu_print_item(2, "Drive_Mode");
            break;

        case SERIAL_MENU_PAGE_RUN_ROUTE:
            serial_menu_write("[SERIAL-MENU] Run_Route\r\n");
            for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
            {
                serial_menu_print_route_item(i, "Run_Route_");
            }
            serial_menu_print_item(PORTION2_ROUTE_COUNT, "Stop_Run");
            break;

        case SERIAL_MENU_PAGE_RECORD_CONTROL:
            serial_menu_write("[SERIAL-MENU] Record_Control\r\n");
            serial_menu_print_item(0, "Select_Record_Route");
            serial_menu_print_item(1, "Start_Stop_Record");
            serial_menu_print_item(2, "Clear_Current_Route");
            serial_menu_print_item(3, "Save_Current_Route");
            break;

        case SERIAL_MENU_PAGE_RECORD_ROUTE:
            serial_menu_write("[SERIAL-MENU] Record_Route\r\n");
            for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
            {
                serial_menu_print_route_item(i, "Record_Route_");
            }
            break;

        case SERIAL_MENU_PAGE_DRIVE_CONTROL:
            serial_menu_write("[SERIAL-MENU] Drive_Control\r\n");
            serial_menu_print_item(0, "K1_Speed_Down");
            serial_menu_print_item(1, "K2_Speed_Up");
            serial_menu_print_item(2, "K4_Back_Record");
            break;

        default:
            serial_menu_page = SERIAL_MENU_PAGE_MAIN;
            serial_menu_cursor = 0;
            serial_menu_render();
            break;
    }
}

static void serial_menu_set_page(serial_menu_page_t page)
{
    serial_menu_page = page;
    serial_menu_cursor = 0;
    serial_menu_render();
}

static void serial_menu_apply_mode(Mode_Choice mode)
{
    out_v_l = 0.0f;
    out_v_r = 0.0f;
    out_servo = 0.0f;

    if(mode == Guandao_Voice)
    {
        main_mode = Guandao_Voice;
        route_setting_choice = 3;
        conrtol_mode = GUANDAO;
    }
    else if(mode == Guandao_Portion2_Recode)
    {
        portion2_run_stop();
        rear_motor_stop();
        portion2_record_enter_mode();
        main_mode = Guandao_Portion2_Recode;
        route_setting_choice = 1;
        conrtol_mode = IDLE;
    }
    else
    {
        portion2_run_stop();
        rear_motor_stop();
        main_mode = Guandao_Drive;
        route_setting_choice = 1;
        conrtol_mode = GUANDAO;
    }

}

static void serial_menu_enter(void)
{
    switch(serial_menu_page)
    {
        case SERIAL_MENU_PAGE_MAIN:
            if(serial_menu_cursor == 0)
            {
                serial_menu_set_page(SERIAL_MENU_PAGE_MODE);
            }
            else if(serial_menu_cursor == 1)
            {
                serial_menu_set_page(SERIAL_MENU_PAGE_RUN_ROUTE);
            }
            else if(serial_menu_cursor == 2)
            {
                serial_menu_set_page(SERIAL_MENU_PAGE_RECORD_CONTROL);
            }
            else
            {
                serial_menu_set_page(SERIAL_MENU_PAGE_DRIVE_CONTROL);
            }
            break;

        case SERIAL_MENU_PAGE_MODE:
            if(serial_menu_cursor == 0)
            {
                serial_menu_apply_mode(Guandao_Voice);
            }
            else if(serial_menu_cursor == 1)
            {
                serial_menu_apply_mode(Guandao_Portion2_Recode);
            }
            else
            {
                serial_menu_apply_mode(Guandao_Drive);
            }
            serial_menu_render();
            break;

        case SERIAL_MENU_PAGE_RUN_ROUTE:
            serial_menu_apply_mode(Guandao_Voice);
            if(serial_menu_cursor < PORTION2_ROUTE_COUNT)
            {
                portion2_run_select_route(serial_menu_cursor);
            }
            else
            {
                portion2_run_stop();
                rear_motor_stop();
            }
            serial_menu_render();
            break;

        case SERIAL_MENU_PAGE_RECORD_CONTROL:
            serial_menu_apply_mode(Guandao_Portion2_Recode);
            if(serial_menu_cursor == 0)
            {
                serial_menu_set_page(SERIAL_MENU_PAGE_RECORD_ROUTE);
                return;
            }
            else if(serial_menu_cursor == 1)
            {
                portion2_record_remote_start_stop_request();
            }
            else if(serial_menu_cursor == 2)
            {
                portion2_record_remote_clear_request();
            }
            else
            {
                portion2_record_remote_save_request();
            }
            serial_menu_render();
            break;

        case SERIAL_MENU_PAGE_RECORD_ROUTE:
            serial_menu_apply_mode(Guandao_Portion2_Recode);
            portion2_record_select_route(serial_menu_cursor);
            serial_menu_render();
            break;

        case SERIAL_MENU_PAGE_DRIVE_CONTROL:
            serial_menu_apply_mode(Guandao_Drive);
            if(serial_menu_cursor == 0)
            {
                key1_flag = 1;
            }
            else if(serial_menu_cursor == 1)
            {
                key2_flag = 1;
            }
            else
            {
                portion2_run_stop();
                rear_motor_stop();
                portion2_record_enter_mode();
                main_mode = Guandao_Portion2_Recode;
                route_setting_choice = 1;
                conrtol_mode = IDLE;
            }
            serial_menu_render();
            break;

        default:
            serial_menu_set_page(SERIAL_MENU_PAGE_MAIN);
            break;
    }
}

uint8 serial_menu_handle_byte(uint8 data)
{
    if(data == '\r' || data == '\n')
    {
        return serial_menu_active;
    }

    if(data == 'M')
    {
        serial_menu_active = 1;
        serial_menu_set_page(SERIAL_MENU_PAGE_MAIN);
        return 1;
    }

    if(!serial_menu_active)
    {
        return 0;
    }

    if(data == 'w')
    {
        if(serial_menu_cursor == 0)
        {
            serial_menu_cursor = serial_menu_item_count() - 1;
        }
        else
        {
            serial_menu_cursor--;
        }
        serial_menu_render();
        return 1;
    }
    if(data == 's')
    {
        serial_menu_cursor++;
        if(serial_menu_cursor >= serial_menu_item_count())
        {
            serial_menu_cursor = 0;
        }
        serial_menu_render();
        return 1;
    }
    if(data == 'e')
    {
        serial_menu_enter();
        return 1;
    }
    if(data == 'b')
    {
        if(serial_menu_page == SERIAL_MENU_PAGE_MAIN)
        {
            serial_menu_active = 0;
            serial_menu_write("[SERIAL-MENU] closed\r\n");
        }
        else
        {
            serial_menu_set_page(SERIAL_MENU_PAGE_MAIN);
        }
        return 1;
    }

    return 0;
}
