#pragma once

#include <d3d11.h>
#include <string>

extern ID3D11Device* g_pd3dDevice;
// extern std::string global_message;

[[nodiscard]] bool _loadPicture(unsigned char* data, unsigned long int size, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);

[[nodiscard]] bool loadPicture(std::string name, std::string type, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);

struct ImageTexture
{
    ID3D11ShaderResourceView* texture;
    int width;
    int height;
};

