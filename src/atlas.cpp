#include "engine/atlas.h"
#include "engine/file.h"
#include "engine/log.h"
#include "engine/math.inl"
#include "engine/texture.h"

#include <cassert>
#include <cjson/cJSON.h>
#include <collection_types.h>
#include <hash.h>
#include <memory.h>
#include <murmur_hash.h>
#include <string_stream.h>
#include <temp_allocator.h>

#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>

namespace engine {
using namespace foundation;
using namespace foundation::string_stream;

Atlas::Atlas(foundation::Allocator &allocator, const char *atlas_filename)
: allocator(allocator)
, sprite_names(nullptr)
, frames(nullptr)
, texture(nullptr) {
    sprite_names = MAKE_NEW(allocator, Array<Buffer *>, allocator);
    frames = MAKE_NEW(allocator, Hash<AtlasFrame>, allocator);

    string_stream::Buffer data(allocator);

    if (!file::read(data, atlas_filename)) {
        log_fatal("Could not read atlas");
    }

    const cJSON *json = cJSON_Parse(string_stream::c_str(data));
    if (!json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            log_fatal("Could not parse atlas %s:\n%s", atlas_filename, error_ptr);
        } else {
            log_fatal("Could not parse atlas %s: unknown error", atlas_filename);
        }
    }

    // meta
    {
        const cJSON *meta = cJSON_GetObjectItem(json, "meta");
        if (!meta || !cJSON_IsObject(meta)) {
            log_fatal("Could not parse atlas %s: missing \"meta\": {...}", atlas_filename);
        }

        const cJSON *image = cJSON_GetObjectItemCaseSensitive(meta, "image");
        if (!image || !cJSON_IsString(image)) {
            log_fatal("Could not parse atlas %s: meta missing \"image\"", atlas_filename);
        }

        assert(image->valuestring);
        const char *image_filename = image->valuestring;
        texture = MAKE_NEW(allocator, Texture, allocator, image_filename);
    }

    // frames
    {
        const cJSON *frames_item = cJSON_GetObjectItem(json, "frames");
        if (!frames_item || !cJSON_IsArray(frames_item)) {
            log_fatal("Could not parse atlas %s: missing \"frames\": [...]", atlas_filename);
        }

        const cJSON *frame = nullptr;
        cJSON_ArrayForEach(frame, frames_item) {
            cJSON *filename = cJSON_GetObjectItemCaseSensitive(frame, "filename");
            if (!filename || !cJSON_IsString(filename)) {
                log_fatal("Could not parse atlas %s: frame missing \"filename\": ...", atlas_filename);
            }

            cJSON *frame_rect = cJSON_GetObjectItemCaseSensitive(frame, "frame");
            if (!frame_rect || !cJSON_IsObject(frame_rect)) {
                log_fatal("Could not parse atlas %s: frame missing \"frame\": ...", atlas_filename);
            }

            cJSON *x = cJSON_GetObjectItemCaseSensitive(frame_rect, "x");
            if (!x || !cJSON_IsNumber(x)) {
                log_fatal("Could not parse atlas %s: frame missing \"x\": ...", atlas_filename);
            }

            cJSON *y = cJSON_GetObjectItemCaseSensitive(frame_rect, "y");
            if (!y || !cJSON_IsNumber(y)) {
                log_fatal("Could not parse atlas %s: frame missing \"y\": ...", atlas_filename);
            }

            cJSON *w = cJSON_GetObjectItemCaseSensitive(frame_rect, "w");
            if (!w || !cJSON_IsNumber(w)) {
                log_fatal("Could not parse atlas %s: frame missing \"w\": ...", atlas_filename);
            }

            cJSON *h = cJSON_GetObjectItemCaseSensitive(frame_rect, "h");
            if (!h || !cJSON_IsNumber(h)) {
                log_fatal("Could not parse atlas %s: frame missing \"h\": ...", atlas_filename);
            }

            glm::vec2 pv = {0.5f, 0.5f};

            cJSON *pivot = cJSON_GetObjectItemCaseSensitive(frame, "pivot");
            if (pivot && cJSON_IsObject(pivot)) {
                cJSON *pvx = cJSON_GetObjectItemCaseSensitive(pivot, "x");
                if (!pvx || !cJSON_IsNumber(pvx)) {
                    log_fatal("Could not parse atlas %s: frame pivot missing \"x\": ...", atlas_filename);
                }

                cJSON *pvy = cJSON_GetObjectItemCaseSensitive(pivot, "y");
                if (!pvy || !cJSON_IsNumber(pvy)) {
                    log_fatal("Could not parse atlas %s: frame pivot missing \"y\": ...", atlas_filename);
                }

                pv.x = (float)pvx->valuedouble;
                pv.y = (float)pvy->valuedouble;
            }

            math::Rect r;
            r.origin.x = x->valueint;
            r.origin.y = y->valueint;
            r.size.x = w->valueint;
            r.size.y = h->valueint;

            AtlasFrame fr;
            fr.pivot = pv;
            fr.rect = r;

            Buffer *sprite_name = MAKE_NEW(allocator, Buffer, allocator);
            *sprite_name << filename->valuestring;
            array::push_back(*sprite_names, sprite_name);

            uint64_t key = murmur_hash_64(c_str(*sprite_name), array::size(*sprite_name), 0);
            hash::set(*this->frames, key, fr);
        }
    }
}

Atlas::~Atlas() {
    for (uint32_t i = 0; i < array::size(*sprite_names); ++i) {
        Buffer *b = (*sprite_names)[i];
        MAKE_DELETE(allocator, Buffer, b);
    }

    MAKE_DELETE(allocator, Array, sprite_names);
    MAKE_DELETE(allocator, Hash, frames);
    MAKE_DELETE(allocator, Texture, texture);
}

const AtlasFrame *atlas_frame(const Atlas &atlas, const char *sprite_name) {
    if (!sprite_name) {
        return nullptr;
    }

    uint64_t key = murmur_hash_64(sprite_name, (uint32_t)strlen(sprite_name), 0);
    for (auto i = hash::begin(*atlas.frames); i != hash::end(*atlas.frames); ++i) {
        if (i->key == key) {
            return &(i->value);
        }
    }

    return nullptr;
}

const math::Rect *atlas_rect(const Atlas &atlas, const char *sprite_name) {
    const AtlasFrame *frame = atlas_frame(atlas, sprite_name);
    if (!frame) {
        return nullptr;
    }

    return &frame->rect;
}

} // namespace engine
