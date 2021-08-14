#include "engine/tilesheet.h"
#include "engine/engine.h"
#include "engine/shader.h"
#include "engine/texture.h"
#include "engine/log.h"
#include "engine/config.inl"

#include <array.h>
#include <proto/engine.pb.h>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace {

const char *vertex_source = R"(
#version 440 core

uniform mat4 projection;
uniform mat4 model;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_texture_coords;

smooth out vec2 uv;
smooth out vec4 color;

void main() {
   mat4 mvp = projection * model;
   gl_Position = mvp * vec4(in_position, 1.0);
   uv = in_texture_coords;
   color = vec4(in_color, 1.0);
}
)";

const char *fragment_source = R"(
#version 440 core

uniform sampler2D texture0;
in vec2 uv;
in vec4 color;

out vec4 out_color;

void main() {
   out_color = color * texture(texture0, uv);
}
)";

}

namespace engine {

Tilesheet::Tilesheet(Allocator &allocator, const char *params_path)
: allocator(allocator)
, params(nullptr)
, tilesheet_shader(nullptr)
, tilesheet_vbo(0)
, tilesheet_vao(0)
, tilesheet_ebo(0)
, tilesheet_atlas(nullptr)
, tile_size(0)
, tiles_width(0)
, tiles_height(0)
, tiles(nullptr) {
    params = MAKE_NEW(allocator, TilesheetParams);
    config::read(params_path, params);

    tile_size = params->tile_size();

    tilesheet_shader = MAKE_NEW(allocator, engine::Shader, nullptr, vertex_source, fragment_source);
    tilesheet_atlas = MAKE_NEW(allocator, engine::Texture, allocator, params->atlas_filename().c_str());
    tiles = MAKE_NEW(allocator, Array<Tile>, allocator);
}

Tilesheet::~Tilesheet() {
    if (tilesheet_shader) {
        MAKE_DELETE(allocator, Shader, tilesheet_shader);
    }

    if (tilesheet_vbo) {
        glDeleteBuffers(1, &tilesheet_vbo);
    }

    if (tilesheet_vao) {
        glDeleteVertexArrays(1, &tilesheet_vao);
    }

    if (tilesheet_atlas) {
        MAKE_DELETE(allocator, Texture, tilesheet_atlas);
    }

    if (tilesheet_ebo) {
        glDeleteBuffers(1, &tilesheet_ebo);
    }

    if (tiles) {
        MAKE_DELETE(allocator, Array, tiles);
    }

    if (params) {
        MAKE_DELETE(allocator, TilesheetParams, params);
    }
}

void init_tilesheet(Tilesheet &tilesheet, uint32_t tiles_width, uint32_t tiles_height) {
    tilesheet.tiles_width = tiles_width;
    tilesheet.tiles_height = tiles_height;

    array::resize(*tilesheet.tiles, tilesheet.tiles_width * tilesheet.tiles_height);
    for (Tile *it = array::begin(*tilesheet.tiles); it != array::end(*tilesheet.tiles); ++it) {
        *it = Tile();
    }
}

void commit_tilesheet(Tilesheet &tilesheet) {
    if (tilesheet.tilesheet_vao) {
        glDeleteBuffers(1, &tilesheet.tilesheet_vao);
        tilesheet.tilesheet_vao = 0;
    }

    if (tilesheet.tilesheet_ebo) {
        glDeleteBuffers(1, &tilesheet.tilesheet_ebo);
        tilesheet.tilesheet_ebo = 0;
    }

    uint32_t tiles_width = tilesheet.tiles_width;
    Array<Tile> &tiles = *tilesheet.tiles;

    Allocator &allocator = tilesheet.allocator;
    size_t tiles_count = array::size(tiles);

    size_t vertex_count = 4 * tiles_count;
    Vertex *vertex_data = (Vertex *)allocator.allocate(sizeof(Vertex) * vertex_count);

    size_t index_count = 6 * tiles_count;
    GLuint *index_data = (GLuint *)allocator.allocate(sizeof(GLuint) * index_count);

    for (uint32_t i = 0; i < tiles_count; ++i) {
        Tile &tile = tiles[i];
        uint32_t x, y;
        coord(i, x, y, tiles_width);

        // position
        {
            vertex_data[i * 4 + 0].position = {(float)x,     (float)y,     0.0f};
            vertex_data[i * 4 + 1].position = {(float)x + 1, (float)y + 1, 0.0f};
            vertex_data[i * 4 + 2].position = {(float)x,     (float)y + 1, 0.0f};
            vertex_data[i * 4 + 3].position = {(float)x + 1, (float)y,     0.0f};
        }

        // color
        {
            vertex_data[i * 4 + 0].color = tile.color;
            vertex_data[i * 4 + 1].color = tile.color;
            vertex_data[i * 4 + 2].color = tile.color;
            vertex_data[i * 4 + 3].color = tile.color;
        }

        // texture_coords
        {
            uint32_t texcoord_x, texcoord_y;
            const uint32_t tilesheet_width_tiles = tilesheet.params->tiles_width();
            coord(tile.tile, texcoord_x, texcoord_y, tilesheet_width_tiles);

            texcoord_x = texcoord_x * (tilesheet.params->tile_size() + tilesheet.params->atlas_gutter());
            texcoord_y = tilesheet.tilesheet_atlas->height - (texcoord_y + 1) * (tilesheet.params->tile_size() + tilesheet.params->atlas_gutter());

            float cell_x = (float)texcoord_x / tilesheet.tilesheet_atlas->width;
            float cell_y = (float)texcoord_y / tilesheet.tilesheet_atlas->height;
            float cell_width = (float)tilesheet.tile_size / tilesheet.tilesheet_atlas->width;
            float cell_height = (float)tilesheet.tile_size / tilesheet.tilesheet_atlas->height;

            vertex_data[i * 4 + 0].texture_coords = {cell_x, cell_y};
            vertex_data[i * 4 + 1].texture_coords = {cell_x + cell_width, cell_y + cell_height};
            vertex_data[i * 4 + 2].texture_coords = {cell_x, cell_y + cell_height};
            vertex_data[i * 4 + 3].texture_coords = {cell_x + cell_width, cell_y};

            index_data[i * 6 + 0] = i * 4 + 0;
            index_data[i * 6 + 1] = i * 4 + 1;
            index_data[i * 6 + 2] = i * 4 + 2;

            index_data[i * 6 + 3] = i * 4 + 0;
            index_data[i * 6 + 4] = i * 4 + 3;
            index_data[i * 6 + 5] = i * 4 + 1;
        }
    }
 
    glGenVertexArrays(1, &tilesheet.tilesheet_vao);
    glBindVertexArray(tilesheet.tilesheet_vao);

    glGenBuffers(1, &tilesheet.tilesheet_vbo);
    glGenBuffers(1, &tilesheet.tilesheet_ebo);

    glBindBuffer(GL_ARRAY_BUFFER, tilesheet.tilesheet_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertex_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)0);

    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, color));

    // texture_coords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid *)offsetof(Vertex, texture_coords));

    allocator.deallocate(vertex_data);

    // Element index array
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tilesheet.tilesheet_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLuint), index_data, GL_STATIC_DRAW);

    allocator.deallocate(index_data);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
}

