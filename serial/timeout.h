#ifndef _TIMEOUT_H_
#define _TIMEOUT_H_

#include <time.h>

class CTimeout
{
public:
    using duration_t = long long;

    CTimeout()
        : _infinite(true), _duration(0)
    {
        start();
    }
    
    CTimeout(duration_t duration_msec)
    {
        _infinite = duration_msec < 0;
        _duration = duration_msec;

        start();
    }

    bool is_infinite()
    {
        return _infinite || _duration < 0;
    }

    bool is_expired()
    {
        return time_left() != nullptr && _left_time == 0;
    }
    
    
    timespec *time_left()
    {
        if (is_infinite()) {
            return nullptr;
        }

        _left_time = 0;
        _left_time_ts.tv_sec = 0;
        _left_time_ts.tv_nsec = 0;
        
        if (_duration == 0) {
            return &_left_time_ts;
        }

        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        duration_t now = ts_to_nsec(ts);
        if (now >= _target_time) {
            return &_left_time_ts;
        }

        duration_t d = _target_time - now;
        _left_time = d;
        nsec_to_ts(d, _left_time_ts);
        return &_left_time_ts;
    }
    
private:
    bool _infinite;
    duration_t _duration;

    duration_t _target_time;
    duration_t _left_time;
    struct timespec _left_time_ts;

    const duration_t msec_to_nsec = 1000000;
    const duration_t sec_to_nsec = 1000000000;

    duration_t ts_to_nsec(const timespec &ts)
    {
        return ts.tv_sec * sec_to_nsec + ts.tv_nsec;
    }
    void nsec_to_ts(duration_t t, timespec &ts)
    {
        duration_t sec = t / sec_to_nsec;
        duration_t nsec = t - (sec * sec_to_nsec);
        ts.tv_sec = (time_t)sec;
        ts.tv_nsec = (long)nsec;
    }
    
    void start()
    {
        if (is_infinite())
        {
            return;
        }

        timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        duration_t now = ts_to_nsec(ts);
        _target_time = now + _duration * msec_to_nsec;
    }
    
};

#endif
