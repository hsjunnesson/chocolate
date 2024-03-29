#include "engine/canvas.h"
#include "engine/color.inl"
#include "engine/config.h"
#include "engine/engine.h"
#include "engine/ini.h"
#include "engine/log.h"
#include "engine/math.inl"
#include "engine/shader.h"
#include "engine/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "engine/stb_image_write.h"

#include <GLFW/glfw3.h>
#include <array.h>
#include <cassert>
#include <emmintrin.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <hash.h>
#include <limits>
#include <memory.h>
#include <murmur_hash.h>
#include <string>
#include <string_stream.h>
#include <temp_allocator.h>

// clang-format off
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif
// clang-format on

using engine::Canvas;
using engine::Engine;

namespace {
const char *vertex_source = R"(
#version 410 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_texture_coords;

smooth out vec2 uv;

void main() {
    gl_Position = vec4(in_position.x, in_position.y, 0.0, 1.0);
    uv = in_texture_coords;
}
)";

const char *fragment_source = R"(
#version 410 core

precision highp float;

uniform sampler2D texture0;
smooth in vec2 uv;

out vec4 out_color;

void main() {
    out_color = texture(texture0, uv);
}
)";

math::Vertex vertices[]{
    {// top right
     {1.0f, 1.0f, 0.0f},
     {1.0f, 1.0f, 1.0f, 1.0f},
     {1.0f, 0.0f}},
    {// bottom right
     {1.0f, -1.0f, 0.0f},
     {1.0f, 1.0f, 1.0f, 1.0f},
     {1.0f, 1.0f}},
    {// bottom left
     {-1.0f, -1.0f, 0.0f},
     {1.0f, 1.0f, 1.0f, 1.0f},
     {0.0f, 1.0f}},
    {// top left
     {-1.0f, 1.0f, 0.0f},
     {1.0f, 1.0f, 1.0f, 1.0f},
     {0.0f, 0.0f}},
};

GLuint indices[] = {
    0, 1, 3, // first Triangle
    1, 2, 3  // second Triangle
};

} // namespace

namespace engine {

using namespace foundation;

Canvas::Canvas(Allocator &allocator)
: allocator(allocator)
, shader(nullptr)
, texture(0)
, vao(0)
, vbo(0)
, ebo(0)
, data(allocator)
, width(0)
, height(0)
, sprites_data(allocator)
, sprites_data_width(0)
, sprites_indices(allocator)
, sprite_size(0)
, clip_mask({{0, 0}, {-1, -1}}) {
    shader = MAKE_NEW(allocator, Shader, nullptr, vertex_source, fragment_source, "Canvas");

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glGenTextures(1, &texture);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(math::Vertex), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices, GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(math::Vertex), (const GLvoid *)0);
    glEnableVertexAttribArray(0);

    // texture_coords
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(math::Vertex), (const GLvoid *)offsetof(math::Vertex, texture_coords));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glObjectLabel(GL_VERTEX_ARRAY, vao, -1, "Canvas Vertex Array Object");
    glObjectLabel(GL_BUFFER, vbo, -1, "Canvas Vertex Buffer Object");
    glObjectLabel(GL_BUFFER, ebo, -1, "Canvas Element Array Buffer Object");
}

Canvas::~Canvas() {
    MAKE_DELETE(allocator, Shader, shader);

    if (vbo) {
        glDeleteBuffers(1, &vbo);
    }

    if (ebo) {
        glDeleteBuffers(1, &ebo);
    }

    if (vao) {
        glDeleteVertexArrays(1, &vao);
    }

    if (texture) {
        glDeleteTextures(1, &texture);
    }
}

void init_canvas(const Engine &engine, Canvas &canvas, const ini_t *config, Array<uint8_t> *sprites_data) {
    init_canvas(engine.window_rect.size.x / engine.render_scale, engine.window_rect.size.y / engine.render_scale, canvas, config, sprites_data);
}

