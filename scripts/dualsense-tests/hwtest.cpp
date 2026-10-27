// Plays test patterns through DualSenseBtHaptics on a real controller
#include <dualsensebthaptics.h>
#include <chiaki/log.h>
#include <cmath>
#include <cstdio>
#include <time.h>
static void sleep_until(timespec &t, long add_ns){ t.tv_nsec+=add_ns; while(t.tv_nsec>=1000000000L){t.tv_nsec-=1000000000L;t.tv_sec++;} clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&t,nullptr); }
int main(int argc,char**argv){
  ChiakiLog log; chiaki_log_init(&log, CHIAKI_LOG_ALL & ~CHIAKI_LOG_VERBOSE, chiaki_log_cb_print, nullptr);
  SDL_Init(0);
  DualSenseBtHaptics h(&log);
  if(!h.Open(argv[1])) return 1;
  timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
  const int rate=3000; long frame=0;
  // 0-3s silence | 3-5 180 Hz both | 5-7 silence | 7-9 60 Hz left | 9-11 silence | 11-13 60 Hz right | 13-15 silence | 15-17 pulses both
  const int total_s=17;
  for(int pkt=0; pkt<100*total_s; pkt++){        // 30 frames per 10 ms, like the console
    QByteArray b(60, 0);
    for(int i=0;i<30;i++,frame++){
      double s=(double)frame/rate; int sec=(int)s;
      int8_t l=0, r=0;
      if(sec>=3 && sec<5){ int8_t v=(int8_t)(std::sin(2*M_PI*180*s)*110); l=r=v; }
      else if(sec>=7 && sec<9){ l=(int8_t)(std::sin(2*M_PI*60*s)*120); }
      else if(sec>=11 && sec<13){ r=(int8_t)(std::sin(2*M_PI*60*s)*120); }
      else if(sec>=15 && sec<17){ bool on=fmod(s,0.25)<0.08; int8_t v=on?(int8_t)(std::sin(2*M_PI*120*s)*127):0; l=r=v; }
      b[i*2]=(char)l; b[i*2+1]=(char)r;
    }
    h.PushSamples(b);
    sleep_until(t,10000000L);
  }
  h.Close(); SDL_Quit(); return 0;
}
