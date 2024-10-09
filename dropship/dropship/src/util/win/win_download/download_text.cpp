#include "pch.h"

#include "download_txt.h"

namespace util::win_download {


    namespace {
        struct ComInit
        {
            HRESULT hr;
            ComInit() : hr(::CoInitialize(nullptr)) {}
            ~ComInit() { if (SUCCEEDED(hr)) ::CoUninitialize(); }
        };
    }


    std::string _download_txt(std::string _uri) {

        std::string _result;

        //std::wstring uri = __string_to_LPCWSTR(_uri);
        std::wstring uri = std::wstring(_uri.begin(), _uri.end());

        ComInit init;

        // use CComPtr so you don't have to manually call Release()
        CComPtr<IStream> pStream;

        // Open the HTTP request.
        HRESULT hr = URLOpenBlockingStreamW(nullptr, uri.c_str(), &pStream, 0, nullptr);
        if (FAILED(hr))
        {
            //std::cout << "ERROR: Could not connect. HRESULT: 0x" << std::hex << hr << std::dec << "\n";
            throw std::runtime_error("could not connect");
        }

        // Download the response and write it to stdout.
        char buffer[4096];
        do
        {
            DWORD bytesRead = 0;
            hr = pStream->Read(buffer, sizeof(buffer), &bytesRead);

            if (bytesRead > 0)
            {
                //std::cout.write(buffer, bytesRead);
                _result.append(buffer, bytesRead);
            }
        } while (SUCCEEDED(hr) && hr != S_FALSE);

        if (FAILED(hr))
        {
            //std::cout << "ERROR: Download failed. HRESULT: 0x" << std::hex << hr << std::dec << "\n";
            throw std::runtime_error("download failed");
        }

        return _result;
    };

}