void init_canvas(int32_t width, int32_t height, Canvas &canvas, const ini_t *config, Array<uint8_t> *sprites_data) {
    assert(config != nullptr);

    glBindTexture(GL_TEXTURE_2D, canvas.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    canvas.width = width;
    canvas.height = height;
    int32_t size = canvas.width * canvas.height * 4;
    array::resize(canvas.data, size);

    canvas::clear(canvas, engine::color::black);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvas.width, canvas.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, array::begin(canvas.data));
    glObjectLabel(GL_TEXTURE, canvas.texture, -1, "Canvas Texture");

    glUseProgram(canvas.shader->program);
    GLint z_offset = glGetUniformLocation(canvas.shader->program, "z_offset");
    glUniform1f(z_offset, 1.0f);

    {
        if (!engine::config::has_property(config, "canvas", "sprite_size")) {
            log_fatal("Config file missing [canvas] sprite_size]");
        }

        canvas.sprite_size = atoi(engine::config::read_property(config, "canvas", "sprite_size"));
    }

    if (sprites_data == nullptr) {
        const char *sprites_filename = engine::config::read_property(config, "canvas", "sprites_filename");
        if (!sprites_filename) {
            log_fatal("Config file missing [canvas] sprites_filename");
        }

        int channels, sprites_width, sprites_height;
        unsigned char *data = stbi_load(sprites_filename, &sprites_width, &sprites_height, &channels, 0);
        if (!data) {
            log_fatal("Couldn't load texture %s: %s", sprites_filename, stbi_failure_reason());
        }

        canvas.sprites_data_width = sprites_width;
        array::resize(canvas.sprites_data, sprites_width * sprites_height * 4);

        if (channels == 4) {
            memcpy(array::begin(canvas.sprites_data), data, sprites_width * sprites_height * 4);
        } else {
            for (int x = 0; x < sprites_width; ++x) {
                for (int y = 0; y < sprites_height; ++y) {
                    if (channels == 3) {
                        array::push_back(canvas.sprites_data, data[(y * sprites_width + x) * 3 + 0]);
                        array::push_back(canvas.sprites_data, data[(y * sprites_width + x) * 3 + 1]);
                        array::push_back(canvas.sprites_data, data[(y * sprites_width + x) * 3 + 2]);
                        array::push_back(canvas.sprites_data, (unsigned char)255);
                    } else if (channels == 1) {
                        array::push_back(canvas.sprites_data, data[y * sprites_width + x]);
                        array::push_back(canvas.sprites_data, data[y * sprites_width + x]);
                        array::push_back(canvas.sprites_data, data[y * sprites_width + x]);
                        array::push_back(canvas.sprites_data, (unsigned char)255);
                    }
                }
            }
        }

        stbi_image_free(data);
    } else {
        uint32_t sprites_data_size = array::size(*sprites_data);
        uint32_t channels = 4;
        if (sprites_data_size % channels != 0) {
            log_fatal("Canvas sprites data invalid format.");
        }

        uint32_t side_length = static_cast<uint32_t>(sqrt(sprites_data_size));
        bool power_of_two = (side_length != 0) && ((side_length & (side_length - 1)) == 0);
        if (!power_of_two) {
            log_fatal("Canvas sprites data not power of two");
        }

        canvas.sprites_data = *sprites_data;
        canvas.sprites_data_width = static_cast<int32_t>(sqrt(sprites_data_size / channels));
    }

    // Read all key-values into canvas.sprites_indices
    {
        using namespace foundation;

        int canvas_section_index = ini_find_section(config, "canvas", 0);
        if (canvas_section_index == INI_NOT_FOUND) {
            log_fatal("Config file missing [canvas]");
        }

        for (int i = 0; i < ini_property_count(config, canvas_section_index); ++i) {
            const char *name = ini_property_name(config, canvas_section_index, i);
            const char *value = ini_property_value(config, canvas_section_index, i);

            size_t name_len = strlen(name);

            // Trim off trailing space when you do "NAME = VALUE"
            if (name[name_len - 1] == ' ') {
                --name_len;
            }

            uint64_t name_key = murmur_hash_64(name, (uint32_t)name_len, 0);

            try {
                long val = std::stol(value);
                if (val > std::numeric_limits<int32_t>::max() || val < std::numeric_limits<int32_t>::min()) {
                    log_error("Value out of range [canvas] %s", name);
                    continue;
                }

                uint32_t num = static_cast<uint32_t>(val);
                hash::set(canvas.sprites_indices, name_key, num);
            } catch (...) {
                continue;
            }
        }
    }
}

