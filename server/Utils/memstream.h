#pragma once

#include <iostream>

namespace utils
{

struct membuf : public std::basic_streambuf<char>
{
  membuf(char* p, size_t l) { setg(p, p, p + l); }
};


class memstream : public std::istream {
public:
  memstream(char* p, size_t l) :
    std::istream(&_buffer),
    _buffer(p, l) {
    rdbuf(&_buffer);
  }

private:
  membuf _buffer;
};


} // namespace utils
