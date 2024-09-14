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
    /*std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (in)
    {
        return(std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>()));
    }*/

    std::string s; //string
    std::fstream f; //file stream
    f.open(filename); //open your word list
    std::getline(f, s); //get line from f (your word list) and put it in s (the string)
    
    f.close();

    return s;
}

class DownloadException : public std::exception {
private:
    std::string message;

public:
    DownloadException(const char* msg)
        : message(msg)
    {
    }

    // Override the what() method to return our message 
    const char* what() const throw()
    {
        return message.c_str();
    }
};


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


static void download_file(std::string uri, std::string filename, float* progress, std::string* data = nullptr, std::filesystem::path* path = nullptr)
{
    std::filesystem::path _path = std::filesystem::temp_directory_path().append("dropship");

    if (!std::filesystem::is_directory(_path) || !std::filesystem::exists(_path)) {
        std::filesystem::create_directory(_path);
    }

    _path.append(filename);

    if (std::filesystem::exists(_path))
        std::filesystem::remove(_path);

    std::string _path_name = _path.string();

    DeleteUrlCacheEntry(_path_name.c_str());

    printf("downloading: %s\n", _path_name.c_str());

    _downloading _progress(progress);
    if (URLDownloadToFile(NULL, uri.c_str(), _path_name.c_str(), 0, &_progress) != S_OK)
    {
        printf("download failed\n");
        throw DownloadException("download failed");
    }
    else
    {
        printf("download worked\n");

        if (data != nullptr)
        {

            *data = get_file_contents(_path_name.c_str());
        }

        if (path != nullptr)
        {
            *path = _path;
        }

        // std::filesystem::hash_value

        //system(std::format("Get-FileHash {0} -Algorithm SHA512 | Select-Object -ExpandProperty Hash | Out-File {1}.comp", filePath.string(), filePath.string()).c_str());

        //std::filesystem::
    }
}