void render_canvas(const Engine &engine, Canvas &canvas) {
    (void)engine;
    assert(canvas.texture);

    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "render canvas");

    const GLuint shader_program = canvas.shader->program;

    glUseProgram(shader_program);
    glBindVertexArray(canvas.vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, canvas.texture);

    // TODO: Add a subimage uint8_t buffer. Rename subimage to subimage_rect. Draw operations work only in the subimage.
    // Enqueue draw operations
    // Calculate the whole bounds of the draw operation
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, canvas.width, canvas.height, GL_RGBA, GL_UNSIGNED_BYTE, array::begin(canvas.data));

    glUniform1i(glGetUniformLocation(shader_program, "texture0"), 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    glPopDebugGroup();
}

void write_png(const Canvas &canvas, const char *filename) {
    const int comp = 4;
    if (canvas.clip_mask.size.x == -1) {
        stbi_write_png(filename, canvas.width, canvas.height, comp, array::begin(canvas.data), canvas.width * comp);
    } else {
        uint32_t pixel_data_length = canvas.clip_mask.size.x * canvas.clip_mask.size.y * comp;
        uint8_t *pixel_data = (uint8_t *)canvas.allocator.allocate(sizeof(uint8_t) * pixel_data_length);
        if (!pixel_data) {
            log_fatal("Could not allocate data");
        }

        const uint8_t *canvas_data = array::begin(canvas.data);
        for (int32_t y = 0; y < canvas.clip_mask.size.y; ++y) {
            for (int32_t x = 0; x < canvas.clip_mask.size.x; ++x) {
                int src_index = ((canvas.clip_mask.origin.y + y) * canvas.width + (canvas.clip_mask.origin.x + x)) * comp;
                int dest_index = (y * canvas.clip_mask.size.x + x) * comp;
                memcpy(&pixel_data[dest_index], &canvas_data[src_index], comp);
            }
        }

        stbi_write_png(filename, canvas.clip_mask.size.x, canvas.clip_mask.size.y, comp, pixel_data, canvas.clip_mask.size.x * comp);

        canvas.allocator.deallocate(pixel_data);
    }
}

