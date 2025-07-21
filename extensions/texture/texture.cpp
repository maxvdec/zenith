/*
* texture.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: 
* Copyright (c) 2025 Max Van den Eynde
*/

#ifdef ZENITH_EXT_TEXTURE
#ifdef ZENITH_VULKAN

#include <zenith/texture.h>
#define STB_IMAGE_IMPLEMENTATION
#include <iostream>
#include <stb/stb_image.h>

using namespace zen;

void zen::texture::TextureData::load(const std::string& path) {
    int width, height, channels;
    stbi_uc* imageData = stbi_load(path.c_str(), &width, &height, &channels, 0);
    this->width = static_cast<uint32_t>(width);
    this->height = static_cast<uint32_t>(height);
    size = width * height * channels;
    if (!imageData) {
        throw std::runtime_error("Failed to load texture image: " + path);
    }
    data = std::shared_ptr<void>(imageData, [](void* ptr)
    {
        stbi_image_free(ptr);
    });
}


#endif
#endif
