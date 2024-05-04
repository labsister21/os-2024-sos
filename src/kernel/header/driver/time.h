#ifndef _TIME_H
#define _TIME_H

extern struct TimeRTC startup_time;
extern struct TimeRTC current_time;

void enable_rtc_interrupt();
void handle_rtc_interrupt();
void time_handle_timer_interrupt();
void setup_time();

#endif