void print(const Canvas &canvas, const char *printer) {
    math::Rect clip_mask = canvas.clip_mask;
    if (clip_mask.size.x == -1) {
        clip_mask.origin.x = 0;
        clip_mask.origin.y = 0;
        clip_mask.size.x = canvas.width;
        clip_mask.size.y = canvas.height;
    }

    const int comp = 4;

    uint32_t pixel_data_length = clip_mask.size.x * clip_mask.size.y * comp;
    uint8_t *pixel_data = (uint8_t *)canvas.allocator.allocate(sizeof(uint8_t) * pixel_data_length);
    if (!pixel_data) {
        log_fatal("Could not allocate data");
    }

    const uint8_t *canvas_data = array::begin(canvas.data);
    for (int32_t y = 0; y < canvas.clip_mask.size.y; ++y) {
        for (int32_t x = 0; x < canvas.clip_mask.size.x; ++x) {
            int src_index = ((canvas.clip_mask.origin.y + y) * canvas.width + (canvas.clip_mask.origin.x + x)) * comp;
            int dest_index = (y * canvas.clip_mask.size.x + x) * comp;
            memcpy(&pixel_data[dest_index], &canvas_data[src_index], comp);
        }
    }

#if defined(_WIN32)
    // Initialize a document
    DOCINFOW di = {sizeof(DOCINFOW), L"My Document", NULL};

    int printer_length = MultiByteToWideChar(CP_ACP, 0, printer, -1, NULL, 0);
    wchar_t *wide_printer = (wchar_t *)canvas.allocator.allocate(sizeof(wchar_t) * printer_length);
    MultiByteToWideChar(CP_ACP, 0, printer, -1, wide_printer, printer_length);

    // Start a print job
    HDC printerDC = CreateDCW(L"WINSPOOL", wide_printer, NULL, NULL);
    if (printerDC == NULL) {
        canvas.allocator.deallocate(pixel_data);
        canvas.allocator.deallocate(wide_printer);
        log_error("Failed to create a DC for the printer.");
        return;
    }

    canvas.allocator.deallocate(wide_printer);

    if (StartDocW(printerDC, &di) <= 0) {
        DeleteDC(printerDC);
        canvas.allocator.deallocate(pixel_data);
        log_error("Failed to start a document.");
        return;
    }

    if (StartPage(printerDC) <= 0) {
        EndDoc(printerDC);
        DeleteDC(printerDC);
        canvas.allocator.deallocate(pixel_data);
        log_error("Failed to start a page.");
        return;
    }

    const int width = clip_mask.size.x;
    const int height = clip_mask.size.y;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // Negative for top-down bitmap
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32; // 32 bits per pixel for RGBA
    bmi.bmiHeader.biCompression = BI_RGB;

    // Draw the bitmap
    StretchDIBits(
        printerDC,
        0, 0, width, height,
        0, 0, width, height,
        pixel_data,
        &bmi,
        DIB_RGB_COLORS,
        SRCCOPY);

    // Finish the print job
    if (EndPage(printerDC) <= 0) {
        log_error("Failed to end the page.");
    }

    if (EndDoc(printerDC) <= 0) {
        log_error("Failed to end the document.");
    }

    // Clean up
    DeleteDC(printerDC);
#elif
    log_error("Platform not supported");
#endif

    canvas.allocator.deallocate(pixel_data);
}

void canvas::clip(Canvas &canvas) {
    canvas.clip_mask.origin = {0, 0};
    canvas.clip_mask.size = {-1, -1};
}

void canvas::clip(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
    assert(x2 >= x1);
    assert(y2 >= y1);

    canvas.clip_mask.origin = {x1, y1};
    canvas.clip_mask.size = {x2 - x1, y2 - y1};
}

// Returns true if coordinate lays inside the clip mask.
bool is_clipped(Canvas &canvas, int32_t x, int32_t y) {
    if (canvas.clip_mask.size.x == -1) {
        return true;
    }

    return x >= canvas.clip_mask.origin.x &&
           x < (canvas.clip_mask.origin.x + canvas.clip_mask.size.x) &&
           y >= canvas.clip_mask.origin.y &&
           y < (canvas.clip_mask.origin.y + canvas.clip_mask.size.y);
}

void canvas::pset(Canvas &canvas, int32_t x, int32_t y, glm::vec4 col) {
    if (x < 0 || y < 0 || x >= canvas.width || y >= canvas.height) {
        return;
    }

    if (!is_clipped(canvas, x, y)) {
        return;
    }

    uint32_t i = 4 * math::index(x, y, canvas.width);
    uint8_t r = (uint8_t)(col.r * 255);
    uint8_t g = (uint8_t)(col.g * 255);
    uint8_t b = (uint8_t)(col.b * 255);
    uint8_t a = (uint8_t)(col.a * 255);
    canvas.data[i] = r;
    canvas.data[i + 1] = g;
    canvas.data[i + 2] = b;
    canvas.data[i + 3] = a;
}

