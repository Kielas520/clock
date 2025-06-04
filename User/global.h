#ifndef __GLOBAL_H
#define __GLOBAL_H
#include <stdint.h>


typedef struct {
    int8_t sec;    
    int8_t min;    
    int8_t hour;   
    int8_t day;    
    int8_t month;  
    int16_t year;  
} Time_t;

typedef struct {
uint8_t set_mode;
uint8_t confirm;
uint8_t trigger_up;
uint8_t trigger_down;
}Key_Function_t;

typedef struct {
uint8_t alarm_triggered;
uint8_t countdown_start;
}Alarm_Function_t;


typedef enum{
    DATE_MODE=0,
    CLOCK_MODE,
}Show_Clock_Mode_e;

typedef enum{
    SET_TIME_NONE=0,
    SET_TIME_HOUR,
    SET_TIME_MIN,
    SET_TIME_SEC,
    SET_TIME_DAY,
    SET_TIME_MONTH,
    SET_TIME_YEAR,
}Set_Time_Mode_e;

typedef enum{
    SET_CLOCK_NONE=0,
    SET_CLOCK_HOUR,
    SET_CLOCK_MIN,
    SET_CLOCK_SEC,
    SET_CLOCK_DAY,
    SET_CLOCK_MONTH,
    SET_CLOCK_YEAR,
}Set_Clock_Mode_e;

typedef enum{
    COUNTDOWN_NONE=0,
    COUNTDOWN_HOUR,
    COUNTDOWN_MIN,
    COUNTDOWN_SEC,
    COUNTDOWN_DAY,
    COUNTDOWN_MONTH,
    COUNTDOWN_YEAR,
}Countdown_Mode_e;

typedef enum{
    FUNCTION_NONE=0,
    SHOW_MODE,
    SET_MODE,
    COUNTDOWN_MODE,
}Function_Mode_e;

typedef struct {
Key_Function_t key_function;
Alarm_Function_t alarm_function;
Show_Clock_Mode_e show_clock_mode;
Set_Clock_Mode_e set_clock_mode;
Set_Time_Mode_e set_time_mode;
Countdown_Mode_e countdown_mode;
Function_Mode_e function_mode;
}Smart_Clock_t;

extern const uint8_t days_in_month_normal[12];
extern const uint8_t days_in_month_leap[12];

extern Time_t time;
extern Time_t clock;
extern Time_t countdown;
extern Smart_Clock_t smart_clock;

extern uint8_t days_in_current_month;
extern uint8_t choose;
extern int8_t choose_buff;
extern uint8_t is_leap = 0;
extern uint8_t key_states[4];

#endif