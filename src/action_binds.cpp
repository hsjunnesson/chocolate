#include "engine/action_binds.h"
#include "engine/log.h"
#include "engine/file.h"
#include "engine/config.h"
#include "engine/ini.h"

#pragma warning(push, 0)
#include <array.h>
#include <hash.h>
#include <memory.h>
#include <temp_allocator.h>
#include <string_stream.h>
#include <murmur_hash.h>

#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>
#pragma warning(pop)

namespace engine {
using namespace foundation;
using namespace foundation::string_stream;

ActionBindsBind bind_from_descriptor(const char *);
const char *bind_descriptor(const ActionBindsBind);

ActionBinds::ActionBinds(foundation::Allocator &allocator, const char *config_path)
: allocator(allocator)
, actions(allocator)
, action_binds(allocator) {
    TempAllocator1024 ta;

    // Parse config file
    {
        string_stream::Buffer buffer(ta);
        if (!file::read(buffer, config_path)) {
            log_fatal("Could not open config file %s", config_path);
        }

        ini_t *ini = ini_load(string_stream::c_str(buffer), nullptr);
        
        if (!ini) {
            log_fatal("Could not parse config file %s", config_path);
        }

        int section = ini_find_section(ini, "actionbinds", 0);
        if (section == INI_NOT_FOUND) {
            log_error("Config file %s missing [actionbinds]");
        } else {
            for (int property = 0; property < ini_property_count(ini, section); ++property) {
                const char *name = ini_property_name(ini, section, property);
                const char *value = ini_property_value(ini, section, property);
                size_t value_len = strlen(value);

                if (name && value) {
                    {
                        Buffer *action = MAKE_NEW(allocator, Buffer, allocator);
                        *action << name;
                        array::push_back(actions, action);
                    }

                    uint64_t actions_index = array::size(actions) - 1;

                    char *buf = (char *)ta.allocate((uint32_t)value_len + 1);
                    strncpy_s(buf, value_len + 1, value, value_len);

                    char *context = nullptr;
                    buf = strtok_s(buf, ",", &context);

                    while (buf) {
                        ActionBindsBind bind = bind_from_descriptor(buf);
                        if (bind == ActionBindsBind::NOT_FOUND) {
                            log_fatal("Invalid actionbinds %s = %s", name, buf);
                        } else {
                            multi_hash::insert(action_binds, actions_index, bind);
                        }

                        buf = strtok_s(NULL, ",", &context);
                    }
                }
            }
        }

        ini_destroy(ini);
    }

    // Validate actionbinds
    {
        Hash<bool> found_actions(ta);

        for (uint32_t i = 0; i < array::size(actions); ++i) {
            Buffer *b = actions[i];
            uint64_t key = murmur_hash_64(array::begin(*b), array::size(*b), 0);
            if (hash::has(found_actions, key)) {
                log_fatal("Invalid actionbinds, multiple actions %s", c_str(*b));
            }

            hash::set(found_actions, key, true);
        }
    }
}

ActionBinds::~ActionBinds() {
    for (uint32_t i = 0; i < array::size(actions); ++i) {
        Buffer *b = actions[i];
        MAKE_DELETE(allocator, Buffer, b);
    }
}

/// Returns the Bind from a descriptor. or NOT_FOUND if not a valid descriptor.
ActionBindsBind bind_from_descriptor(const char *descriptor) {
    for (int i = static_cast<int>(ActionBindsBind::FIRST); i != static_cast<int>(ActionBindsBind::LAST); ++i) {
        ActionBindsBind bind = ActionBindsBind(i);
        const char *d = bind_descriptor(bind);
        if (d && strcmp(d, descriptor) == 0) {
            return bind;
        }
    }

    return ActionBindsBind::NOT_FOUND;
}

const char *bind_descriptor(const ActionBindsBind bind) {
    switch (bind) {
    case ActionBindsBind::GAMEPAD_BUTTON_A:
        return "GAMEPAD_BUTTON_A";
    case ActionBindsBind::GAMEPAD_BUTTON_B:
        return "GAMEPAD_BUTTON_B";
    case ActionBindsBind::GAMEPAD_BUTTON_X:
        return "GAMEPAD_BUTTON_X";
    case ActionBindsBind::GAMEPAD_BUTTON_Y:
        return "GAMEPAD_BUTTON_Y";
    case ActionBindsBind::GAMEPAD_BUTTON_LEFT_BUMPER:
        return "GAMEPAD_BUTTON_LEFT_BUMPER";
    case ActionBindsBind::GAMEPAD_BUTTON_RIGHT_BUMPER:
        return "GAMEPAD_BUTTON_RIGHT_BUMPER";
    case ActionBindsBind::GAMEPAD_BUTTON_BACK:
        return "GAMEPAD_BUTTON_BACK";
    case ActionBindsBind::GAMEPAD_BUTTON_START:
        return "GAMEPAD_BUTTON_START";
    case ActionBindsBind::GAMEPAD_BUTTON_GUIDE:
        return "GAMEPAD_BUTTON_GUIDE";
    case ActionBindsBind::GAMEPAD_BUTTON_LEFT_THUMB:
        return "GAMEPAD_BUTTON_LEFT_THUMB";
    case ActionBindsBind::GAMEPAD_BUTTON_RIGHT_THUMB:
        return "GAMEPAD_BUTTON_RIGHT_THUMB";
    case ActionBindsBind::GAMEPAD_BUTTON_DPAD_UP:
        return "GAMEPAD_BUTTON_DPAD_UP";
    case ActionBindsBind::GAMEPAD_BUTTON_DPAD_RIGHT:
        return "GAMEPAD_BUTTON_DPAD_RIGHT";
    case ActionBindsBind::GAMEPAD_BUTTON_DPAD_DOWN:
        return "GAMEPAD_BUTTON_DPAD_DOWN";
    case ActionBindsBind::GAMEPAD_BUTTON_DPAD_LEFT:
        return "GAMEPAD_BUTTON_DPAD_LEFT";
    case ActionBindsBind::GAMEPAD_BUTTON_CROSS:
        return "GAMEPAD_BUTTON_CROSS";
    case ActionBindsBind::GAMEPAD_BUTTON_CIRCLE:
        return "GAMEPAD_BUTTON_CIRCLE";
    case ActionBindsBind::GAMEPAD_BUTTON_SQUARE:
        return "GAMEPAD_BUTTON_SQUARE";
    case ActionBindsBind::GAMEPAD_BUTTON_TRIANGLE:
        return "GAMEPAD_BUTTON_TRIANGLE";
    case ActionBindsBind::GAMEPAD_AXIS_LEFT_X:
        return "GAMEPAD_AXIS_LEFT_X";
    case ActionBindsBind::GAMEPAD_AXIS_LEFT_Y:
        return "GAMEPAD_AXIS_LEFT_Y";
    case ActionBindsBind::GAMEPAD_AXIS_RIGHT_X:
        return "GAMEPAD_AXIS_RIGHT_X";
    case ActionBindsBind::GAMEPAD_AXIS_RIGHT_Y:
        return "GAMEPAD_AXIS_RIGHT_Y";
    case ActionBindsBind::GAMEPAD_AXIS_LEFT_TRIGGER:
        return "GAMEPAD_AXIS_LEFT_TRIGGER";
    case ActionBindsBind::GAMEPAD_AXIS_RIGHT_TRIGGER:
        return "GAMEPAD_AXIS_RIGHT_TRIGGER";
    case ActionBindsBind::KEY_SPACE:
        return "KEY_SPACE";
    case ActionBindsBind::KEY_APOSTROPHE:
        return "KEY_APOSTROPHE";
    case ActionBindsBind::KEY_COMMA:
        return "KEY_COMMA";
    case ActionBindsBind::KEY_MINUS:
        return "KEY_MINUS";
    case ActionBindsBind::KEY_PERIOD:
        return "KEY_PERIOD";
    case ActionBindsBind::KEY_SLASH:
        return "KEY_SLASH";
    case ActionBindsBind::KEY_0:
        return "KEY_0";
    case ActionBindsBind::KEY_1:
        return "KEY_1";
    case ActionBindsBind::KEY_2:
        return "KEY_2";
    case ActionBindsBind::KEY_3:
        return "KEY_3";
    case ActionBindsBind::KEY_4:
        return "KEY_4";
    case ActionBindsBind::KEY_5:
        return "KEY_5";
    case ActionBindsBind::KEY_6:
        return "KEY_6";
    case ActionBindsBind::KEY_7:
        return "KEY_7";
    case ActionBindsBind::KEY_8:
        return "KEY_8";
    case ActionBindsBind::KEY_9:
        return "KEY_9";
    case ActionBindsBind::KEY_SEMICOLON:
        return "KEY_SEMICOLON";
    case ActionBindsBind::KEY_EQUAL:
        return "KEY_EQUAL";
    case ActionBindsBind::KEY_A:
        return "KEY_A";
    case ActionBindsBind::KEY_B:
        return "KEY_B";
    case ActionBindsBind::KEY_C:
        return "KEY_C";
    case ActionBindsBind::KEY_D:
        return "KEY_D";
    case ActionBindsBind::KEY_E:
        return "KEY_E";
    case ActionBindsBind::KEY_F:
        return "KEY_F";
    case ActionBindsBind::KEY_G:
        return "KEY_G";
    case ActionBindsBind::KEY_H:
        return "KEY_H";
    case ActionBindsBind::KEY_I:
        return "KEY_I";
    case ActionBindsBind::KEY_J:
        return "KEY_J";
    case ActionBindsBind::KEY_K:
        return "KEY_K";
    case ActionBindsBind::KEY_L:
        return "KEY_L";
    case ActionBindsBind::KEY_M:
        return "KEY_M";
    case ActionBindsBind::KEY_N:
        return "KEY_N";
    case ActionBindsBind::KEY_O:
        return "KEY_O";
    case ActionBindsBind::KEY_P:
        return "KEY_P";
    case ActionBindsBind::KEY_Q:
        return "KEY_Q";
    case ActionBindsBind::KEY_R:
        return "KEY_R";
    case ActionBindsBind::KEY_S:
        return "KEY_S";
    case ActionBindsBind::KEY_T:
        return "KEY_T";
    case ActionBindsBind::KEY_U:
        return "KEY_U";
    case ActionBindsBind::KEY_V:
        return "KEY_V";
    case ActionBindsBind::KEY_W:
        return "KEY_W";
    case ActionBindsBind::KEY_X:
        return "KEY_X";
    case ActionBindsBind::KEY_Y:
        return "KEY_Y";
    case ActionBindsBind::KEY_Z:
        return "KEY_Z";
    case ActionBindsBind::KEY_LEFT_BRACKET:
        return "KEY_LEFT_BRACKET";
    case ActionBindsBind::KEY_BACKSLASH:
        return "KEY_BACKSLASH";
    case ActionBindsBind::KEY_RIGHT_BRACKET:
        return "KEY_RIGHT_BRACKET";
    case ActionBindsBind::KEY_GRAVE_ACCENT:
        return "KEY_GRAVE_ACCENT";
    case ActionBindsBind::KEY_WORLD_1:
        return "KEY_WORLD_1";
    case ActionBindsBind::KEY_WORLD_2:
        return "KEY_WORLD_2";
    case ActionBindsBind::KEY_ESCAPE:
        return "KEY_ESCAPE";
    case ActionBindsBind::KEY_ENTER:
        return "KEY_ENTER";
    case ActionBindsBind::KEY_TAB:
        return "KEY_TAB";
    case ActionBindsBind::KEY_BACKSPACE:
        return "KEY_BACKSPACE";
    case ActionBindsBind::KEY_INSERT:
        return "KEY_INSERT";
    case ActionBindsBind::KEY_DELETE:
        return "KEY_DELETE";
    case ActionBindsBind::KEY_RIGHT:
        return "KEY_RIGHT";
    case ActionBindsBind::KEY_LEFT:
        return "KEY_LEFT";
    case ActionBindsBind::KEY_DOWN:
        return "KEY_DOWN";
    case ActionBindsBind::KEY_UP:
        return "KEY_UP";
    case ActionBindsBind::KEY_PAGE_UP:
        return "KEY_PAGE_UP";
    case ActionBindsBind::KEY_PAGE_DOWN:
        return "KEY_PAGE_DOWN";
    case ActionBindsBind::KEY_HOME:
        return "KEY_HOME";
    case ActionBindsBind::KEY_END:
        return "KEY_END";
    case ActionBindsBind::KEY_CAPS_LOCK:
        return "KEY_CAPS_LOCK";
    case ActionBindsBind::KEY_SCROLL_LOCK:
        return "KEY_SCROLL_LOCK";
    case ActionBindsBind::KEY_NUM_LOCK:
        return "KEY_NUM_LOCK";
    case ActionBindsBind::KEY_PRINT_SCREEN:
        return "KEY_PRINT_SCREEN";
    case ActionBindsBind::KEY_PAUSE:
        return "KEY_PAUSE";
    case ActionBindsBind::KEY_F1:
        return "KEY_F1";
    case ActionBindsBind::KEY_F2:
        return "KEY_F2";
    case ActionBindsBind::KEY_F3:
        return "KEY_F3";
    case ActionBindsBind::KEY_F4:
        return "KEY_F4";
    case ActionBindsBind::KEY_F5:
        return "KEY_F5";
    case ActionBindsBind::KEY_F6:
        return "KEY_F6";
    case ActionBindsBind::KEY_F7:
        return "KEY_F7";
    case ActionBindsBind::KEY_F8:
        return "KEY_F8";
    case ActionBindsBind::KEY_F9:
        return "KEY_F9";
    case ActionBindsBind::KEY_F10:
        return "KEY_F10";
    case ActionBindsBind::KEY_F11:
        return "KEY_F11";
    case ActionBindsBind::KEY_F12:
        return "KEY_F12";
    case ActionBindsBind::KEY_F13:
        return "KEY_F13";
    case ActionBindsBind::KEY_F14:
        return "KEY_F14";
    case ActionBindsBind::KEY_F15:
        return "KEY_F15";
    case ActionBindsBind::KEY_F16:
        return "KEY_F16";
    case ActionBindsBind::KEY_F17:
        return "KEY_F17";
    case ActionBindsBind::KEY_F18:
        return "KEY_F18";
    case ActionBindsBind::KEY_F19:
        return "KEY_F19";
    case ActionBindsBind::KEY_F20:
        return "KEY_F20";
    case ActionBindsBind::KEY_F21:
        return "KEY_F21";
    case ActionBindsBind::KEY_F22:
        return "KEY_F22";
    case ActionBindsBind::KEY_F23:
        return "KEY_F23";
    case ActionBindsBind::KEY_F24:
        return "KEY_F24";
    case ActionBindsBind::KEY_F25:
        return "KEY_F25";
    case ActionBindsBind::KEY_KP_0:
        return "KEY_KP_0";
    case ActionBindsBind::KEY_KP_1:
        return "KEY_KP_1";
    case ActionBindsBind::KEY_KP_2:
        return "KEY_KP_2";
    case ActionBindsBind::KEY_KP_3:
        return "KEY_KP_3";
    case ActionBindsBind::KEY_KP_4:
        return "KEY_KP_4";
    case ActionBindsBind::KEY_KP_5:
        return "KEY_KP_5";
    case ActionBindsBind::KEY_KP_6:
        return "KEY_KP_6";
    case ActionBindsBind::KEY_KP_7:
        return "KEY_KP_7";
    case ActionBindsBind::KEY_KP_8:
        return "KEY_KP_8";
    case ActionBindsBind::KEY_KP_9:
        return "KEY_KP_9";
    case ActionBindsBind::KEY_KP_DECIMAL:
        return "KEY_KP_DECIMAL";
    case ActionBindsBind::KEY_KP_DIVIDE:
        return "KEY_KP_DIVIDE";
    case ActionBindsBind::KEY_KP_MULTIPLY:
        return "KEY_KP_MULTIPLY";
    case ActionBindsBind::KEY_KP_SUBTRACT:
        return "KEY_KP_SUBTRACT";
    case ActionBindsBind::KEY_KP_ADD:
        return "KEY_KP_ADD";
    case ActionBindsBind::KEY_KP_ENTER:
        return "KEY_KP_ENTER";
    case ActionBindsBind::KEY_KP_EQUAL:
        return "KEY_KP_EQUAL";
    case ActionBindsBind::KEY_LEFT_SHIFT:
        return "KEY_LEFT_SHIFT";
    case ActionBindsBind::KEY_LEFT_CONTROL:
        return "KEY_LEFT_CONTROL";
    case ActionBindsBind::KEY_LEFT_ALT:
        return "KEY_LEFT_ALT";
    case ActionBindsBind::KEY_LEFT_SUPER:
        return "KEY_LEFT_SUPER";
    case ActionBindsBind::KEY_RIGHT_SHIFT:
        return "KEY_RIGHT_SHIFT";
    case ActionBindsBind::KEY_RIGHT_CONTROL:
        return "KEY_RIGHT_CONTROL";
    case ActionBindsBind::KEY_RIGHT_ALT:
        return "KEY_RIGHT_ALT";
    case ActionBindsBind::KEY_RIGHT_SUPER:
        return "KEY_RIGHT_SUPER";
    case ActionBindsBind::KEY_MENU:
        return "KEY_MENU";
    case ActionBindsBind::MOUSE_BUTTON_1:
        return "MOUSE_BUTTON_1";
    case ActionBindsBind::MOUSE_BUTTON_2:
        return "MOUSE_BUTTON_2";
    case ActionBindsBind::MOUSE_BUTTON_3:
        return "MOUSE_BUTTON_3";
    case ActionBindsBind::MOUSE_BUTTON_4:
        return "MOUSE_BUTTON_4";
    case ActionBindsBind::MOUSE_BUTTON_5:
        return "MOUSE_BUTTON_5";
    case ActionBindsBind::MOUSE_BUTTON_6:
        return "MOUSE_BUTTON_6";
    case ActionBindsBind::MOUSE_BUTTON_7:
        return "MOUSE_BUTTON_7";
    case ActionBindsBind::MOUSE_BUTTON_8:
        return "MOUSE_BUTTON_8";
    case ActionBindsBind::MOUSE_BUTTON_LEFT:
        return "MOUSE_BUTTON_LEFT";
    case ActionBindsBind::MOUSE_BUTTON_RIGHT:
        return "MOUSE_BUTTON_RIGHT";
    case ActionBindsBind::MOUSE_BUTTON_MIDDLE:
        return "MOUSE_BUTTON_MIDDLE";
    case ActionBindsBind::JOYSTICK_1:
        return "JOYSTICK_1";
    case ActionBindsBind::JOYSTICK_2:
        return "JOYSTICK_2";
    case ActionBindsBind::JOYSTICK_3:
        return "JOYSTICK_3";
    case ActionBindsBind::JOYSTICK_4:
        return "JOYSTICK_4";
    case ActionBindsBind::JOYSTICK_5:
        return "JOYSTICK_5";
    case ActionBindsBind::JOYSTICK_6:
        return "JOYSTICK_6";
    case ActionBindsBind::JOYSTICK_7:
        return "JOYSTICK_7";
    case ActionBindsBind::JOYSTICK_8:
        return "JOYSTICK_8";
    case ActionBindsBind::JOYSTICK_9:
        return "JOYSTICK_9";
    case ActionBindsBind::JOYSTICK_10:
        return "JOYSTICK_10";
    case ActionBindsBind::JOYSTICK_11:
        return "JOYSTICK_11";
    case ActionBindsBind::JOYSTICK_12:
        return "JOYSTICK_12";
    case ActionBindsBind::JOYSTICK_13:
        return "JOYSTICK_13";
    case ActionBindsBind::JOYSTICK_14:
        return "JOYSTICK_14";
    case ActionBindsBind::JOYSTICK_15:
        return "JOYSTICK_15";
    case ActionBindsBind::JOYSTICK_16:
        return "JOYSTICK_16";
    default:
        return nullptr;
    }
}

} // namespace engine
