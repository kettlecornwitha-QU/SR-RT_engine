#include "render/texture.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#if __has_include("stb_image.h")
#define HAVE_STB_IMAGE 1
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"
#endif

namespace {

double clamp01(double x) {
    if (x < 0.0)
        return 0.0;
    if (x > 1.0)
        return 1.0;
    return x;
}

int wrap_index(double t, int size) {
    if (size <= 0)
        return 0;
    double x = t - std::floor(t);
    int i = static_cast<int>(x * size);
    if (i < 0)
        i = 0;
    if (i >= size)
        i = size - 1;
    return i;
}

bool load_ppm_texture(const std::string& path,
                      ImageTexture& out,
                      std::string& message)
{
    std::ifstream in(path);
    if (!in) {
        message = "failed to open texture file";
        return false;
    }

    std::string magic;
    in >> magic;
    if (magic != "P3") {
        message = "unsupported texture format without stb_image (expected P3 PPM)";
        return false;
    }

    int width = 0;
    int height = 0;
    int maxv = 0;
    in >> width >> height >> maxv;
    if (!in || width <= 0 || height <= 0 || maxv <= 0) {
        message = "invalid PPM texture header";
        return false;
    }

    out.width = width;
    out.height = height;
    out.channels = 3;
    out.pixels.assign(static_cast<size_t>(width) * static_cast<size_t>(height) * 3u, 0);

    for (size_t i = 0; i < out.pixels.size(); ++i) {
        int v = 0;
        in >> v;
        if (!in) {
            message = "truncated PPM texture";
            return false;
        }
        out.pixels[i] = static_cast<unsigned char>(255.0 * clamp01(static_cast<double>(v) / maxv));
    }

    message = "loaded PPM texture";
    return true;
}

} // namespace

bool ImageTexture::valid() const {
    return width > 0 && height > 0 && (channels == 3 || channels == 4) && !pixels.empty();
}

Vec3 ImageTexture::sample(double u, double v) const {
    if (!valid())
        return {1, 1, 1};

    int x = wrap_index(u, width);
    int y = wrap_index(1.0 - v, height);
    size_t idx = (static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x)) * static_cast<size_t>(channels);

    double r = pixels[idx + 0] / 255.0;
    double g = pixels[idx + 1] / 255.0;
    double b = pixels[idx + 2] / 255.0;
    return {r, g, b};
}

std::shared_ptr<ImageTexture> ImageTexture::load(const std::string& path,
                                                 std::string& message)
{
    auto tex = std::make_shared<ImageTexture>();

#if HAVE_STB_IMAGE
    int w = 0;
    int h = 0;
    int n = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &n, 0);
    if (data && w > 0 && h > 0 && (n == 3 || n == 4)) {
        tex->width = w;
        tex->height = h;
        tex->channels = n;
        size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(n);
        tex->pixels.assign(data, data + size);
        stbi_image_free(data);
        message = "loaded image texture";
        return tex;
    }
    if (data)
        stbi_image_free(data);
#endif

    if (!load_ppm_texture(path, *tex, message)) {
#if !HAVE_STB_IMAGE
        message += " (stb_image.h not found for PNG/JPG)";
#endif
        return nullptr;
    }
    return tex;
}
