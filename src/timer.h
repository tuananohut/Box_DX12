#ifndef TIMER_H
#define TIMER_H

typedef __int64 s64;

typedef float f32; 
typedef double real64; 

struct Timer
{
  real64 seconds_per_count;
  real64 delta_time;

  s64 base_time;
  s64 paused_time;
  s64 stop_time;
  s64 prev_time;
  s64 curr_time;

  bool is_stopped; 
};

void InitializeTimer(Timer* timer); 

f32 TotalTime(Timer* timer);

void Reset(Timer* timer);
void Start(Timer* timer);
void Stop(Timer* timer);
void Tick(Timer* timer); 

#endif
