#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#include <iostream> // must be included before hack.h as SAL annotations
#include <iomanip>  // collide with GCC's standard library implmentation

#include <filesystem>  // collide with GCC's standard library implmentation

#include "urlmon.h" // copied into project folder (from Windows SDK)
// along with the import library and an empty "msxml.h" file


#pragma comment( lib, "urlmon.lib" )
#pragma comment(lib, "wininet.Lib" )

//IBindStatusCallback _downloading()

#include <wininet.h> // MinGW does provide this (needed for DeleteUrlCacheEntry)
#include <urlmon.h>    
#include <iostream>

#pragma comment (lib, "urlmon.lib")

#include <fstream>
#include <streambuf>

static std::string get_file_contents(const char* filename)
{
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        return(std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
    }
}


class _downloading : public IBindStatusCallback
{
private:
    //float download_progress = 0.0f; // 0.0f = 1.0f

    float* p_progress = nullptr;

public:
    _downloading(float* progress) : p_progress(progress) {}

    ~_downloading() { }

    // This one is called by URLDownloadToFile
    STDMETHOD(OnProgress)(/* [in] */ ULONG ulProgress, /* [in] */ ULONG ulProgressMax, /* [in] */ ULONG ulStatusCode, /* [in] */ LPCWSTR wszStatusText)
    {
        std::cout << "Downloaded " << ulProgress << " of " << ulProgressMax << " byte(s), " << " Status Code = " << ulStatusCode << std::endl;

        if (ulProgressMax > 0)
        {
            *(this->p_progress) = (float) ((double) ulProgress / (double) ulProgressMax);
        }

        return S_OK;
    }

    // The rest  don't do anything...
    STDMETHOD(OnStartBinding)(/* [in] */ DWORD dwReserved, /* [in] */ IBinding __RPC_FAR* pib)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetPriority)(/* [out] */ LONG __RPC_FAR* pnPriority)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(OnLowResource)(/* [in] */ DWORD reserved)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(OnStopBinding)(/* [in] */ HRESULT hresult, /* [unique][in] */ LPCWSTR szError)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(GetBindInfo)(/* [out] */ DWORD __RPC_FAR* grfBINDF, /* [unique][out][in] */ BINDINFO __RPC_FAR* pbindinfo)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(OnDataAvailable)(/* [in] */ DWORD grfBSCF, /* [in] */ DWORD dwSize, /* [in] */ FORMATETC __RPC_FAR* pformatetc, /* [in] */ STGMEDIUM __RPC_FAR* pstgmed)
    {
        return E_NOTIMPL;
    }

    STDMETHOD(OnObjectAvailable)(/* [in] */ REFIID riid, /* [iid_is][in] */ IUnknown __RPC_FAR* punk)
    {
        return E_NOTIMPL;
    }

    // IUnknown stuff
    STDMETHOD_(ULONG, AddRef)()
    {
        return 0;
    }

    STDMETHOD_(ULONG, Release)()
    {
        return 0;
    }

    STDMETHOD(QueryInterface)(/* [in] */ REFIID riid, /* [iid_is][out] */ void __RPC_FAR* __RPC_FAR* ppvObject)
    {
        return E_NOTIMPL;
    }
};


static void download_file(std::wstring uri, std::wstring filename, float* progress, std::string* data = nullptr, std::filesystem::path* path = nullptr)
{
    std::wstring directory = std::filesystem::temp_directory_path().wstring() + L"/dropship";

    if (!std::filesystem::is_directory(directory) || !std::filesystem::exists("src")) {
        std::filesystem::create_directory(directory);
    }

    std::filesystem::path filePath = directory + L"/" + filename;

    DeleteUrlCacheEntry(filePath.c_str());

    wprintf(filePath.c_str());

    _downloading _progress(progress);
    if (URLDownloadToFile(NULL, uri.c_str(), filePath.c_str(), 0, &_progress) != S_OK)
    {
        printf("download failed\n");
    }
    else
    {
        printf("download worked\n");

        if (data != nullptr)
        {
            *data = get_file_contents(filePath.string().c_str());
        }

        if (path != nullptr)
        {
            *path = filePath;
        }

        // std::filesystem::hash_value

        //system(std::format("Get-FileHash {0} -Algorithm SHA512 | Select-Object -ExpandProperty Hash | Out-File {1}.comp", filePath.string(), filePath.string()).c_str());

        //std::filesystem::
    }
}

/*
    //const TCHAR url[] = _T("http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3337.pdf");
    //const TCHAR filePath[] = _T("C:\\Test\\n3337.pdf");
    //const TCHAR filePath[] = _T("C:\\Test\\n3337.pdf");

    printf("hi");
    wprintf(L"xx");

    std::wstring filePath = std::filesystem::temp_directory_path().wstring() + L"/" + filename;

    wprintf(L"downloading: %s", uri.c_str());
    wprintf(L"to: %s%s", filePath.c_str(), filename.c_str());

    // invalidate cache, so file is always downloaded from web site
    // (if not called, the file will be retieved from the cache if
    // it's already been downloaded.)
    // DeleteUrlCacheEntry(filePath.c_str());

    HRESULT hr = URLDownloadToFile(
        NULL,   // A pointer to the controlling IUnknown interface (not needed here)
        uri.c_str(),
        filePath.c_str(),
        0,      // Reserved. Must be set to 0.
        NULL); // status callback interface (not needed for basic use)
    if (SUCCEEDED(hr))
    {
        printf("downloaded");
    }
    else
    {
        wprintf(L"An error occured : error code = 0x%s", hr);
    }*/