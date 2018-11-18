#ifndef _EVENT_BASE_H_
#define _EVENT_BASE_H_

#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <memory>

enum CEventMatchResult {
    EV_MATCHED,
    EV_UNMATCHED,
    EV_SHORT_LENGTH,
    EV_ERROR,
};

template <class T>
class CEventParser 
{
public:
    using ptr_type = std::unique_ptr<T>;
    
    static const char *get_event_name() 
    {
        return T::get_event_name();
    }
    
    static const char *get_magic_number()
    {
        return T::get_magic_number();
    }
    
    static const long get_magic_number_size()
    {
        return T::get_magic_number_size();
    }
    

    static CEventMatchResult compare_magic_number(const std::vector<char> &buf, const long start, const long length)
    {
        return bufncmp(get_magic_number(), buf, start, length, get_magic_number_size());
    }

    static ptr_type create_instance() 
    {
        ptr_type pointer(new T());
        return std::move(pointer);
    }
    
};

class CEventBase
{
public:
    // will be overrided
    virtual CEventMatchResult parse(std::vector<char> &buf, long start, long length, long &next_pos) = 0;

    void print() const;
    
protected:
    std::map<std::string, std::vector<char>> _values;

    template <int count>
    CEventMatchResult parseParams(const char *(&param_names)[count], const std::vector<char> &buf, const long buf_start, const long buf_length, long &out_next_pos);

    CEventMatchResult parseData(const std::vector<char> &buf, const long buf_start, const long buf_length, const char *length_name, const char *data_name, long &out_next_pos);

    template <class Traits>
        void print(std::basic_ostream<char, Traits> &ss, std::vector<char>::iterator begin, std::vector<char>::iterator end) const;
        
    bool valid_buffer_params(const std::vector<char> &buf, const long buf_start, const long buf_length) const
    {
        // if buf_length == 0, it will be VALID
        return buf_start < 0 || buf_length < 0 || (buf_start + buf_length) > (signed)buf.size();
    }

    void shift_position(const long n, long &start, long &length) const
    {
        assert(length >= n);
        start += n;
        length -= n;
    }
    
    CEventMatchResult shift_position(const std::vector<char> &buf, long &start, long &length, const char *str, const long count) const
    {
        assert((signed)buf.size() >= start + count);
        if (bufncmp(str, buf, start, length, count) != EV_MATCHED) {
            return EV_UNMATCHED;
        }
        shift_position(count, start, length);
        return EV_MATCHED;
    }

    template <long count>
    CEventMatchResult shift_position(const std::vector<char> &buf, long &start, long &length, const char (&str)[count]) const
    {
        return shift_position(buf, start, length, str, count);
    }
    
    CEventMatchResult shift_space(const std::vector<char> &buf, long &start, long &length) const
    {
        const char space[] = {' '};
        return shift_position(buf, start, length, space);
    }
    
    CEventMatchResult shift_crlf(const std::vector<char> &buf, long &start, long &length) const
    {
        const char crlf[] = {'\r', '\n'};
        return shift_position(buf, start, length, crlf);
    }

    long find_separator(const std::vector<char> &buf, const long buf_start, const long buf_length, long &out_crlf_pos) const;

public:
    static CEventMatchResult bufncmp(const char *s1, const std::vector<char> &s2_buf, const long buf_start, const long buf_length, const long compare_length);

    template <long size>
        static CEventMatchResult bufncmp(const char (&s1)[size], const std::vector<char> &s2_buf, const long buf_start, const long buf_length)
    {
        return bufncmp(s1, s2_buf, buf_start, buf_length, size);
    }  
};

#endif
