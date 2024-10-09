#include "pch.h"

#include "download_file.h"


namespace util::win_download {

    void download_file(std::string uri, std::string filename, float* progress, std::string* data, std::filesystem::path* path)
    {
        std::filesystem::path _path = std::filesystem::temp_directory_path().append("dropship");

        if (!std::filesystem::is_directory(_path) || !std::filesystem::exists(_path)) {
            std::filesystem::create_directory(_path);
        }

        _path.append(filename);

        if (std::filesystem::exists(_path))
            std::filesystem::remove(_path);

        std::string _path_name = _path.string();

        DeleteUrlCacheEntryA(_path_name.c_str());

        printf("downloading: %s\n", _path_name.c_str());

        _downloading _progress(progress);
        if (URLDownloadToFileA(NULL, uri.c_str(), _path_name.c_str(), 0, &_progress) != S_OK)
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


    std::string get_file_contents(const char* filename) {
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

}
