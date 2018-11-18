#ifndef _EVENT_ERXUDP_H_
#define _EVENT_ERXUDP_H_

#include "event_base.h"

class CEvERXUDP : public CEventBase
{
public:
    static constexpr char *event_name = "ERXUDP";
    static constexpr char magic_number[] = {
        'E', 'R', 'X', 'U', 'D', 'P', ' ',
    };

    static const char *get_event_name() 
    {
        return event_name;
    }

    static const char *get_magic_number() 
    {
        return magic_number;
    }

    static const char *get_magic_number_size()
    {
        return sizeof(magic_number);
    }

    CEventMatchResult parse(std::vector<char> &buf, long start, long length, long &next_pos) 
    {
        const char *field_names[] = {
            "SENDER",
            "DEST",
            "RPORT",
            "LPORT",
            "SENDERLLA",
            "SECURED"
        };
        const char *length_name = "DATALEN";
        const char *data_name = "DATA";

        long next = start;
        auto result = parseParams(field_names, buf, start, length, length_name, data_name, next);
        if (result != EV_MATCHED) {
            return result;
        }
        long next_length = length - (next - start);
        result = parseData(buf, next, next_length, length_name, data_name, next);
        if (result != EV_MATCHED) {
            return result;
        }

        next_pos = next;
        return EV_MATCHED;
    }
    
};

#endif
