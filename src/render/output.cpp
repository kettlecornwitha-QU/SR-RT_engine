#include "render/output.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#if __has_include(<filesystem>)
#include <filesystem>
#endif

#include "core/material.h"
#include "render/constants.h"

#if defined(ENABLE_OIDN) && __has_include(<OpenImageDenoise/oidn.hpp>)
#define HAVE_OIDN 1
#include <OpenImageDenoise/oidn.hpp>
#endif

#if __has_include("tinyexr.h")
#define HAVE_TINYEXR 1
#include "tinyexr.h"
#endif

#if __has_include("stb_image_write.h")
#define HAVE_STB_IMAGE_WRITE 1
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "stb_image_write.h"
#endif

namespace {

std::string temp_oidn_stem() {
#if __has_include(<filesystem>)
    const auto base = std::filesystem::temp_directory_path();
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::ostringstream oss;
    oss << (base / ("sr_rt_oidn_" + std::to_string(now))).string();
    return oss.str();
#else
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return "/tmp/sr_rt_oidn_" + std::to_string(now);
#endif
}

bool write_pfm(const std::string& path,
               const std::vector<Vec3>& framebuffer,
               int width,
               int height)
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
        return false;

    out << "PF\n" << width << " " << height << "\n-1.0\n";
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const Vec3& c = framebuffer[y * width + x];
            const float rgb[3] = {
                static_cast<float>(c.x),
                static_cast<float>(c.y),
                static_cast<float>(c.z),
            };
            out.write(reinterpret_cast<const char*>(rgb), sizeof(rgb));
        }
    }
    return static_cast<bool>(out);
}

bool read_pfm(const std::string& path,
              int width,
              int height,
              std::vector<Vec3>& framebuffer)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;

    std::string magic;
    in >> magic;
    if (magic != "PF")
        return false;

    int file_w = 0;
    int file_h = 0;
    float scale = 0.0f;
    in >> file_w >> file_h >> scale;
    in.get();
    if (!in || file_w != width || file_h != height)
        return false;

    std::vector<Vec3> flipped(static_cast<size_t>(width * height), Vec3{0, 0, 0});
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            float rgb[3] = {0.0f, 0.0f, 0.0f};
            in.read(reinterpret_cast<char*>(rgb), sizeof(rgb));
            if (!in)
                return false;
            flipped[y * width + x] = {rgb[0], rgb[1], rgb[2]};
        }
    }

    framebuffer = std::move(flipped);
    return true;
}

bool denoise_oidn_cli_if_available(const std::vector<Vec3>& beauty,
                                   const std::vector<Vec3>& albedo,
                                   const std::vector<Vec3>& normal,
                                   int width,
                                   int height,
                                   std::vector<Vec3>& denoised,
                                   std::string& message)
{
    const char* oidn_bin = std::getenv("SR_RT_OIDN_DENOISE_BIN");
    if (!oidn_bin || !oidn_bin[0])
        return false;

    const std::string stem = temp_oidn_stem();
    const std::string color_path = stem + "_color.pfm";
    const std::string albedo_path = stem + "_albedo.pfm";
    const std::string normal_path = stem + "_normal.pfm";
    const std::string output_path = stem + "_output.pfm";
    const std::string log_path = stem + "_oidn.log";

    auto cleanup = [&]() {
        std::remove(color_path.c_str());
        std::remove(albedo_path.c_str());
        std::remove(normal_path.c_str());
        std::remove(output_path.c_str());
        std::remove(log_path.c_str());
    };

    if (!write_pfm(color_path, beauty, width, height) ||
        !write_pfm(albedo_path, albedo, width, height) ||
        !write_pfm(normal_path, normal, width, height)) {
        cleanup();
        message = "failed to write temporary PFM files for oidnDenoise";
        return false;
    }

    std::string oidn_dir;
#if __has_include(<filesystem>)
    oidn_dir = std::filesystem::path(oidn_bin).parent_path().string();
#endif

    std::ostringstream cmd;
    cmd << "\"" << oidn_bin << "\""
        << " --hdr \"" << color_path << "\""
        << " --alb \"" << albedo_path << "\""
        << " --nrm \"" << normal_path << "\""
        << " -o \"" << output_path << "\""
        << " --clean_aux"
        << " --verbose 2"
        << " > \"" << log_path << "\" 2>&1";

    const char* previous_dyld = std::getenv("DYLD_LIBRARY_PATH");
    std::string previous_dyld_value = previous_dyld ? previous_dyld : "";
    if (!oidn_dir.empty())
        setenv("DYLD_LIBRARY_PATH", oidn_dir.c_str(), 1);

    const int ret = std::system(cmd.str().c_str());

    if (!oidn_dir.empty()) {
        if (previous_dyld)
            setenv("DYLD_LIBRARY_PATH", previous_dyld_value.c_str(), 1);
        else
            unsetenv("DYLD_LIBRARY_PATH");
    }
    if (ret != 0) {
        std::ifstream log(log_path);
        std::ostringstream details;
        if (log)
            details << log.rdbuf();
        cleanup();
        message = "oidnDenoise subprocess failed";
        if (!details.str().empty())
            message += ": " + details.str();
        return false;
    }

    const bool ok = read_pfm(output_path, width, height, denoised);
    cleanup();
    if (!ok) {
        message = "failed to read oidnDenoise output";
        return false;
    }

    message = "OIDN denoise complete";
    return true;
}

