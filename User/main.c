#include "stm32f10x.h"                  
#include "Delay.h"
#include "global.h"
#include "OLED.h"
#include "timer1.h"
#include "Key.h"

// 每月天数数组
const uint8_t days_in_month_normal[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const uint8_t days_in_month_leap[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 全局变量定义
Time time = {0, 0, 0, 1, 1, 2025};
Time clock = {0, 0, 0, 1, 1, 2025};
Time countdown = {0, 0, 0, 0, 0, 0};
uint8_t days_in_current_month = 30;
uint8_t time_mode = 0;
uint8_t set_mode = 0;
uint8_t trigger_up = 0;
uint8_t trigger_down = 0;
uint8_t choose = 0;
int8_t choose_buff = 0;
uint8_t set_time = 0;
uint8_t set_clock = 0;
uint8_t set_countdown = 0;
uint8_t confirm = 0;
uint8_t key_states[4] = {0}; // 存储按键状态

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
    OLED_ShowNum(1, 6, key_states[0], 1); // PB13 (set_mode)
    OLED_ShowNum(1, 8, key_states[1], 1); // PB15 (trigger_up)
    OLED_ShowNum(1, 10, key_states[2], 1); // PA9 (trigger_down)
    OLED_ShowNum(1, 12, key_states[3], 1); // PA11 (confirm)
    OLED_ShowString(1, 14, "M:");
    OLED_ShowNum(1, 16, set_mode, 1); // 显示set_mode状态
    OLED_ShowString(2, 1, "Cd:");
    OLED_ShowNum(2, 4, set_countdown, 1); // 显示set_countdown状态
}

int main(void) {
    Key_Init();
    OLED_Init();        
    TIM1_Init();
    OLED_ShowString(1, 1, "Kielas");                        

    while (1) {
        Key_GetNum(key_states);
        UpdateTime();
        
        // 调试：取消注释以启用
        // ShowKeyStates();

        if (set_mode == 0) {  
            if (confirm) {
                time_mode = !time_mode;
                confirm = 0;
            }
            if (time_mode == 0) {
                ShowTime();
            } else if (time_mode == 1) {
                ShowDate();
            }
        } 
        else if (set_mode == 1) {
            if (choose == 0) {
                OLED_ShowString(4, 1, "--Mode----Set---");
                OLED_ShowString(2, 1, "-----Choose-----");
                if (trigger_up) {
                    trigger_up = 0;
                    choose_buff++;
                } 
                else if (trigger_down) {
                    trigger_down = 0;
                    choose_buff--;
                }
				if(choose_buff < 0){
					choose_buff = 2;
				}
                choose_buff = choose_buff % 3;
                if (choose_buff == 0) {
                    OLED_ShowString(3, 1, "<Clk>--Cd---Tim-");
                    if (confirm) {
                        confirm = 0;
                        set_clock = 1;
                        choose = 1;
                    }
                } 
                else if (choose_buff == 1) {
                    OLED_ShowString(3, 1, "-Clk--<Cd>--Tim-");
                    if (confirm) {
                        confirm = 0;
                        set_countdown = 1;
                        choose = 1;
                    }
                }
                else if (choose_buff == 2) {
                    OLED_ShowString(3, 1, "-Clk---Cd--<Tim>");
                    if (confirm) {
                        confirm = 0;
                        set_time = 1;
                        choose = 1;
                    }
                }
            }
            if (set_time >= 1 && choose == 1) {
                if (set_time == 1) {
                    if (trigger_up) {
                        time.hour++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        time.hour--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_time++;
                    }
                }
                if (set_time == 2 && choose == 1) {
                    if (trigger_up) {
                        time.min++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        time.min--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_time++;
                    }
                }
                if (set_time == 3 && choose == 1) {
                    if (trigger_up) {
                        time.sec++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        time.sec--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_time++;
                    }
                }
                if (set_time == 4 && choose == 1) {
                    if (trigger_up) {
                        time.day++;
                        days_in_current_month = get_days_in_month(time.month, time.year);
                        if (time.day > days_in_current_month) time.day = 1;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        time.day--;
                        days_in_current_month = get_days_in_month(time.month, time.year);
                        if (time.day < 1) time.day = days_in_current_month;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_time++;
                    }
                }
                if (set_time == 5 && choose == 1) {
                    if (trigger_up) {
                        time.month++;
                        if (time.month > 12) time.month = 1;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        time.month--;
                        if (time.month < 1) time.month = 12;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_time++;
                    }
                }
                if (set_time == 6 && choose == 1) {
                    if (trigger_up) {
                        time.year++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        time.year--;
                        if (time.year < 1970) time.year = 1970;
                        trigger_up = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_time = 0; // 完成时间和日期设置，退出
                        set_mode = 0;
                        choose = 0;
                    }
                }
            }
            if (set_clock >= 1 && choose == 1) {
                if (set_clock == 1) {
                    if (trigger_up) {
                        clock.hour++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        clock.hour--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_clock++;
                    }
                }
                if (set_clock == 2 && choose == 1) {
                    if (trigger_up) {
                        clock.min++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        clock.min--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_clock++;
                    }
                }
                if (set_clock == 3 && choose == 1) {
                    if (trigger_up) {
                        clock.sec++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        clock.sec--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_clock++;
                    }
                }
                if (set_clock == 4 && choose == 1) {
                    if (trigger_up) {
                        clock.day++;
                        days_in_current_month = get_days_in_month(clock.month, clock.year);
                        if (clock.day > days_in_current_month) clock.day = 1;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        clock.day--;
                        days_in_current_month = get_days_in_month(clock.month, clock.year);
                        if (clock.day < 1) clock.day = days_in_current_month;
                        trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "---Clock Day----");
                    OLED_ShowString(3, 1, " ");
                    OLED_ShowNum(3, 2, clock.year, 4);
                    OLED_ShowString(3, 6, " : ");
                    OLED_ShowNum(3, 9, clock.month, 2);
                    OLED_ShowString(3, 11, " :<");
                    OLED_ShowNum(3, 14, clock.day, 2);
                    OLED_ShowString(3, 16, "> ");
                    if (confirm) {
                        confirm = 0;
                        set_clock++;
                    }
                }
                if (set_clock == 5 && choose == 1) {
                    if (trigger_up) {
                        clock.month++;
                        if (clock.month > 12) clock.month = 1;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        clock.month--;
                        if (clock.month < 1) clock.month = 12;
                        trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "--Clock Month---");
                    OLED_ShowString(3, 1, " ");
                    OLED_ShowNum(3, 2, clock.year, 4);
                    OLED_ShowString(3, 6, " :<");
                    OLED_ShowNum(3, 9, clock.month, 2);
                    OLED_ShowString(3, 11, ">: ");
                    OLED_ShowNum(3, 14, clock.day, 2);
                    OLED_ShowString(3, 16, " ");
                    if (confirm) {
                        confirm = 0;
                        set_clock++;
                    }
                }
                if (set_clock == 6 && choose == 1) {
                    if (trigger_up) {
                        clock.year++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        clock.year--;
                        if (clock.year < 1970) clock.year = 1970;
                        trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "---Clock Year---");
                    OLED_ShowString(3, 1, "<");
                    OLED_ShowNum(3, 2, clock.year, 4);
                    OLED_ShowString(3, 6, ">: ");
                    OLED_ShowNum(3, 9, clock.month, 2);
                    OLED_ShowString(3, 11, " : ");
                    OLED_ShowNum(3, 14, clock.day, 2);
                    OLED_ShowString(3, 16, " ");
                    if (confirm) {
                        confirm = 0;
                        set_clock = 0; // 完成闹钟设置，退出
                        set_mode = 0;
                        choose = 0;
                    }
                }
            }
            if (set_countdown >= 1 && choose == 1) {
                if (set_countdown == 1) {
                    if (trigger_up) {
                        countdown.hour++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        countdown.hour--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_countdown++;
                    }
                }
                if (set_countdown == 2 && choose == 1) {
                    if (trigger_up) {
                        countdown.min++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        countdown.min--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_countdown++;
                    }
                }
                if (set_countdown == 3 && choose == 1) {
                    if (trigger_up) {
                        countdown.sec++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        countdown.sec--;
                        trigger_down = 0;
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
                    if (confirm) {
                        confirm = 0;
                        set_countdown++;
                    }
                }
                if (set_countdown == 4 && choose == 1) {
                    if (trigger_up) {
                        countdown.day++;
                        days_in_current_month = get_days_in_month(countdown.month, countdown.year);
                        if (countdown.day > days_in_current_month) countdown.day = 1;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        countdown.day--;
                        days_in_current_month = get_days_in_month(countdown.month, countdown.year);
                        if (countdown.day < 1) countdown.day = days_in_current_month;
                        trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "--Count Day-----");
                    OLED_ShowString(3, 1, " ");
                    OLED_ShowNum(3, 2, countdown.year, 4);
                    OLED_ShowString(3, 6, " : ");
                    OLED_ShowNum(3, 9, countdown.month, 2);
                    OLED_ShowString(3, 11, " :<");
                    OLED_ShowNum(3, 14, countdown.day, 2);
                    OLED_ShowString(3, 16, "> ");
                    if (confirm) {
                        confirm = 0;
                        set_countdown++;
                    }
                }
                if (set_countdown == 5 && choose == 1) {
                    if (trigger_up) {
                        countdown.month++;
                        if (countdown.month > 12) countdown.month = 1;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        countdown.month--;
                        if (countdown.month < 1) countdown.month = 12;
                        trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "-Count Month----");
                    OLED_ShowString(3, 1, " ");
                    OLED_ShowNum(3, 2, countdown.year, 4);
                    OLED_ShowString(3, 6, " :<");
                    OLED_ShowNum(3, 9, countdown.month, 2);
                    OLED_ShowString(3, 11, ">: ");
                    OLED_ShowNum(3, 14, countdown.day, 2);
                    OLED_ShowString(3, 16, " ");
                    if (confirm) {
                        confirm = 0;
                        set_countdown++;
                    }
                }
                if (set_countdown == 6 && choose == 1) {
                    if (trigger_up) {
                        countdown.year++;
                        trigger_up = 0;
                    } else if (trigger_down) {
                        countdown.year--;
                        if (countdown.year < 1970) countdown.year = 1970;
                        trigger_down = 0;
                    }
                    OLED_ShowString(2, 1, "--Count Year----");
                    OLED_ShowString(3, 1, "<");
                    OLED_ShowNum(3, 2, countdown.year, 4);
                    OLED_ShowString(3, 6, ">: ");
                    OLED_ShowNum(3, 9, countdown.month, 2);
                    OLED_ShowString(3, 11, " : ");
                    OLED_ShowNum(3, 14, countdown.day, 2);
                    OLED_ShowString(3, 16, " ");
                    if (confirm) {
                        confirm = 0;
                        set_countdown = 0; // 完成倒计时设置，退出
                        set_mode = 0;
                        choose = 0;
                    }
                }
            }
        }
    }
}

void TIM1_UP_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
        if (set_mode == 0) {
            time.sec++;
            if (time.sec % 10 == 0) {
                time_mode = !time_mode;
            }
        }
    }
}