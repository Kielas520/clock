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
} Time;

extern const uint8_t days_in_month_normal[12];
extern const uint8_t days_in_month_leap[12];

extern Time time;
extern Time clock;
extern Time countdown;
extern uint8_t days_in_current_month;
extern uint8_t time_mode;
extern uint8_t set_mode;
extern uint8_t trigger_up;
extern uint8_t trigger_down;
extern uint8_t choose;
extern int8_t choose_buff;
extern uint8_t set_time;
extern uint8_t set_clock;
extern uint8_t set_countdown;
extern uint8_t confirm;
extern uint8_t key_states[4];

#endif