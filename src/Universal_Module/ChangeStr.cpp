#include "ChangeStr.h"

#include <cerrno>
#include<string>
#include <stdexcept>

#ifdef __linux__
#include <iconv.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace ChangeStr {
  std::string wstringToUtf8(const std::wstring& message){
  #if defined(__linux__)
    if (message.empty()) {
        return {};
    }

    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        throw std::runtime_error("iconv_open failed");
    }

    std::string result;
    result.resize(message.size() * 4);

    char* input = reinterpret_cast<char*>(
        const_cast<wchar_t*>(message.data())
    );
    size_t inputLeft = message.size() * sizeof(wchar_t);

    char* output = result.data();
    size_t outputLeft = result.size();

    while (inputLeft > 0) {
        size_t rc = iconv(cd, &input, &inputLeft, &output, &outputLeft);

        if (rc != static_cast<size_t>(-1)) {
            continue;
        }

        if (errno == E2BIG) {
            const size_t used = result.size() - outputLeft;
            result.resize(result.size() * 2);
            output = result.data() + used;
            outputLeft = result.size() - used;
            continue;
        }

        iconv_close(cd);
        throw std::runtime_error("wstringToUtf8: invalid input or conversion failed");
    }

    result.resize(result.size() - outputLeft);
    iconv_close(cd);
    return result;

  #elif defined(_WIN32)
     
    if (message.empty()) {
        return {};
    }

    int size = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        message.data(),
        static_cast<int>(message.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (size == 0) {
        throw std::runtime_error("WideCharToMultiByte failed");
    }

    std::string result(size, '\0');

    int converted = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        message.data(),
        static_cast<int>(message.size()),
        result.data(),
        size,
        nullptr,
        nullptr
    );

    if (converted != size) {
      throw std::runtime_error("WideCharToMultiByte conversion failed");
    }

    return result;

  #else
    throw std::runtime_error("wstringToUtf8: unsupported platform");

  #endif
  }
}

 