double clamp01(double x) {
    return std::min(1.0, std::max(0.0, x));
}

double aces_tonemap_channel(double x) {
    const double a = 2.51;
    const double b = 0.03;
    const double c = 2.43;
    const double d = 0.59;
    const double e = 0.14;
    return clamp01((x * (a * x + b)) / (x * (c * x + d) + e));
}

Vec3 apply_display_transform(const Vec3& linear_color, double exposure) {
    Vec3 exposed = linear_color * exposure;
    Vec3 mapped = {
        aces_tonemap_channel(exposed.x),
        aces_tonemap_channel(exposed.y),
        aces_tonemap_channel(exposed.z)
    };

    return {
        std::pow(clamp01(mapped.x), 1.0 / 2.2),
        std::pow(clamp01(mapped.y), 1.0 / 2.2),
        std::pow(clamp01(mapped.z), 1.0 / 2.2)
    };
}

} // namespace

std::vector<unsigned char> make_ldr_buffer(const std::vector<Vec3>& framebuffer,
                                           int width,
                                           int height,
                                           double exposure)
{
    std::vector<unsigned char> pixels(width * height * 3, 0);
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const Vec3& linear = framebuffer[j * width + i];
            Vec3 display = apply_display_transform(linear, exposure);
            int idx = (j * width + i) * 3;
            pixels[idx + 0] = static_cast<unsigned char>(255.999 * clamp01(display.x));
            pixels[idx + 1] = static_cast<unsigned char>(255.999 * clamp01(display.y));
            pixels[idx + 2] = static_cast<unsigned char>(255.999 * clamp01(display.z));
        }
    }
    return pixels;
}

std::vector<unsigned char> make_aov_ldr_buffer(const std::vector<Vec3>& framebuffer,
                                               int width,
                                               int height,
                                               bool is_normal)
{
    std::vector<unsigned char> pixels(width * height * 3, 0);
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            Vec3 c = framebuffer[j * width + i];
            if (is_normal) {
                c = 0.5 * (c + Vec3{1, 1, 1});
            }
            c = {clamp01(c.x), clamp01(c.y), clamp01(c.z)};

            int idx = (j * width + i) * 3;
            pixels[idx + 0] = static_cast<unsigned char>(255.999 * c.x);
            pixels[idx + 1] = static_cast<unsigned char>(255.999 * c.y);
            pixels[idx + 2] = static_cast<unsigned char>(255.999 * c.z);
        }
    }
    return pixels;
}

bool gather_primary_aovs(const Ray& r,
                         const Hittable& world,
                         Vec3& albedo,
                         Vec3& normal)
{
    HitRecord rec;
    if (!world.hit(r, kHitTMin, std::numeric_limits<double>::infinity(), rec))
        return false;

    albedo = rec.material->aov_albedo(rec);
    normal = rec.normal.normalize();
    return true;
}

bool write_ppm(const std::string& path,
               const std::vector<unsigned char>& rgb,
               int width,
               int height)
{
    std::ofstream image(path);
    if (!image)
        return false;

    image << "P3\n" << width << " " << height << "\n255\n";
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int idx = (j * width + i) * 3;
            image << static_cast<int>(rgb[idx + 0]) << " "
                  << static_cast<int>(rgb[idx + 1]) << " "
                  << static_cast<int>(rgb[idx + 2]) << "\n";
        }
    }
    return true;
}

bool write_png_if_available(const std::string& path,
                            const std::vector<unsigned char>& rgb,
                            int width,
                            int height)
{
#if HAVE_STB_IMAGE_WRITE
    return stbi_write_png(path.c_str(), width, height, 3, rgb.data(), width * 3) != 0;
#else
    (void)path;
    (void)rgb;
    (void)width;
    (void)height;
    return false;
#endif
}

