// Copyright (c) 2026 Colile Sibanda. All rights reserved.
// Proprietary — see LICENSE for terms. Unauthorised use prohibited.
#pragma once
#include <imgui.h>
#include <string>
#include <unordered_map>

// graphics/IconLibrary.hpp : rasterises the resources/icons SVG set to OpenGL
// textures for use as ImGui images, with a switchable view mode (the same
// symbol / realistic-2D / realistic-3D choice exposed by resources/icons/index.html).
//
// Textures are loaded lazily on first request and cached per (view, category,
// name); switching the view simply selects a different cache key, so already
// loaded variants are reused. All GL textures must be released with shutdown()
// while the GL context is still current.

enum class IconView { Symbol, Realistic2D, Realistic3D };

class IconLibrary {
public:
    IconLibrary() = default;
    ~IconLibrary() = default;

    // The current rendering variant. setView only changes which cache key is
    // used on subsequent texture() calls; it does not free existing textures.
    IconView view() const { return m_view; }
    void     setView(IconView v) { m_view = v; }

    // Pixel size (square) the SVGs are rasterised to.
    int rasterSize() const { return RASTER_PX; }

    // Returns an ImGui texture handle for category/name (e.g. "joints","fixed")
    // in the current view, rasterising and caching on first use. Returns 0 when
    // the SVG cannot be found or parsed (caller should fall back to text).
    ImTextureID texture(const char* category, const char* name);

    // Deletes all GL textures. Call before the GL context is destroyed.
    void shutdown();

private:
    static constexpr int RASTER_PX = 64;

    IconView m_view = IconView::Symbol;
    // Cache key is "<modeDir>/<category>/<name>"; value is the GL texture id.
    std::unordered_map<std::string, unsigned int> m_cache;
    // Resolved resources/icons base prefix (probed once across candidate paths).
    std::string m_basePath;
    bool        m_basePathResolved = false;

    const char* modeDir(IconView v) const;          // "symbols" / "realistic/2d" / "realistic/3d"
    std::string resolveBasePath();                   // first existing icons dir prefix
    unsigned int rasterizeToTexture(const std::string& fullPath); // 0 on failure
};