void clear_simd(Canvas &canvas, glm::vec4 col) {
    uint8_t r = (uint8_t)(col.r * 255);
    uint8_t g = (uint8_t)(col.g * 255);
    uint8_t b = (uint8_t)(col.b * 255);
    uint8_t a = (uint8_t)(col.a * 255);

    uint32_t packed_color = (a << 24) | (b << 16) | (g << 8) | r;
    __m128i color = _mm_set1_epi32(packed_color);
    int32_t num_pixels = canvas.width * canvas.height;
    int32_t num_iterations = num_pixels / 4;

    for (int32_t i = 0; i < num_iterations; ++i) {
        _mm_storeu_si128(reinterpret_cast<__m128i *>(&canvas.data[i * 16]), color);
    }

    // remaining pixels
    for (int32_t i = num_iterations * 4; i < num_pixels; ++i) {
        int32_t index = i * 4;
        canvas.data[index] = r;
        canvas.data[index + 1] = g;
        canvas.data[index + 2] = b;
        canvas.data[index + 3] = a;
    }
}

void canvas::clear(Canvas &canvas, glm::vec4 col) {
    if (canvas.clip_mask.size.x == -1) {
        clear_simd(canvas, col);
        return;
    }

    uint8_t r = (uint8_t)(col.r * 255);
    uint8_t g = (uint8_t)(col.g * 255);
    uint8_t b = (uint8_t)(col.b * 255);
    uint8_t a = (uint8_t)(col.a * 255);

    int32_t max = canvas.width * canvas.height * 4;
    for (int32_t i = 0; i < max; i += 4) {
        int32_t x;
        int32_t y;
        math::coord(i / 4, x, y, canvas.width);

        if (is_clipped(canvas, x, y)) {
            canvas.data[i] = r;
            canvas.data[i + 1] = g;
            canvas.data[i + 2] = b;
            canvas.data[i + 3] = a;
        }
    }
}

void canvas::print(Canvas &canvas, const char *str, int32_t x, int32_t y, glm::vec4 col, uint8_t scale_w, uint8_t scale_h, bool invert, bool mask, glm::vec4 mask_col) {
    if (array::empty(canvas.sprites_data)) {
        log_fatal("Attempting to canvas::print without sprites");
    }

    int32_t xx = x;
    int32_t yy = y;

    for (int32_t i = 0; i < (int32_t)strlen(str); ++i) {
        char c = str[i];

        if (c == ' ') {
            xx += canvas.sprite_size * scale_w;
            continue;
        }

        if (c == '\n') {
            yy += canvas.sprite_size * scale_h;
            xx = x;
            continue;
        }

        const char *character_key = canvas::character_key(c);
        if (!character_key) {
            log_fatal("print with missing character key %c", c);
        }

        uint64_t key = murmur_hash_64(character_key, (uint32_t)strlen(character_key), 0);
        if (!hash::has(canvas.sprites_indices, key)) {
            log_fatal("Missing sprite index for %s", character_key);
        }

        uint32_t sprite_index = hash::get(canvas.sprites_indices, key, (uint32_t)0);
        sprite(canvas, sprite_index, xx, yy, col, 1, 1, scale_w, scale_h, false, false, invert, mask, mask_col);

        xx += canvas.sprite_size * scale_w;
    }
}

void canvas::circle(Canvas &canvas, int32_t x_center, int32_t y_center, int32_t r, glm::vec4 col) {
    if (r <= 0.0f) {
        return;
    }

    int32_t x = r;
    int32_t y = 0;
    int32_t p = 1 - r;

    while (x >= y) {
        pset(canvas, (int32_t)(x_center + x), (int32_t)(y_center + y), col);
        pset(canvas, (int32_t)(x_center - x), (int32_t)(y_center + y), col);
        pset(canvas, (int32_t)(x_center + x), (int32_t)(y_center - y), col);
        pset(canvas, (int32_t)(x_center - x), (int32_t)(y_center - y), col);
        pset(canvas, (int32_t)(x_center + y), (int32_t)(y_center + x), col);
        pset(canvas, (int32_t)(x_center - y), (int32_t)(y_center + x), col);
        pset(canvas, (int32_t)(x_center + y), (int32_t)(y_center - x), col);
        pset(canvas, (int32_t)(x_center - y), (int32_t)(y_center - x), col);

        ++y;

        if (p <= 0) {
            p = p + 2 * y + 1;
        } else {
            if (p + 2 * (y - x + 1) < 0) {
                pset(canvas, (int32_t)(x_center + x), (int32_t)(y_center + y - 1), col);
                pset(canvas, (int32_t)(x_center - x), (int32_t)(y_center + y - 1), col);
                pset(canvas, (int32_t)(x_center + x), (int32_t)(y_center - y + 1), col);
                pset(canvas, (int32_t)(x_center - x), (int32_t)(y_center - y + 1), col);
            }
            --x;
            p = p + 2 * y - 2 * x + 1;
        }
    }
}

