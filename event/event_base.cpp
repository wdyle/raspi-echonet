#include "event_base.h"
#include <cstdlib>
#include <sstream>
#include <pair>
#include <limits>

#define DEBUG_EVENT_BASE

void CEventBase::print() const
{
    std::stringstream ss;
    ss << get_event_name();
    ss << " {";
    ss << std::endl;
    for (auto t : _values) {
        std::string &name = t.first;
        std::vector<char> &value = t.second;
        ss << name;
        ss << " :";
        ss << std::endl;
        
        print(ss, value);
    }
    ss << "}" << std::endl;
}

void CEventBase::print(std::basic_ostream<char, Traits> &ss, std::vector<char>::iterator begin, std::vector<char>::iterator end) const
{
    ss << "\t";
    for (std::vector<char>::iterator it = begin; it != end; ++it) {
        char c = *it;
        if (0x20 <= c && c <= 0x7e) {
            ss << c;
            ss << " ";
        } else if (0x0d == c) {
            ss << "\\r";
        } else if (0x0a == c) {
            ss << "\\n";
        } else {
            ss << "? ";
        }
    }
    ss << std::endl;
        
    ss << "\t";
    for (std::vector<char>::iterator it = begin; it != end; ++it) {
        char c = *it;
        char b[3];
        sprintf(b, "%02X", c);
        ss << b;
    }
    ss << std::endl;           
}

CEventMatchResult CEventBase::bufncmp(const char *s1, const std::vector<char> &s2_buf, const long buf_start, const long buf_length, const long compare_length)
{
    if (!valid_buffer_params(s2_buf, buf_start, buf_length)) {
        // invalid arg
        return EV_ERROR;
    }

    if (s1 == nullptr || compare_length <= 0)
    {
        // invalid arg
        return EV_ERROR;
    }
    
    if (compare_length > buf_length) {
        return EV_SHORT_LENGTH;
    }
    
    int result = std::strncmp(s1, buf.data() + start, compare_length);
    
    return result == 0 ? EV_MATCHED : EV_UNMATCHED;
}

long CEventBase::find_separator(const std::vector<char> &buf, const long buf_start, const long buf_length, long &out_crlf_pos) const
{
    const long end = buf_start + buf_length;
    long pos_space = -1;
    long pos_crlf = -1;

    for (long pos = buf_start; pos < end; ++pos) {
        if (buf[pos] == ' ') {
            pos_space = pos;
            break;
        } else if (pos + 1 < end && buf[pos] == '\r' && buf[pos+1] == '\n') {
            pos_crlf = pos;
            break;
        }      
    }
    
    out_crlf_pos = pos_crlf;
    return pos_space;
}

template <int count>
CEventMatchResult CEventBase::parseParams(const char *(&param_names)[count], const std::vector<char> &buf, const long buf_start, const long buf_length, long &out_next_pos)
{
#ifdef DEBUG_EVENT_BASE
    fprintf(stderr, "[DEBUG] CEventBase::parseParams(param_names[%d], [size=%d], %d, %d, [out])\n",
            count, buf.size(), buf_start, buf_length);
#endif
    
    if (!valid_buffer_params(buf, buf_start, buf_length) || count <= 0) {
        // invalid arg
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseParams(...): EV_ERROR(invalid argument)\n");
#endif
        return EV_ERROR;
    }
    for (int i = 0; i < count; ++i) {
        if (param_names[i] == NULL || *(param_names[i]) == 0) {
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseParams(...): EV_ERROR(invalid argument: param_names)\n");
#endif
            return EV_ERROR;
        }
    }

    std::array<std::pair<long, long>, count> result;
    
    long cur_pos = buf_start;
    long cur_length = buf_length;
    for (int i = 0; i < count; ++i) {
        long pos_crlf;
        long pos_space = find_separator(buf, cur_pos, cur_length, pos_crlf);
        if ((i + 1 != count && pos_crlf >= 0) || pos_space < 0) {
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseParams(...): EV_SHOT_LENGTH\n"
                        "        cur_pos=%d, cur_length=%d, pos_space=%d, pos_crlf=%d\n",
                cur_pos, cur_length, pos_space, pos_crlf);
#endif
            return EV_SHORT_LENGTH;
        }
        long pos_separator = std::max(pos_space, pos_crlf);
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseParams(...): param_names[%d]=\"%s\"\n"
                        "        cur_pos=%d, cur_length=%d, pos_space=%d, pos_crlf=%d\n",
                i, param_namses[i], cur_pos, cur_length, pos_space, pos_crlf);
        print(std::err, buf.begin() + cur_pos, buf.begin() + pos_separator);
