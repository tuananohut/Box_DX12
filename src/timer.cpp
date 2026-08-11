#include <windows.h>
#include "timer.h"

real64 seconds_per_count;
  real64 delta_time;

  s64 base_time;
  s64 paused_time;
  s64 stop_time;
  s64 prev_time;
  s64 curr_time;

  bool is_stopped; 

void InitializeTimer(Timer* timer)
{
  timer->seconds_per_count = 0.0;
  timer->delta_time = -1.0;
  timer->base_time = 0;
  timer->paused_time = 0;
  timer->prev_time = 0;
  timer->curr_time = 0;
  timer->is_stopped = false;

  s64 counts_per_second;
  QueryPerformanceFrequency((LARGE_INTEGER*)&counts_per_second);
  timer->seconds_per_count = 1.0 / (real64)counts_per_second; 
}

f32 TotalTime(Timer* timer) 
{
  if (timer->is_stopped)
    {
      return (f32)(((timer->stop_time - timer->paused_time) - timer->base_time) * timer->seconds_per_count);
    }
  else
    {
      return (f32)(((timer->curr_time - timer->paused_time) - timer->base_time) * timer->seconds_per_count);
    }
}

void Reset(Timer* timer)
{
  s64 curr_time;
  QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);
  
  timer->base_time = curr_time;
  timer->prev_time = curr_time;
  timer->stop_time = 0;
  timer->is_stopped = false; 
}

void Start(Timer* timer)
{
  s64 start_time;
  QueryPerformanceCounter((LARGE_INTEGER*)&start_time);

  if (timer->is_stopped)
    {
      timer->paused_time += (start_time - timer->stop_time);

      timer->prev_time = start_time;
      timer->stop_time = 0;
      timer->is_stopped = false; 
    }
}

void Stop(Timer* timer)
{
  if (!timer->is_stopped)
    {
      s64 curr_time;
      QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);

      timer->stop_time = curr_time;
      timer->is_stopped = true; 
    }
}

void Tick(Timer* timer)
{
  if (timer->is_stopped)
    {
      timer->delta_time = 0.0;
      return; 
    }

  s64 curr_time;
  QueryPerformanceCounter((LARGE_INTEGER*)&curr_time);
  timer->curr_time = curr_time;

  timer->delta_time = (timer->curr_time - timer->prev_time) * timer->seconds_per_count;

  timer->prev_time = timer->curr_time;

  if (timer->delta_time < 0.0)
    {
      timer->delta_time = 0.0;
    }
} 
