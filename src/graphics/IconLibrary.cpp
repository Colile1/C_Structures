// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#include "graphics/IconLibrary.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <vector>

// nanosvg is a single-header SVG parser + rasteriser (zlib/public domain). The
// implementation lives only in this translation unit.
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

// modeDir
// Purpose: map a view mode to its on-disk sub-directory under resources/icons,
//          matching the layout that resources/icons/index.html switches between.
const char* IconLibrary::modeDir(IconView v) const {
    switch (v) {
        case IconView::Symbol:       return "symbols";
        case IconView::Realistic2D:  return "realistic/2d";
        case IconView::Realistic3D:  return "realistic/3d";
    }
    return "symbols";
}

// resolveBasePath
// Purpose: find the resources/icons directory regardless of the working dir the
//          app was launched from (mirrors main.cpp's font-path probing).
// Output:  a prefix ending in '/', or "" if none of the candidates exist.
std::string IconLibrary::resolveBasePath() {
    if (m_basePathResolved) return m_basePath;
    const char* candidates[] = {
        "resources/icons/",
        "../resources/icons/",
        "../../resources/icons/",
    };
    for (const char* c : candidates) {
        std::string probe = std::string(c) + "manifest.json";
        if (FILE* f = std::fopen(probe.c_str(), "rb")) {
            std::fclose(f);
            m_basePath = c;
            break;
        }
    }
    m_basePathResolved = true;
    return m_basePath;
}

// rasterizeToTexture
// Purpose: parse one SVG file and upload it as an RGBA GL texture at RASTER_PX.
// Inputs:  fullPath — path to the .svg file.
// Output:  a GL texture id, or 0 if the file is missing/unparseable. nanosvg
//          ignores <pattern> fills (e.g. support hatching) but renders the
//          shapes and gradients the icons rely on.
unsigned int IconLibrary::rasterizeToTexture(const std::string& fullPath) {
    NSVGimage* image = nsvgParseFromFile(fullPath.c_str(), "px", 96.0f);
    if (!image) return 0;
    if (image->width <= 0.0f || image->height <= 0.0f) { nsvgDelete(image); return 0; }

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) { nsvgDelete(image); return 0; }

    const int   px    = RASTER_PX;
    const float scale = px / std::max(image->width, image->height);
    // Centre the (possibly non-square) drawing in the square raster.
    const float tx = (px - image->width  * scale) * 0.5f;
    const float ty = (px - image->height * scale) * 0.5f;

    std::vector<unsigned char> rgba(static_cast<size_t>(px) * px * 4, 0);
    nsvgRasterize(rast, image, tx, ty, scale,
                  rgba.data(), px, px, px * 4);

    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, px, px, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

ImTextureID IconLibrary::texture(const char* category, const char* name) {
    const std::string key = std::string(modeDir(m_view)) + "/" + category + "/" + name;
    auto it = m_cache.find(key);
    if (it != m_cache.end())
        return static_cast<ImTextureID>(static_cast<intptr_t>(it->second));

    std::string base = resolveBasePath();
    unsigned int tex = 0;
    if (!base.empty())
        tex = rasterizeToTexture(base + key + ".svg");

    m_cache.emplace(key, tex); // cache misses (0) too, so we probe disk only once
    return static_cast<ImTextureID>(static_cast<intptr_t>(tex));
}

void IconLibrary::shutdown() {
    for (auto& kv : m_cache) {
        if (kv.second) {
            GLuint t = kv.second;
            glDeleteTextures(1, &t);
        }
    }
    m_cache.clear();
}
