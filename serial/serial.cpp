#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "serial.h"
#include "timeout.h"

#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#include <errno.h>

void debug_print_buffer(const char *buf, long len)
{
    fprintf(stderr, "        ");
    for (int i=0; i < len; ++i) {
        if (0x20 <= buf[i] && buf[i] <= 0x7e)
            fprintf(stderr, "%c ", buf[i]);
        else if (0x0d == buf[i])
            fprintf(stderr, "\\r");
        else if (0x0a == buf[i])
            fprintf(stderr, "\\n");
        else
            fprintf(stderr, "? ");
    }
    fprintf(stderr, "\n        ");
    for (int i=0; i < len; ++i) {
        fprintf(stderr, "%02X", buf[i]);
    }
    fprintf(stderr, "\n");
}
#endif

CSerial::~CSerial()
{
#ifdef DEBUG_SERIAL
    fprintf(stderr, "[DEBUG] CSerial::~CSerial(void)\n"); 
#endif
    close();
}

int CSerial::open(const char *name, speed_t brate)
{
#ifdef DEBUG_SERIAL
    fprintf(stderr, "[DEBUG] CSerial::open(\"%s\", %d)\n", name, brate); 
#endif
    if (name == nullptr) {
#ifdef DEBUG_SERIAL
        fprintf(stderr, "[DEBUG] CSerial::open(...): E_INVALID_ARG\n"); 
#endif
        return E_INVALID_ARG;
    }
    
    int fd = ::open(name, O_RDWR /* | O_NDELAY*/);
    if (fd < 0) {
#ifdef DEBUG_SERIAL
        fprintf(stderr, "[DEBUG] CSerial::open(...): E_OPEN_FAILED\n"
                        "        ret=%d, errno=%d, msg=\"%s\"\n",
                fd, errno, strerror(errno)); 
#endif
        return E_OPEN_FAILED;
    }
    
    tcgetattr(fd, &_oldtio);

    memset(&_newtio, 0, sizeof(_newtio));
    _newtio.c_cflag = CREAD | CLOCAL | CS8;
    cfsetspeed(&_newtio, brate);
    cfmakeraw(&_newtio);

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &_newtio);

    _fd = fd;
    
#ifdef DEBUG_SERIAL
    fprintf(stderr, "[DEBUG] CSerial::open(...): open succeeded. fd=%d\n", fd); 
#endif
    return 0;
}

void CSerial::close()
{
#ifdef DEBUG_SERIAL
    fprintf(stderr, "[DEBUG] CSerial::close(void)\n"); 
#endif
    if (!is_opened()) {
#ifdef DEBUG_SERIAL
        fprintf(stderr, "[DEBUG] CSerial::close(...): (E_NOT_OPENED)\n"); 
#endif
        return;
    }

    tcsetattr(_fd, TCSANOW, &_oldtio);
    
    ::close(_fd);
    _fd = CLOSED;
    
#ifdef DEBUG_SERIAL
    fprintf(stderr, "[DEBUG] CSerial::close(...): close succeeded.\n"); 
#endif
}

long CSerial::read(std::vector<char> &buf, long start, long read_size)
{
    /*
      read <size> bytes from port.
      this function blocks until read specified bytes
      or elapse specified time.
     */
#ifdef DEBUG_SERIAL
    fprintf(stderr, "[DEBUG] CSerial::read([size=%d], %ld, %ld)\n",
            buf.size(), start, read_size); 
#endif
    if (!is_opened()) {
#ifdef DEBUG_SERIAL
        fprintf(stderr, "[DEBUG] CSerial::read(...): E_NOT_OPENED\n"); 
#endif
        return E_NOT_OPENED;
    }

    long cur = start;
    long buffer_left = buf.size() - cur;
    long read_left = read_size;

    if (cur < 0 || read_left <= 0 || buffer_left <= 0) {
#ifdef DEBUG_SERIAL
        fprintf(stderr, "[DEBUG] CSerial::read(...): E_INVALID_ARG\n"); 
#endif
        return E_INVALID_ARG;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(_fd, &fds);
    
    CTimeout timeout(_timeout_msec);
    while (read_left > 0 && buffer_left > 0) {
        int ret = pselect(_fd + 1, &fds, nullptr, nullptr, timeout.time_left(), nullptr);
        if (ret == 0) {
            // timeout
#ifdef DEBUG_SERIAL
            fprintf(stderr, "[DEBUG] CSerial::read(...): timeout return (t=%ld msec)\n", _timeout_msec); 
#endif
            break;
        } else if (ret < 0) {
            // pselect_error
#ifdef DEBUG_SERIAL
            fprintf(stderr, "[DEBUG] CSerial::read(...): E_SELECT_FAILED\n"
                            "        ret=%d, errno=%d, msg=\"%s\"\n",
                ret, errno, strerror(errno)); 
#endif
            return E_SELECT_FAILED;
        }

        int read_bytes = ::read(_fd, buf.data() + cur, buffer_left);
        if (read_bytes < 0) {
#ifdef DEBUG_SERIAL
            fprintf(stderr, "[DEBUG] CSerial::read(...): E_READ_FAILED\n"
                            "        ret=%d, errno=%d, msg=\"%s\"\n",
                read_bytes, errno, strerror(errno)); 
#endif
            return E_READ_FAILED;
        } else if (read_bytes == 0) {
#ifdef DEBUG_SERIAL
            fprintf(stderr, "[DEBUG] CSerial::read(...): eof\n"); 
#endif
            break;
        }
        
        cur += read_bytes;
        buffer_left -= read_bytes;
        read_left -= read_bytes;
#ifdef DEBUG_SERIAL
        fprintf(stderr, "[DEBUG] CSerial::read(...): read_bytes=%d\n"
                        "        cur=%ld, buffer_left=%ld, read_left=%ld\n",
                read_bytes, cur, buffer_left, read_left);
        debug_print_buffer(buf.data() + (cur - read_bytes), read_bytes);
#endif  
        if (timeout.is_expired()) {
#ifdef DEBUG_SERIAL
            fprintf(stderr, "[DEBUG] CSerial::read(...): is_expired() == true\n"); 
#endif
            break;
        }
    }

    return cur - start;
}
 
size_t CSerial::write(const void *buf, size_t count)
{
#ifdef DEBUG_SERIAL
    fprintf(stderr, "[DEBUG] CSerial::write(<pointer>, %d)\n", count); 
#endif
    if (!is_opened()) {
#ifdef DEBUG_SERIAL
        fprintf(stderr, "[DEBUG] CSerial::write(...): E_NOT_OPENED\n"); 
#endif
        return E_NOT_OPENED;
    }

    size_t ret = ::write(_fd, buf, count);
    if (ret < 0) {
#ifdef DEBUG_SERIAL
        fprintf(stderr, "[DEBUG] CSerial::write(...): E_WRITE_FAILED\n"
                        "        ret=%d, errno=%d, msg=\"%s\"\n",
                ret, errno, strerror(errno)); 
#endif
        return E_WRITE_FAILED;
    }
#ifdef DEBUG_SERIAL
    fprintf(stderr, "[DEBUG] CSerial::write(...): write succeeded (ret=%d)\n", ret); 
#endif
    return ret;
}