// Draw a horizontal, straight line using SIMD
void line_simd(Canvas &canvas, int32_t x0, int32_t y, int32_t x1, glm::vec4 col) {
    y = (y >= 0) ? y : 0;
    y = (y < canvas.height) ? y : canvas.height - 1;
    x0 = (x0 >= 0) ? x0 : 0;
    x1 = (x1 < canvas.width) ? x1 : canvas.width - 1;

    if (x0 > x1) {
        std::swap(x0, x1);
    }

    uint8_t r = static_cast<uint8_t>(col.r * 255);
    uint8_t g = static_cast<uint8_t>(col.g * 255);
    uint8_t b = static_cast<uint8_t>(col.b * 255);
    uint8_t a = static_cast<uint8_t>(col.a * 255);

    uint32_t packedColor = (a << 24) | (b << 16) | (g << 8) | r;
    __m128i color = _mm_set1_epi32(packedColor);

    int32_t end = x1 - (x1 - x0) % 4;

    // SIMD loop for bulk pixel setting
    for (int32_t i = x0; i < end; i += 4) {
        _mm_storeu_si128(reinterpret_cast<__m128i *>(&canvas.data[(y * canvas.width + i) * 4]), color);
    }

    // Handle remaining pixels
    for (int32_t i = end; i <= x1; ++i) {
        int32_t index = (y * canvas.width + i) * 4;
        canvas.data[index] = r;
        canvas.data[index + 1] = g;
        canvas.data[index + 2] = b;
        canvas.data[index + 3] = a;
    }
}

void canvas::circle_fill(Canvas &canvas, int32_t x_center, int32_t y_center, int32_t r, glm::vec4 col) {
    if (r <= 0.0f) {
        return;
    }

    int32_t x = r;
    int32_t y = 0;
    int32_t p = 1 - r;

    bool has_clip_mask = canvas.clip_mask.size.x != -1;

    while (x >= y) {
        if (has_clip_mask) {
            line(canvas, x_center - x, y_center + y, x_center + x, y_center + y, col);
            line(canvas, x_center - x, y_center - y, x_center + x, y_center - y, col);
            line(canvas, x_center - y, y_center + x, x_center + y, y_center + x, col);
            line(canvas, x_center - y, y_center - x, x_center + y, y_center - x, col);
        } else {
            line_simd(canvas, x_center - x, y_center + y, x_center + x, col);
            line_simd(canvas, x_center - x, y_center - y, x_center + x, col);
            line_simd(canvas, x_center - y, y_center + x, x_center + y, col);
            line_simd(canvas, x_center - y, y_center - x, x_center + y, col);
        }

        ++y;

        if (p <= 0) {
            p = p + 2 * y + 1;
        } else {
            if (p + 2 * (y - x + 1) < 0) {
                if (has_clip_mask) {
                    line(canvas, x_center - x, y_center + y - 1, x_center + x, y_center + y - 1, col);
                    line(canvas, x_center - x, y_center - y + 1, x_center + x, y_center - y + 1, col);
                } else {
                    line_simd(canvas, x_center - x, y_center + y - 1, x_center + x, col);
                    line_simd(canvas, x_center - x, y_center - y + 1, x_center + x, col);
                }
            }
            --x;
            p = p + 2 * y - 2 * x + 1;
        }
    }
}

