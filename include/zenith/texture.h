/*
* texture.h
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: ${FILE_DESCRIPTION}
* Copyright (c) 2025 Max Van den Eynde
*/

#ifndef TEXTURE_H
#define TEXTURE_H

#ifdef ZENITH_EXT_TEXTURE

#include <zenith/zenith.h>

#ifdef ZENITH_VULKAN

namespace zen::texture {
    class TextureData {
    public:
        VkDeviceSize size = 0;
        std::shared_ptr<void> data;
        size_t width, height;

        void load(const std::string& path);
    };
}


#endif

#endif

#endif //TEXTURE_H