#endif
        result[i] = std::make_pair(cur_pos, pos_separator);
        shift_position(pos_separatro - cur_pos, cur_pos, cur_length);

        if (i + 1 == count) {
            break;
        }
        if(shift_space(buf, cur_pos, cur_length) != CV_MATCHED) {
            assert(false);
        }
    }
    
    for (int i = 0; i < count; ++i) {
        long start = result[i].first;
        long end = result[i].second;
        std::vector<char> data(buf.begin() + start, buf.begin() + end);
        _values.insert(std::make_pair(std::string(param_names[i]), data));
    }

    out_next_pos = cur_pos;
    return EV_MATCHED;
}

CEventMatchResult CEventBase::parseData(const std::vector<char> &buf, const long buf_start, const long buf_length, const char *length_name, const char *data_name, long &out_next_pos) 
{
#ifdef DEBUG_EVENT_BASE
    fprintf(stderr, "[DEBUG] CEventBase::parseData([size=%d], %d, %d, \"%s\", \"%s\", [out])\n",
            buf.size(), buf_start, buf_length, length_name, data_name);
#endif
    if (!valid_buffer_params(buf, buf_start, buf_length)) {
        // invalid arg
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseData(...): EV_ERROR\n");
#endif
        return EV_ERROR;
    }

    long cur_pos = buf_start;
    long cur_length = buf_length;

    long pos_crlf;
    long pos_space = find_separator(buf, cur_pos, cur_length, pos_crlf);
    if (pos_crlf >= 0 || pos_space < 0) {
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseData(...): EV_SHORT_LENGTH\n"
                        "        cur_pos=%d, cur_length=%d, pos_space=%d, pos_crlf=%d\n",
                cur_pos, cur_length, pos_space, pos_crlf);
#endif
        return EV_SHORT_LENGTH;
    }

    std::vector<char> length_data(buf.begin() + cur_pos, buf.begin() + pos_space);
    std::string length_str(buf.begin() + cur_pos, buf.begin() + pos_space);
    int data_length = std::atoi(length_str.c_str());
    if (data_length < 0 || data_length > USHRT_MAX) {
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseData(...): EV_UNMATCHED\n"
                        "        cur_pos=%d, cur_length=%d, data_length=%d\n",
                cur_pos, cur_length, data_length);
#endif
        return EV_UNMATCHED;
    }
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseData(...): name=\"%s\", length=%d\n"
                        "        cur_pos=%d, cur_length=%d, pos_space=%d, pos_crlf=%d\n",
                length_name, length_data.size(), cur_pos, cur_length, pos_space, pos_crlf);
        print(std::err, length_data.begin(), length_data.begin());
#endif
    
    shift_position(pos_space - cur_pos, cur_pos, cur_length);
    if(shift_space(buf, cur_pos, cur_length) != CV_MATCHED) {
        assert(false);
    }

    if (cur_length <= data_length) {
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseData(...): EV_SHORT_LENGTH\n"
                        "        cur_pos=%d, cur_length=%d, data_length=%d\n",
                cur_pos, cur_length, data_length);
#endif
        return EV_SHORT_LENGTH;
    }
    
    std::vector<char> data(buf.begin() + cur_pos, buf.begin() + cur_pos + data_length);
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseData(...): name=\"%s\", length=%d\n"
                        "        cur_pos=%d, cur_length=%d, data_length=%d\n",
                data_name, data.size(), cur_pos, cur_length, data_length);
        print(std::err, data.begin(), data.begin());
#endif
    shift_position(data_length, cur_pos, cur_length);
    
    pos_space = find_separator(buf, cur_pos, cur_length, pos_crlf);
    if (pos_crlf != 0 && pos_space != 0) {
#ifdef DEBUG_EVENT_BASE
        fprintf(stderr, "[DEBUG] CEventBase::parseData(...): EV_UNMATCHED\n"
                        "        cur_pos=%d, cur_length=%d, pos_space=%d, pos_crlf=%d\n",
                cur_pos, cur_length, pos_space, pos_crlf);
#endif
        return EV_UNMATCHED;
    }
    
    _values.insert(std::make_pair(std::string(length_name), length_data));
    _values.insert(std::make_pair(std::string(data_name), data));

    out_next_pos = cur_pos;
    return EV_MATCHED;
}
