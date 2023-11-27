#pragma once

#include "math.inl"

#include <collection_types.h>
#include <engine/color.inl>
#include <memory_types.h>

typedef struct ini_t ini_t;

namespace engine {

struct Engine;
struct Shader;

using foundation::Allocator;
using foundation::Array;
using foundation::Hash;
using math::Color4f;

struct Canvas {
    Canvas(Allocator &allocator);
    ~Canvas();

    Allocator &allocator;
    Shader *shader;
    unsigned int texture;
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;

    // The array of chars backing the texture that is this Canavs. In GL_RGBA8 format.
    Array<uint8_t> data;

    // The pixel width of the screenspace canvas.
    int32_t width;

    // The pixel height of the screenspace canvas.
    int32_t height;

    // Sprite data available to blit with spr() or print(). Empty if no sprites_filename in config.
    Array<uint8_t> sprites_data;

    // Tilemap width in pixels.
    int32_t sprites_data_width;

    // A lookup table for indices into the sprites tilemap, example key "char_c"
    Hash<uint32_t> sprites_indices;

    // The square pixel size of a sprite in the sprites tilemap.
    int32_t sprite_size;

    // The rect that is the clip mask.
    math::Rect clip_mask;
};

/** @brief Initializes the canvas with the resolution of the engine window, i.e. engine.resolution / engine.render_scale.
 * @param engine The `Engine` object.
 * @param canvas The `Canvas` to initialize.
 * @param config The `ini_t` to read config values from.
 * @param sprites_data An optional buffer of pixels to read from memory instead of reading from [canvas] sprites_filename ini value.
 * The buffer is assumed to be 32-bit RBGA.
 */
void init_canvas(const Engine &engine, Canvas &canvas, const ini_t *config, Array<uint8_t> *sprites_data = nullptr);

/** @brief Initializes the canvas with a fixed resolution.
 * @param width The resolution width in pixels of the canvas.
 * @param height The resolution height in pixels of the canvas.
 * @param canvas The `Canvas` to initialize.
 * @param config The `ini_t` to read config values from.
 * @param sprites_data An optional buffer of pixels to read from memory instead of reading from [canvas] sprites_filename ini value.
 * The buffer is assumed to be 32-bit RBGA.
 */
void init_canvas(int32_t width, int32_t height, Canvas &canvas, const ini_t *config, Array<uint8_t> *sprites_data = nullptr);

/** Renders the canvas
 * @param engine The `Engine` object.
 * @param canvas The `Canvas` to render.
 */
void render_canvas(const Engine &engine, Canvas &canvas);

/** Writes the canvas to a PNG.
 * @param canvas The `Canvas` to save.
 * @param filename The filename to save to.
 * This operation honors the clipping mask. Setting a clipping mask saves only that part of the canvas.
 */
void write_png(const Canvas &canvas, const char *filename);

/** Prints the canvas through the OS to a printer.
 * @param canvas The `Canvas` to print.
 * @param printer The name of the printer.
 * This operation honors the clipping mask. Setting a clipping mask prints only that part of the canvas.
 */
void print(const Canvas &canvas, const char *printer);

namespace canvas {

// Clear the clipping mask.
void clip(Canvas &canvas);

// Sets the clipping mask. Pixels will only be drawn painted inside this rectangle.
void clip(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2);

void pset(Canvas &canvas, int32_t x, int32_t y, Color4f col);
void clear(Canvas &canvas, Color4f col);
void print(Canvas &canvas, const char *str, int32_t x, int32_t y, Color4f col, bool invert = false, bool mask = true, Color4f mask_col = engine::color::black);
void circle(Canvas &canvas, int32_t x_center, int32_t y_center, int32_t r, Color4f col);
void circle_fill(Canvas &canvas, int32_t x_center, int32_t y_center, int32_t r, Color4f col);
void line(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2, Color4f col);
void rectangle(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2, Color4f col);
void rectangle_fill(Canvas &canvas, int32_t x1, int32_t y1, int32_t x2, int32_t y2, Color4f col);

// Draw sprite at position `x`, `y`. `w` and `h` determine how many sprites wide and tall to blit.
void sprite(Canvas &canvas, uint32_t n, int32_t x, int32_t y, Color4f col = engine::color::white, uint8_t w = 1, uint8_t h = 1, uint8_t scale_w = 1, uint8_t scale_h = 1, bool flip_x = false, bool flip_y = false, bool invert = false, bool mask = true, Color4f mask_col = engine::color::black);

// Returns a key used to lookup the sprite to blit for a character using the print() function.
constexpr const char *character_key(char c) {
    switch (c) {
    case 'a':
        return "char_a";
    case 'b':
        return "char_b";
    case 'c':
        return "char_c";
    case 'd':
        return "char_d";
    case 'e':
        return "char_e";
    case 'f':
        return "char_f";
    case 'g':
        return "char_g";
    case 'h':
        return "char_h";
    case 'i':
        return "char_i";
    case 'j':
        return "char_j";
    case 'k':
        return "char_k";
    case 'l':
        return "char_l";
    case 'm':
        return "char_m";
    case 'n':
        return "char_n";
    case 'o':
        return "char_o";
    case 'p':
        return "char_p";
    case 'q':
        return "char_q";
    case 'r':
        return "char_r";
    case 's':
        return "char_s";
    case 't':
        return "char_t";
    case 'u':
        return "char_u";
    case 'v':
        return "char_v";
    case 'w':
        return "char_w";
    case 'x':
        return "char_x";
    case 'y':
        return "char_y";
    case 'z':
        return "char_z";
    // case '¶':
    //     return "char_paragraph";
    case '!':
        return "char_exclamation";
    case '"':
        return "char_doublequote";
    case '\'':
        return "char_quote";
    case '#':
        return "char_hash";
    case '$':
        return "char_dollar";
    case '`':
        return "char_backtick";
    case '(':
        return "char_open_parenthesis";
    case ')':
        return "char_close_parenthesis";
    case '*':
        return "char_asterisk";
    case '+':
        return "char_plus";
    case ',':
        return "char_comma";
    case '-':
        return "char_minus";
    case '.':
        return "char_dot";
    case '/':
        return "char_slash";
    case '0':
        return "char_0";
    case '1':
        return "char_1";
    case '2':
        return "char_2";
    case '3':
        return "char_3";
    case '4':
        return "char_4";
    case '5':
        return "char_5";
    case '6':
        return "char_6";
    case '7':
        return "char_7";
    case '8':
        return "char_8";
    case '9':
        return "char_9";
    case ':':
        return "char_colon";
    case ';':
        return "char_semicolon";
    case '<':
        return "char_less_than";
    case '=':
        return "char_equals";
    case '>':
        return "char_greater_than";
    case '?':
        return "char_question_mark";
    case '@':
        return "char_at";
    case '[':
        return "char_open_square_bracket";
    case '\\':
        return "char_backslash";
    case ']':
        return "char_close_square_bracket";
    case '{':
        return "char_open_curly_brace";
    case '|':
        return "char_pipe";
    case '}':
        return "char_close_curly_brace";
    case '~':
        return "char_tilde";
    default:
        return nullptr;
    }
}

} // namespace canvas
} // namespace engine
