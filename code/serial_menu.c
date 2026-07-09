#include "zf_common_headfile.h"
#include "serial_menu.h"
#include "display.h"
#include "guandao.h"

typedef enum
{
    SERIAL_MENU_PAGE_MAIN = 0,
    SERIAL_MENU_PAGE_MODE,
    SERIAL_MENU_PAGE_ROUTE,
    SERIAL_MENU_PAGE_PARAM,
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
        case SERIAL_MENU_PAGE_ROUTE:
            return 3;
        case SERIAL_MENU_PAGE_PARAM:
            return 3;
        default:
            return 1;
    }
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
            serial_menu_print_item(0, "Car_Go");
            serial_menu_print_item(1, "Mode_Choice");
            serial_menu_print_item(2, "Show_Route");
            serial_menu_print_item(3, "Parameter");
            break;

        case SERIAL_MENU_PAGE_MODE:
            serial_menu_write("[SERIAL-MENU] Mode_Choice\r\n");
            serial_menu_print_item(0, "Run_Mode");
            serial_menu_print_item(1, "Record_Mode");
            serial_menu_print_item(2, "Drive_Mode");
            break;

        case SERIAL_MENU_PAGE_ROUTE:
            serial_menu_write("[SERIAL-MENU] Show_Route\r\n");
            serial_menu_print_item(0, "passage");
            serial_menu_print_item(1, "portion_3");
            serial_menu_print_item(2, "portion_2_run");
            break;

        case SERIAL_MENU_PAGE_PARAM:
            serial_menu_write("[SERIAL-MENU] Parameter\r\n");
            serial_menu_print_item(0, "PID");
            serial_menu_print_item(1, "Control");
            serial_menu_print_item(2, "Status");
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
        main_mode = Guandao_Portion2_Recode;
        route_setting_choice = 1;
        conrtol_mode = IDLE;
    }
    else
    {
        main_mode = Guandao_Drive;
        route_setting_choice = 1;
        conrtol_mode = GUANDAO;
    }

    serial_menu_render();
}

static void serial_menu_enter(void)
{
    switch(serial_menu_page)
    {
        case SERIAL_MENU_PAGE_MAIN:
            if(serial_menu_cursor == 0)
            {
                serial_menu_apply_mode(Guandao_Voice);
            }
            else if(serial_menu_cursor == 1)
            {
                serial_menu_set_page(SERIAL_MENU_PAGE_MODE);
            }
            else if(serial_menu_cursor == 2)
            {
                serial_menu_set_page(SERIAL_MENU_PAGE_ROUTE);
            }
            else
            {
                serial_menu_set_page(SERIAL_MENU_PAGE_PARAM);
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
            break;

        case SERIAL_MENU_PAGE_ROUTE:
            if(serial_menu_cursor == 0)
            {
                route_setting_choice = 1;
            }
            else if(serial_menu_cursor == 1)
            {
                route_setting_choice = 2;
            }
            else
            {
                route_setting_choice = 3;
            }
            serial_menu_render();
            break;

        case SERIAL_MENU_PAGE_PARAM:
            serial_menu_write("[SERIAL-MENU] Parameter edit stays on the physical menu.\r\n");
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
