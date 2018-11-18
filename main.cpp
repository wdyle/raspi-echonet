#include <unistd.h>
#include <stdio.h>
#include "serial/serial.h"
#include "serial/timeout.h"
#include "event/erxudp.h"

int main(int argc, char *argv[])
{
#if 1
    CEvERXUDP ev;
    CEventParser<CEvERXUDP> parser;
    
    CSerial serial;
    const char *port = "/dev/ttyUSB0";
    const speed_t rate = B115200;

    int ret = serial.open(port, rate);
    if (ret < 0) {
        printf("open failed(%d)\n", ret);
        return ret;
    }

    serial.set_timeout(2000);

    char cmd[8] = {'S', 'K', 'V', 'E', 'R', '\r', '\n', 0};
    serial.write(cmd, sizeof(cmd));

    std::vector<char> buf(4096);

    constexpr int count = 3;
    for(int j=0; j < count; ++j) {
        long len = serial.read(buf);
        if (0 < len) {
#if 0
            printf("[len=%ld]\n", len);
            for (int i=0; i < len; ++i) {
                if (0x20 <= buf[i] && buf[i] <= 0x7e)
                    printf("%c ", buf[i]);
                else if (0x0d == buf[i])
                    printf("\\r");
                else if (0x0a == buf[i])
                    printf("\\n");
                else
                    printf("? ");
            }
            printf("\n");
            for (int i=0; i < len; ++i) {
                printf("%02X", buf[i]);
            }
            printf("\n");
#endif
        } else if (len == 0) {
            char cmd2[11] = {'S', 'K', 'A', 'P', 'P', 'V', 'E', 'R', '\r', '\n', 0};
            serial.write(cmd2, sizeof(cmd2));
        }
    }
                     
#else
    CTimeout t(3000);
    while (!t.is_expired()) {
        timespec *ts = t.time_left();
        if (ts == nullptr) {
            printf("infinite!\n");
        } else {
            printf("tv_sec = %ld, tv_nsec = %ld\n", ts->tv_sec, ts->tv_nsec);
        }
        usleep(500000);
    }
#endif
    
    return 0;
}