bool write_exr_if_available(const std::string& path,
                            const std::vector<Vec3>& framebuffer,
                            int width,
                            int height)
{
#if HAVE_TINYEXR
    std::vector<float> images[3];
    images[0].resize(width * height);
    images[1].resize(width * height);
    images[2].resize(width * height);

    for (int i = 0; i < width * height; ++i) {
        images[0][i] = static_cast<float>(framebuffer[i].x);
        images[1][i] = static_cast<float>(framebuffer[i].y);
        images[2][i] = static_cast<float>(framebuffer[i].z);
    }

    EXRImage image;
    InitEXRImage(&image);
    image.num_channels = 3;
    std::vector<float*> image_ptrs = {
        images[2].data(),
        images[1].data(),
        images[0].data()
    };
    image.images = reinterpret_cast<unsigned char**>(image_ptrs.data());
    image.width = width;
    image.height = height;

    EXRHeader header;
    InitEXRHeader(&header);
    header.num_channels = 3;
    header.channels = new EXRChannelInfo[3];
    std::strncpy(header.channels[0].name, "B", 255);
    std::strncpy(header.channels[1].name, "G", 255);
    std::strncpy(header.channels[2].name, "R", 255);

    header.pixel_types = new int[3];
    header.requested_pixel_types = new int[3];
    for (int i = 0; i < 3; ++i) {
        header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF;
    }

    const char* err = nullptr;
    int ret = SaveEXRImageToFile(&image, &header, path.c_str(), &err);

    delete[] header.channels;
    delete[] header.pixel_types;
    delete[] header.requested_pixel_types;

    if (ret != TINYEXR_SUCCESS) {
        if (err)
            FreeEXRErrorMessage(err);
        return false;
    }
    return true;
#else
    (void)path;
    (void)framebuffer;
    (void)width;
    (void)height;
    return false;
#endif
}

bool denoise_oidn_if_available(const std::vector<Vec3>& beauty,
                               const std::vector<Vec3>& albedo,
                               const std::vector<Vec3>& normal,
                               int width,
                               int height,
                               std::vector<Vec3>& denoised,
                               std::string& message)
{
    const char* oidn_bin = std::getenv("SR_RT_OIDN_DENOISE_BIN");
    if (oidn_bin && oidn_bin[0])
        return denoise_oidn_cli_if_available(beauty, albedo, normal, width, height, denoised, message);

    if (denoise_oidn_cli_if_available(beauty, albedo, normal, width, height, denoised, message))
        return true;

#if HAVE_OIDN
    try {
        const int n = width * height;

        // Prefer the CPU backend for packaged-app stability on macOS.
        oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
        device.commit();

        oidn::BufferRef color_buf = device.newBuffer(3 * n * sizeof(float));
        oidn::BufferRef albedo_buf = device.newBuffer(3 * n * sizeof(float));
        oidn::BufferRef normal_buf = device.newBuffer(3 * n * sizeof(float));
        oidn::BufferRef output_buf = device.newBuffer(3 * n * sizeof(float));

        float* color = static_cast<float*>(color_buf.getData());
        float* alb = static_cast<float*>(albedo_buf.getData());
        float* nrm = static_cast<float*>(normal_buf.getData());
        float* out = static_cast<float*>(output_buf.getData());

        for (int i = 0; i < n; ++i) {
            color[3 * i + 0] = static_cast<float>(beauty[i].x);
            color[3 * i + 1] = static_cast<float>(beauty[i].y);
            color[3 * i + 2] = static_cast<float>(beauty[i].z);

            alb[3 * i + 0] = static_cast<float>(albedo[i].x);
            alb[3 * i + 1] = static_cast<float>(albedo[i].y);
            alb[3 * i + 2] = static_cast<float>(albedo[i].z);

            nrm[3 * i + 0] = static_cast<float>(normal[i].x);
            nrm[3 * i + 1] = static_cast<float>(normal[i].y);
            nrm[3 * i + 2] = static_cast<float>(normal[i].z);
        }

        oidn::FilterRef filter = device.newFilter("RT");
        filter.setImage("color", color_buf, oidn::Format::Float3, width, height);
        filter.setImage("albedo", albedo_buf, oidn::Format::Float3, width, height);
        filter.setImage("normal", normal_buf, oidn::Format::Float3, width, height);
        filter.setImage("output", output_buf, oidn::Format::Float3, width, height);
        filter.set("hdr", true);
        filter.set("cleanAux", false);
        filter.commit();
        filter.execute();

        const char* err_msg = nullptr;
        if (device.getError(err_msg) != oidn::Error::None) {
            message = err_msg ? err_msg : "unknown OIDN error";
            return false;
        }

        denoised.assign(n, Vec3{0, 0, 0});
        for (int i = 0; i < n; ++i) {
            denoised[i] = {
                static_cast<double>(out[3 * i + 0]),
                static_cast<double>(out[3 * i + 1]),
                static_cast<double>(out[3 * i + 2])
            };
        }
        message = "OIDN denoise complete";
        return true;
    } catch (const std::exception& e) {
        message = e.what();
        return false;
    }
#else
    (void)beauty;
    (void)albedo;
    (void)normal;
    (void)width;
    (void)height;
    (void)denoised;
    message = "OIDN not enabled at build time (compile with -DENABLE_OIDN and link OpenImageDenoise)";
    return false;
#endif
}
