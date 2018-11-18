#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <termios.h>
#include <vector>

enum CSerialError {
    E_INVALID_ARG   = -1,
    E_NOT_OPENED    = -2,
    E_OPEN_FAILED   = -10,
    E_READ_FAILED   = -11,
    E_WRITE_FAILED  = -12,
    E_SELECT_FAILED = -13,
};

class CSerial 
{
public:
    using timeout_t = long;
    const int CLOSED = -1;
    const timeout_t INFINITE = -1;
  
    CSerial()
        : _timeout_msec(INFINITE), _fd(CLOSED)
    {
    }

    ~CSerial();
    
    int open(const char *name, speed_t brate);

    void close();

    long read(std::vector<char> &buf, long start = 0, long read_size = 1);
    
    size_t write(const void *buf, size_t count);
    
    timeout_t get_timeout() 
    {
        return _timeout_msec;
    }
    void set_timeout(const timeout_t timeout)
    {
        _timeout_msec = timeout;
    }
    bool is_opened()
    {
        return _fd >= 0;
    }
    
private:
    long _timeout_msec;
    
    int _fd;
    struct termios _oldtio, _newtio;
};

#endif