void canvas::line(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2, glm::vec4 col) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int stepx = x1 < x2 ? 1 : -1;
    int stepy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    while (true) {
        pset(canvas, x1, y1, col);

        if (x1 == x2 && y1 == y2)
            break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x1 += stepx;
        }
        if (e2 < dy) {
            err += dx;
            y1 += stepy;
        }
    }
}

void canvas::rectangle(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2, glm::vec4 col) {
    line(canvas, x1, y1, x2, y1, col);
    line(canvas, x2, y1, x2, y2, col);
    line(canvas, x2, y2, x1, y2, col);
    line(canvas, x1, y2, x1, y1, col);
}

void rectangle_fill_simd(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2, glm::vec4 col) {
    int32_t min_x = std::min(x1, x2);
    int32_t max_x = std::max(x1, x2);
    int32_t min_y = std::min(y1, y2);
    int32_t max_y = std::max(y1, y2);

    for (int32_t y = min_y; y < max_y; ++y) {
        line_simd(canvas, min_x, y, max_x, col);
    }
}

void canvas::rectangle_fill(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2, glm::vec4 col) {
    if (canvas.clip_mask.size.x == -1) {
        rectangle_fill_simd(canvas, x1, y1, x2, y2, col);
        return;
    }

    int32_t min_x = std::min(x1, x2);
    int32_t max_x = std::max(x1, x2);
    int32_t min_y = std::min(y1, y2);
    int32_t max_y = std::max(y1, y2);

    for (int32_t y = min_y; y < max_y; ++y) {
        for (int32_t x = min_x; x < max_x; ++x) {
            pset(canvas, x, y, col);
        }
    }
}

inline float edge_function(const glm::vec2 &v0, const glm::vec2 &v1, float px, float py) {
    return (px - v0.x) * (v1.y - v0.y) - (py - v0.y) * (v1.x - v0.x);
}

inline __m128 edge_function_sse(const __m128 &v0x, const __m128 &v0y, const __m128 &v1x, const __m128 &v1y, const __m128 &px, const __m128 &py) {
    return _mm_sub_ps(_mm_mul_ps(_mm_sub_ps(px, v0x), _mm_sub_ps(v1y, v0y)), _mm_mul_ps(_mm_sub_ps(py, v0y), _mm_sub_ps(v1x, v0x)));
}

void canvas::triangle_fill(Canvas &canvas, glm::vec2 v0, glm::vec2 v1, glm::vec2 v2, glm::vec4 col) {
    float signed_area = (v0.x - v2.x) * (v1.y - v2.y) - (v1.x - v2.x) * (v0.y - v2.y);
    if (signed_area > 0.0f) {
        std::swap(v1, v2);
    }

    float min_x = std::min({v0.x, v1.x, v2.x});
    float min_y = std::min({v0.y, v1.y, v2.y});
    float max_x = std::max({v0.x, v1.x, v2.x});
    float max_y = std::max({v0.y, v1.y, v2.y});

    for (int y = static_cast<int>(min_y); y <= static_cast<int>(max_y); ++y) {
        int x = static_cast<int>(min_x);
        for (; x <= static_cast<int>(max_x) - 4; x += 4) {
            __m128 px = _mm_set_ps(x + 3, x + 2, x + 1, x);
            __m128 py = _mm_set1_ps(y);

            __m128 w0 = edge_function_sse(_mm_set1_ps(v1.x), _mm_set1_ps(v1.y), _mm_set1_ps(v2.x), _mm_set1_ps(v2.y), px, py);
            __m128 w1 = edge_function_sse(_mm_set1_ps(v2.x), _mm_set1_ps(v2.y), _mm_set1_ps(v0.x), _mm_set1_ps(v0.y), px, py);
            __m128 w2 = edge_function_sse(_mm_set1_ps(v0.x), _mm_set1_ps(v0.y), _mm_set1_ps(v1.x), _mm_set1_ps(v1.y), px, py);

            int mask = _mm_movemask_ps(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(w0, _mm_setzero_ps()), _mm_cmpge_ps(w1, _mm_setzero_ps())), _mm_cmpge_ps(w2, _mm_setzero_ps())));

            for (int i = 0; i < 4; ++i) {
                if (mask & (1 << i)) {
                    pset(canvas, x + i, y, col);
                }
            }
        }

        for (; x <= static_cast<int>(max_x); ++x) {
            float w0 = edge_function(v1, v2, x, y);
            float w1 = edge_function(v2, v0, x, y);
            float w2 = edge_function(v0, v1, x, y);

            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                pset(canvas, x, y, col);
            }
        }
    }
}

