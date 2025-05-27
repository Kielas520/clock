#include "stm32f10x.h"                  
#include "Delay.h"
#include "global.h"
#include "OLED.h"
#include "timer1.h"
#include "Key.h"
#include "Buzzer.h"

// 每月天数数组
const uint8_t days_in_month_normal[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const uint8_t days_in_month_leap[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 全局变量定义

Time_t time = {0, 0, 0, 1, 1, 2025}; // 定义当前时间，初始值为 2025年1月1日 00:00:00
Time_t clock = {0, 0, 0, 1, 1, 2025}; // 定义闹钟时间，初始值同上
Time_t end = {0, 0, 0, 0, 0, 0}; // 定义倒计时时间，初始值为 0
Time_t countdown = {0, 0, 0, 0, 0, 0}; // 定义倒计时时间，初始值为 0
uint8_t days_in_current_month = 30; // 存储当前月份的天数
Smart_Clock_t smart_clock;
uint8_t alarm_triggered = 0;
uint8_t countdown_start = 0;
uint8_t choose = 0; // 当前选择项
int8_t choose_buff = 0; // 选择缓冲变量
uint8_t key_states[4] = {0}; // 存储按键状态
//判断时间是否为相等
uint8_t is_time_equal(Time_t *t1, Time_t *t2) {
    return (t1->hour == t2->hour && t1->min == t2->min && t1->sec == t2->sec &&
            t1->day == t2->day && t1->month == t2->month && t1->year == t2->year);
}

// 判断是否为闰年
uint8_t is_leap_year(uint16_t year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 获取当前月份的天数
uint8_t get_days_in_month(uint8_t month, uint16_t year) {
    if (month < 1 || month > 12) return 0; // 防止越界
    if (is_leap_year(year)) {
        return days_in_month_leap[month - 1];
    } else {
        return days_in_month_normal[month - 1];
    }
}

// 时间更新函数
void UpdateTime(void) {
    if (time.sec > 59) {
        time.min++;
        time.sec = 0;
    } else if (time.sec < 0) {
        time.min--;
        time.sec = 59;
    }
    if (time.min > 59) {
        time.hour++;
        time.min = 0;
    } else if (time.min < 0) {
        time.hour--;
        time.min = 59;
    }
    if (time.hour > 23) {
        time.day++;
        time.hour = 0;
        days_in_current_month = get_days_in_month(time.month, time.year);
        if (time.day > days_in_current_month) {
            time.month++;
            time.day = 1; 
            if (time.month > 12) {
                time.year++;
                time.month = 1; 
            }
        }
    } else if (time.hour < 0) {
        time.day--;
        time.hour = 23;
        if (time.day < 1) {
            time.month--;
            if (time.month < 1) {
                time.year--;
                time.month = 12;
            }
            time.day = get_days_in_month(time.month, time.year);
        }
    }
}


// 倒计时更新函数
void UpdateCountdownTime(void) {
    if (countdown.sec > 59) {
        countdown.min++;
        countdown.sec = 0;
    } else if (countdown.sec < 0) {
        countdown.min--;
        countdown.sec = 59;
    }
    if (countdown.min > 59) {
        countdown.hour++;
        countdown.min = 0;
    } else if (time.min < 0) {
        countdown.hour--;
        countdown.min = 59;
    }
    if (countdown.hour > 23) {
        countdown.hour = 0;
        
    } else if (countdown.hour < 0) {
        countdown.hour = 23;
    }
}

void ShowTime(void) {
    OLED_ShowString(4, 1, "--Mode----Show--");
    OLED_ShowString(2, 1, "------Time------");
    OLED_ShowString(3, 1, "  ");
    OLED_ShowNum(3, 3, time.hour, 2);
    OLED_ShowString(3, 5, " : ");
    OLED_ShowNum(3, 8, time.min, 2);
    OLED_ShowString(3, 10, " : ");
    OLED_ShowNum(3, 13, time.sec, 2);
    OLED_ShowString(3, 15, "  ");
}

void ShowDate(void) {
    OLED_ShowString(4, 1, "--Mode----Show--");
    OLED_ShowString(2, 1, "------Date------");
    OLED_ShowString(3, 1, " ");
    OLED_ShowNum(3, 2, time.year, 4);
    OLED_ShowString(3, 6, " : ");
    OLED_ShowNum(3, 9, time.month, 2);
    OLED_ShowString(3, 11, " : ");
    OLED_ShowNum(3, 14, time.day, 2);
    OLED_ShowString(3, 16, " ");
}

// 调试显示按键状态
void ShowKeyStates(void) {
    OLED_ShowString(1, 1, "Keys:");
    OLED_ShowNum(1, 6, key_states[0], 1); // PB13 (smart_clock.key_function.set_mode)
    OLED_ShowNum(1, 8, key_states[1], 1); // PB15 (smart_clock.key_function.trigger_up)
    OLED_ShowNum(1, 10, key_states[2], 1); // PA9 (smart_clock.key_function.trigger_down)
    OLED_ShowNum(1, 12, key_states[3], 1); // PA11 (smart_clock.key_function.confirm)
    OLED_ShowString(1, 14, "M:");
    OLED_ShowNum(1, 16, smart_clock.key_function.set_mode, 1); // 显示smart_clock.key_function.set_mode状态
    OLED_ShowString(2, 1, "Cd:");
    OLED_ShowNum(2, 4, smart_clock.countdown_mode, 1); // 显示smart_clock.countdown_mode状态
}

int main(void) {
    Buzzer_Init();
    Key_Init();
    OLED_Init();        
    TIM1_Init();
    OLED_ShowString(1, 1, "Kielas");                        

    while (1) 
    {
		if (is_time_equal(&time, &clock)) 
        {
            if (alarm_triggered) 
            {
                    Buzzer_ON();
					OLED_ShowString(1, 1, "Kielas-CLK-TIME-");
                    Delay_ms(1000);
					OLED_ShowString(1, 1, "Kielas          ");
                    Buzzer_OFF();
                    alarm_triggered = 0;
            }
		}
		else if (is_time_equal(&end, &countdown)) 
        {
            if (countdown_start) 
            {
                    Buzzer_ON();
					OLED_ShowString(1, 1, "Kielas-TIM-OVER-");
                    Delay_ms(1000);
					OLED_ShowString(1, 1, "Kielas          ");
                    Buzzer_OFF();
                    countdown_start = 0;
            }
		}

        // 获取按键状态
        Key_GetNum(key_states);
        // 更新当前时间
        UpdateTime();
        UpdateCountdownTime();
        // 调试：取消注释以启用
        // ShowKeyStates();
        
        //根据设置模式执行不同操作
        if (smart_clock.key_function.set_mode == 0) 
        {  
            if (smart_clock.key_function.confirm) 
            {
								smart_clock.show_clock_mode=!smart_clock.show_clock_mode;
                smart_clock.key_function.confirm = 0;
            }
						
            if (smart_clock.show_clock_mode == 0) 
            {
                ShowTime();
            }
            else if (smart_clock.show_clock_mode == 1) 
            {
                ShowDate();
            }
        } 
        else if (smart_clock.key_function.set_mode == 1) 
        {
            if (choose == 0) 
            {
                OLED_ShowString(4, 1, "--Mode----Set---");
                OLED_ShowString(2, 1, "-----Choose-----");
                if (smart_clock.key_function.trigger_up) 
                {
                    smart_clock.key_function.trigger_up = 0;
                    choose_buff++;
                } 
                else if (smart_clock.key_function.trigger_down) 
                {
                    smart_clock.key_function.trigger_down = 0;
                    choose_buff--;
                }
								
                if(choose_buff < 0)
                {
                    choose_buff = 2;
                }
                choose_buff = choose_buff % 3;
                if (choose_buff == 0) 
                {
                    OLED_ShowString(3, 1, "<Clk>--Cd---Tim-");
                    if (smart_clock.key_function.confirm) 
                    {
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_clock_mode = 1;
                        choose = 1;
                    }
                } 
                else if (choose_buff == 1) 
                {
                    OLED_ShowString(3, 1, "-Clk--<Cd>--Tim-");
                    if (smart_clock.key_function.confirm) 
                    {
                        smart_clock.key_function.confirm = 0;
                        smart_clock.countdown_mode = 1;
                        choose = 1;
                    }
                }
                else if (choose_buff == 2) 
                {
                    OLED_ShowString(3, 1, "-Clk---Cd--<Tim>");
                    if (smart_clock.key_function.confirm) 
                    {
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_time_mode = 1;
                        choose = 1;
                    }
                }
            }
            if (smart_clock.set_time_mode >= SET_TIME_HOUR && choose == 1) 
            {
                if (smart_clock.set_time_mode == 1) 
                {
                    if (smart_clock.key_function.trigger_up) 
                    {
                        time.hour++;
                        smart_clock.key_function.trigger_up = 0;
                    }
                    else if (smart_clock.key_function.trigger_down) 
                    {
                        time.hour--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    UpdateTime();
                    OLED_ShowString(2, 1, "------Hour------");
                    OLED_ShowString(3, 1, " <");
                    OLED_ShowNum(3, 3, time.hour, 2);
                    OLED_ShowString(3, 5, ">: ");
                    OLED_ShowNum(3, 8, time.min, 2);
                    OLED_ShowString(3, 10, " : ");
                    OLED_ShowNum(3, 13, time.sec, 2);
                    OLED_ShowString(3, 15, "  ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_time_mode++;
                    }
                }
                if (smart_clock.set_time_mode == SET_TIME_MIN && choose == 1) 
                {
                    if (smart_clock.key_function.trigger_up)
                    {
                        time.min++;
                        smart_clock.key_function.trigger_up = 0;
                    }
                    else if (smart_clock.key_function.trigger_down) 
                    {
                        time.min--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    UpdateTime();
                    OLED_ShowString(2, 1, "-------Min------");
                    OLED_ShowString(3, 1, "  ");
                    OLED_ShowNum(3, 3, time.hour, 2);
                    OLED_ShowString(3, 5, " :<");
                    OLED_ShowNum(3, 8, time.min, 2);
                    OLED_ShowString(3, 10, ">: ");
                    OLED_ShowNum(3, 13, time.sec, 2);
                    OLED_ShowString(3, 15, "  ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_time_mode++;
                    }
                }
                if (smart_clock.set_time_mode == SET_TIME_SEC && choose == 1) 
								{
                    if (smart_clock.key_function.trigger_up) 
										{
                        time.sec++;
                        smart_clock.key_function.trigger_up = 0;
                    }
										else if (smart_clock.key_function.trigger_down) 
										{
                        time.sec--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    UpdateTime();
                    OLED_ShowString(2, 1, "-------Sec------");
                    OLED_ShowString(3, 1, "  ");
                    OLED_ShowNum(3, 3, time.hour, 2);
                    OLED_ShowString(3, 5, " : ");
                    OLED_ShowNum(3, 8, time.min, 2);
                    OLED_ShowString(3, 10, " :<");
                    OLED_ShowNum(3, 13, time.sec, 2);
                    OLED_ShowString(3, 15, "> ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_time_mode++;
                    }
                }
                if (smart_clock.set_time_mode == SET_TIME_DAY && choose == 1) 
                {
                    if (smart_clock.key_function.trigger_up) 
                    {
                        time.day++;
                        days_in_current_month = get_days_in_month(time.month, time.year);
                        if (time.day > days_in_current_month) time.day = 1;
                        smart_clock.key_function.trigger_up = 0;
                    }
                    else if (smart_clock.key_function.trigger_down) 
                    {
                        time.day--;
                        days_in_current_month = get_days_in_month(time.month, time.year);
                        if (time.day < 1) time.day = days_in_current_month;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    UpdateTime();
                    OLED_ShowString(2, 1, "------Day-------");
                    OLED_ShowString(3, 1, " ");
                    OLED_ShowNum(3, 2, time.year, 4);
                    OLED_ShowString(3, 6, " : ");
                    OLED_ShowNum(3, 9, time.month, 2);
                    OLED_ShowString(3, 11, " :<");
                    OLED_ShowNum(3, 14, time.day, 2);
                    OLED_ShowString(3, 16, "> ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_time_mode++;
                    }
                }
                if (smart_clock.set_time_mode == SET_TIME_MONTH && choose == 1) 
                {
                    if (smart_clock.key_function.trigger_up) 
                    {
                        time.month++;
                        if (time.month > 12) time.month = 1;
                        smart_clock.key_function.trigger_up = 0;
                    }
                    else if (smart_clock.key_function.trigger_down)
                    {
                        time.month--;
                        if (time.month < 1) time.month = 12;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    UpdateTime();
                    OLED_ShowString(2, 1, "-----Month------");
                    OLED_ShowString(3, 1, " ");
                    OLED_ShowNum(3, 2, time.year, 4);
                    OLED_ShowString(3, 6, " :<");
                    OLED_ShowNum(3, 9, time.month, 2);
                    OLED_ShowString(3, 11, ">: ");
                    OLED_ShowNum(3, 14, time.day, 2);
                    OLED_ShowString(3, 16, " ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_time_mode++;
                    }
                }
                if (smart_clock.set_time_mode == SET_TIME_YEAR && choose == 1) 
								{
                    if (smart_clock.key_function.trigger_up) 
										{
                        time.year++;
                        smart_clock.key_function.trigger_up = 0;
                    }
										else if (smart_clock.key_function.trigger_down) 
										{
                        time.year--;
                        if (time.year < 1970) time.year = 1970;
                        smart_clock.key_function.trigger_up = 0;
                    }
                    UpdateTime();
                    OLED_ShowString(2, 1, "------Year------");
                    OLED_ShowString(3, 1, "<");
                    OLED_ShowNum(3, 2, time.year, 4);
                    OLED_ShowString(3, 6, ">: ");
                    OLED_ShowNum(3, 9, time.month, 2);
                    OLED_ShowString(3, 11, " : ");
                    OLED_ShowNum(3, 14, time.day, 2);
                    OLED_ShowString(3, 16, " ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_time_mode = 0; // 完成时间和日期设置，退出
                        smart_clock.key_function.set_mode = 0;
                        choose = 0;
                    }
                }
            }
            if (smart_clock.set_clock_mode >= SET_CLOCK_HOUR && choose == 1) 
						{
                if (smart_clock.set_clock_mode == 1) 
								{
                    if (smart_clock.key_function.trigger_up) 
										{
                        clock.hour++;
                        smart_clock.key_function.trigger_up = 0;
                    }
										else if (smart_clock.key_function.trigger_down) 
										{
                        clock.hour--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    if (clock.hour > 23) clock.hour = 0;
                    if (clock.hour < 0) clock.hour = 23;
                    OLED_ShowString(2, 1, "---Clock Hour---");
                    OLED_ShowString(3, 1, " <");
                    OLED_ShowNum(3, 3, clock.hour, 2);
                    OLED_ShowString(3, 5, ">: ");
                    OLED_ShowNum(3, 8, clock.min, 2);
                    OLED_ShowString(3, 10, " : ");
                    OLED_ShowNum(3, 13, clock.sec, 2);
                    OLED_ShowString(3, 15, "  ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_clock_mode++;
                    }
                }
                if (smart_clock.set_clock_mode == SET_CLOCK_MIN && choose == 1) 
								{
                    if (smart_clock.key_function.trigger_up) 
										{
                        clock.min++;
                        smart_clock.key_function.trigger_up = 0;
                    }
										else if (smart_clock.key_function.trigger_down) 
										{
                        clock.min--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    if (clock.min > 59) clock.min = 0;
                    if (clock.min < 0) clock.min = 59;
                    OLED_ShowString(2, 1, "---Clock Min----");
                    OLED_ShowString(3, 1, "  ");
                    OLED_ShowNum(3, 3, clock.hour, 2);
                    OLED_ShowString(3, 5, " :<");
                    OLED_ShowNum(3, 8, clock.min, 2);
                    OLED_ShowString(3, 10, ">: ");
                    OLED_ShowNum(3, 13, clock.sec, 2);
                    OLED_ShowString(3, 15, "  ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_clock_mode++;
                    }
                }
                if (smart_clock.set_clock_mode == SET_CLOCK_SEC && choose == 1) 
                {
                    if (smart_clock.key_function.trigger_up) 
                    {
                        clock.sec++;
                        smart_clock.key_function.trigger_up = 0;
                    }
                    else if (smart_clock.key_function.trigger_down) 
                    {
                        clock.sec--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    if (clock.sec > 59) clock.sec = 0;
                    if (clock.sec < 0) clock.sec = 59;
                    OLED_ShowString(2, 1, "---Clock Sec----");
                    OLED_ShowString(3, 1, "  ");
                    OLED_ShowNum(3, 3, clock.hour, 2);
                    OLED_ShowString(3, 5, " : ");
                    OLED_ShowNum(3, 8, clock.min, 2);
                    OLED_ShowString(3, 10, " :<");
                    OLED_ShowNum(3, 13, clock.sec, 2);
                    OLED_ShowString(3, 15, "> ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_clock_mode++;
                    }
                }
                if (smart_clock.set_clock_mode == SET_CLOCK_DAY && choose == 1) 
								{
                    if (smart_clock.key_function.trigger_up) 
										{
                        clock.day++;
                        days_in_current_month = get_days_in_month(clock.month, clock.year);
                        if (clock.day > days_in_current_month) clock.day = 1;
                        smart_clock.key_function.trigger_up = 0;
                    }
										else if (smart_clock.key_function.trigger_down) 
										{
                        clock.day--;
                        days_in_current_month = get_days_in_month(clock.month, clock.year);
                        if (clock.day < 1) clock.day = days_in_current_month;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "---Clock Day----");
                    OLED_ShowString(3, 1, " ");
                    OLED_ShowNum(3, 2, clock.year, 4);
                    OLED_ShowString(3, 6, " : ");
                    OLED_ShowNum(3, 9, clock.month, 2);
                    OLED_ShowString(3, 11, " :<");
                    OLED_ShowNum(3, 14, clock.day, 2);
                    OLED_ShowString(3, 16, "> ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_clock_mode++;
                    }
                }
                if (smart_clock.set_clock_mode == SET_CLOCK_MONTH && choose == 1) 
								{
                    if (smart_clock.key_function.trigger_up) 
										{
                        clock.month++;
                        if (clock.month > 12) clock.month = 1;
                        smart_clock.key_function.trigger_up = 0;
                    }
										else if (smart_clock.key_function.trigger_down) 
										{
                        clock.month--;
                        if (clock.month < 1) clock.month = 12;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "--Clock Month---");
                    OLED_ShowString(3, 1, " ");
                    OLED_ShowNum(3, 2, clock.year, 4);
                    OLED_ShowString(3, 6, " :<");
                    OLED_ShowNum(3, 9, clock.month, 2);
                    OLED_ShowString(3, 11, ">: ");
                    OLED_ShowNum(3, 14, clock.day, 2);
                    OLED_ShowString(3, 16, " ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_clock_mode++;
                    }
                }
                if (smart_clock.set_clock_mode == SET_CLOCK_YEAR && choose == 1) 
								{
                    if (smart_clock.key_function.trigger_up) 
										{
                        clock.year++;
                        smart_clock.key_function.trigger_up = 0;
                    }
										else if (smart_clock.key_function.trigger_down) 
										{
                        clock.year--;
                        if (clock.year < 1970) clock.year = 1970;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "---Clock Year---");
                    OLED_ShowString(3, 1, "<");
                    OLED_ShowNum(3, 2, clock.year, 4);
                    OLED_ShowString(3, 6, ">: ");
                    OLED_ShowNum(3, 9, clock.month, 2);
                    OLED_ShowString(3, 11, " : ");
                    OLED_ShowNum(3, 14, clock.day, 2);
                    OLED_ShowString(3, 16, " ");
                    if (smart_clock.key_function.confirm) 
										{
                        smart_clock.key_function.confirm = 0;
                        smart_clock.set_clock_mode = 0; // 完成闹钟设置，退出
						alarm_triggered = 1;
                        smart_clock.key_function.set_mode = 0;
                        choose = 0;
                    }
                }
            }
            if (smart_clock.countdown_mode >= COUNTDOWN_HOUR && choose == 1) {
                if (smart_clock.countdown_mode == 1) {
                    if (smart_clock.key_function.trigger_up) {
                        countdown.hour++;
                        smart_clock.key_function.trigger_up = 0;
                    } else if (smart_clock.key_function.trigger_down) {
                        countdown.hour--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    if (countdown.hour > 23) countdown.hour = 0;
                    if (countdown.hour < 0) countdown.hour = 23;
                    OLED_ShowString(2, 1, "--Count Hour----");
                    OLED_ShowString(3, 1, " <");
                    OLED_ShowNum(3, 3, countdown.hour, 2);
                    OLED_ShowString(3, 5, ">: ");
                    OLED_ShowNum(3, 8, countdown.min, 2);
                    OLED_ShowString(3, 10, " : ");
                    OLED_ShowNum(3, 13, countdown.sec, 2);
                    OLED_ShowString(3, 15, "  ");
                    if (smart_clock.key_function.confirm) {
                        smart_clock.key_function.confirm = 0;
                        smart_clock.countdown_mode++;
                    }
                }
                if (smart_clock.countdown_mode == COUNTDOWN_MIN && choose == 1) {
                    if (smart_clock.key_function.trigger_up) {
                        countdown.min++;
                        smart_clock.key_function.trigger_up = 0;
                    } else if (smart_clock.key_function.trigger_down) {
                        countdown.min--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    if (countdown.min > 59) countdown.min = 0;
                    if (countdown.min < 0) countdown.min = 59;
                    OLED_ShowString(2, 1, "--Count Min-----");
                    OLED_ShowString(3, 1, "  ");
                    OLED_ShowNum(3, 3, countdown.hour, 2);
                    OLED_ShowString(3, 5, " :<");
                    OLED_ShowNum(3, 8, countdown.min, 2);
                    OLED_ShowString(3, 10, ">: ");
                    OLED_ShowNum(3, 13, countdown.sec, 2);
                    OLED_ShowString(3, 15, "  ");
                    if (smart_clock.key_function.confirm) {
                        smart_clock.key_function.confirm = 0;
                        smart_clock.countdown_mode++;
                    }
                }
                if (smart_clock.countdown_mode == COUNTDOWN_SEC && choose == 1) {
                    if (smart_clock.key_function.trigger_up) {
                        countdown.sec++;
                        smart_clock.key_function.trigger_up = 0;
                    } else if (smart_clock.key_function.trigger_down) {
                        countdown.sec--;
                        smart_clock.key_function.trigger_down = 0;
                    }
                    if (countdown.sec > 59) countdown.sec = 0;
                    if (countdown.sec < 0) countdown.sec = 59;
                    OLED_ShowString(2, 1, "--Count Sec-----");
                    OLED_ShowString(3, 1, "  ");
                    OLED_ShowNum(3, 3, countdown.hour, 2);
                    OLED_ShowString(3, 5, " : ");
                    OLED_ShowNum(3, 8, countdown.min, 2);
                    OLED_ShowString(3, 10, " :<");
                    OLED_ShowNum(3, 13, countdown.sec, 2);
                    OLED_ShowString(3, 15, "> ");
                     if (smart_clock.key_function.confirm) {
                        smart_clock.key_function.confirm = 0;
						countdown_start = 1;  // start countdown
                        smart_clock.key_function.set_mode = 0;
                        choose = 0;
                        smart_clock.countdown_mode = 0; // 完成倒计时设置，退出
                    }
                }
            }
        }
    }
}

void TIM1_UP_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
        if (smart_clock.key_function.set_mode == 0) {
            time.sec++;
			if(countdown_start == 1){
				countdown.sec--;
			}
            if (time.sec % 10 == 0) {
                smart_clock.show_clock_mode=!smart_clock.show_clock_mode;
            }
        }
    }
}