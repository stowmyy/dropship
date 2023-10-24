#pragma once

#include <d3d11.h>
#include <string>


#include <unordered_map>

// #include <unordered_map>

extern ID3D11Device* g_pd3dDevice;
// extern std::string global_message;

// bool addTexture(std::string name, )

[[nodiscard]] bool _loadPicture(unsigned char* data, unsigned long int size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);

[[nodiscard]] bool loadPicture(std::string title, std::string type, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);

bool _add_texture(std::string title, std::string type);

ID3D11ShaderResourceView* _get_texture(std::string title);

// static std::unordered_map<std::string, ImageTexture> APP_TEXTURES = { };

struct ImageTexture
{
    ID3D11ShaderResourceView* texture;
    int width;
    int height;
};