void canvas::sprite(Canvas &canvas, uint32_t n, int32_t x, int32_t y, glm::vec4 col, uint8_t w, uint8_t h, uint8_t scale_w, uint8_t scale_h, bool flip_x, bool flip_y, bool invert, bool mask, glm::vec4 mask_col) {
    if (array::empty(canvas.sprites_data)) {
        log_fatal("Attempting to canvas::sprite without sprites");
    }

    const int32_t sprite_size = canvas.sprite_size;
    const int32_t sprite_width = sprite_size * w;
    const int32_t sprite_height = sprite_size * h;

    // Start of source from sprites_data in pixels.
    uint32_t source_data_start;

    {
        const uint32_t sprites_per_row = canvas.sprites_data_width / sprite_size;
        const uint32_t row = n / sprites_per_row;
        const uint32_t column = n % sprites_per_row;
        source_data_start = (row * canvas.sprites_data_width * sprite_size + column * sprite_size);
    }

    uint8_t mask_red = static_cast<uint8_t>(255 * mask_col.r);
    uint8_t mask_green = static_cast<uint8_t>(255 * mask_col.g);
    uint8_t mask_blue = static_cast<uint8_t>(255 * mask_col.b);

    for (int32_t jj = 0; jj < sprite_height * scale_h; ++jj) {
        for (int32_t ii = 0; ii < sprite_width * scale_w; ++ii) {
            if (x + ii < 0 || x + ii >= canvas.width || y + jj < 0 || y + jj >= canvas.height) {
                continue;
            }

            if (!is_clipped(canvas, x + ii, y + jj)) {
                continue;
            }

            int32_t src_ii = flip_x ? ((sprite_width - 1) - (ii / scale_w)) : ii / scale_w;
            int32_t src_jj = flip_y ? ((sprite_height - 1) - (jj / scale_h)) : jj / scale_h;

            const int32_t src = (source_data_start * 4) + (src_ii * 4) + (src_jj * 4 * canvas.sprites_data_width);
            const int32_t dst = math::index(x + ii, y + jj, canvas.width) * 4;

            uint8_t sprite_red = invert ? 255 - canvas.sprites_data[src + 0] : canvas.sprites_data[src + 0];
            uint8_t sprite_green = invert ? 255 - canvas.sprites_data[src + 1] : canvas.sprites_data[src + 1];
            uint8_t sprite_blue = invert ? 255 - canvas.sprites_data[src + 2] : canvas.sprites_data[src + 2];
            uint8_t sprite_alpha = canvas.sprites_data[src + 3];

            uint8_t red = static_cast<uint8_t>(sprite_red * col.r);
            uint8_t green = static_cast<uint8_t>(sprite_green * col.g);
            uint8_t blue = static_cast<uint8_t>(sprite_blue * col.b);
            uint8_t alpha = static_cast<uint8_t>(sprite_alpha * col.a);

            if (mask) {
                if (mask_red == sprite_red && mask_blue == sprite_blue && mask_green == sprite_green) {
                    continue;
                }
            }

            canvas.data[dst + 0] = red;
            canvas.data[dst + 1] = green;
            canvas.data[dst + 2] = blue;
            canvas.data[dst + 3] = alpha;
        }
    }
}

} // namespace engine