void render_tilesheet(Engine &engine, Tilesheet &tilesheet) {
    if (!(tilesheet.tilesheet_shader && tilesheet.tilesheet_shader->program && tilesheet.tilesheet_vao && tilesheet.tilesheet_ebo)) {
        return;
    }

    const uint32_t render_scale = engine.params->render_scale();

    const glm::mat4 projection = glm::ortho(0.0f, (float)engine.window_rect.size.x, 0.0f, (float)engine.window_rect.size.y);
    glm::mat4 view = glm::mat4(1);
    view = glm::translate(view, glm::vec3(engine.camera_offset.x, engine.camera_offset.y, 0.0f));

    const GLuint shader_program = tilesheet.tilesheet_shader->program;

    glUseProgram(shader_program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tilesheet.tilesheet_atlas->texture);
    glUniform1i(glGetUniformLocation(shader_program, "texture0"), 0);

    glm::mat4 model = glm::mat4(1);
    model = glm::scale(model, glm::vec3(tilesheet.tile_size * render_scale, tilesheet.tile_size * render_scale, 1));

    glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, glm::value_ptr(projection * view));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(tilesheet.tilesheet_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tilesheet.tilesheet_ebo);

    glDrawElements(GL_TRIANGLES, 6 * tilesheet.tiles_width * tilesheet.tiles_height, GL_UNSIGNED_INT, (void *)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

} // namespace engine
