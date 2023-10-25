
#include <d3d11.h>

#include "images.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// #include "resource.h"

#include <format>

#include <iostream> // delete this
//#include <string>
//#include <string_view>


// #include <variant>

// #include <iostream>

// #include <vector>

// #include <tchar.h>

#include <mutex>

/* struct Resource
{
    std::variant<int, std::wstring> lpType;
    std::variant<int, std::wstring> lpName;
    HANDLE lpData;
    DWORD dwSize;
};

std::vector<Resource> resources; */

extern std::unordered_map<std::string, ImageTexture> APP_TEXTURES;


[[nodiscard]] bool _loadPicture(unsigned char* data, unsigned long int size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{

    // ::global_message = std::format("loaded {}.{}", name, type);

    // all dx11 stuff
    {
        // Load from disk into a raw RGBA buffer
        int image_width = 0;
        int image_height = 0;

        unsigned char* image_data = stbi_load_from_memory(data, size, &image_width, &image_height, NULL, 4);
        if (image_data == NULL)
            return false;

        // Create texture
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = image_width;
        desc.Height = image_height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;

        ID3D11Texture2D* pTexture = NULL;
        D3D11_SUBRESOURCE_DATA subResource;
        subResource.pSysMem = image_data;
        subResource.SysMemPitch = desc.Width * 4;
        subResource.SysMemSlicePitch = 0;
        g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;

        if (pTexture)
            g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    // ::global_message = std::format("texture {}.{}", name, type);

    // added later??
    //::FreeResource(resource);

    return false;

}


bool loadPicture(std::string title, std::string type, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height) {
    // all resource stuff
    std::wstring tmp_name = std::wstring(title.begin(), title.end());
    std::wstring tmp_type = std::wstring(type.begin(), type.end());

    LPCWSTR w_name = tmp_name.c_str();
    LPCWSTR w_type = tmp_type.c_str();

    // https://stackoverflow.com/a/28753627
    // ugh.
    HRSRC resource = FindResource(NULL, w_name, w_type);

    if (!resource) {
        //::global_message = std::format("failed {}.{}", name, type);
        return FALSE;
    }

    DWORD size = ::SizeofResource(NULL, resource);
    HGLOBAL data_handle = ::LoadResource(NULL, resource);

    // data pointer
    _loadPicture((unsigned char*) ::LockResource(data_handle), size, out_srv, out_width, out_height);
}

bool _add_texture(std::string title, std::string type)
{
    ImageTexture im;
    bool loaded = loadPicture(title, type, &(im.texture), &(im.width), &(im.height));

    APP_TEXTURES.insert({ title, im });

    return loaded;
}

ID3D11ShaderResourceView* _get_texture(std::string title)
{

    if (APP_TEXTURES.contains(title))
        return APP_TEXTURES.at(title).texture;
    
    return nullptr;
}
