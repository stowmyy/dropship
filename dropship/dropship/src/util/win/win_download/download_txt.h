#pragma once

// web https://stackoverflow.com/a/44029974
// web https://github.com/elddy/Windows-NTAPI-Injector/blob/c4f545e27a885bc7d53c4dc303eaab80f2d1d803/NativeInjection/Download_shellcode.h

#include <Windows.h>
#include <Urlmon.h>   // URLOpenBlockingStreamW()
//#include <atlbase.h>  // CComPtr
//#include <iostream>
#pragma comment( lib, "Urlmon.lib" )

#include <string>
#include <stdexcept>

/* // web https://www.geeksforgeeks.org/convert-stdstring-to-lpcwstr-in-c/
inline LPCWSTR __string_to_LPCWSTR(std::string s) {
    // Initializing an object of wstring
    std::wstring ws = std::wstring(s.begin(), s.end());

    // Applying c_str() method on temp
    LPCWSTR wideString = ws.c_str();
    return wideString;
} */

namespace util::win_download {

    std::string _download_txt(std::string _uri);

}
