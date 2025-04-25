#pragma once
#include "platform/input_types.h"

//struct ThreadContext{
//    
//};

struct KeyboardState {
	u8 keys[KEYS_MAX_KEYS];
};

struct MouseState {
	u8 keys[MOUSE_BUTTON_MAX];
};

struct KeyboardInput {
	KeyboardState prev_state;
	KeyboardState curr_state;
};

struct MouseInput {
	MouseState prev_state;
	MouseState curr_state;
};

struct GameInput {
	KeyboardInput keyboard_state;
	MouseInput mouse_state;
};


//#define INPUT_FUNCTION(name) bool name(GameInput* input, Keys key)
//typedef INPUT_FUNCTION(is_key_pressed_fn);
//typedef INPUT_FUNCTION(is_key_just_pressed_fn);

internal u8 is_key_just_pressed(GameInput* input, Keys key) {
    return input->keyboard_state.curr_state.keys[key] == 1 && input->keyboard_state.prev_state.keys[key] == 0;
}

internal u8 is_key_pressed(GameInput* input, Keys key) {
    return input->keyboard_state.curr_state.keys[key] == 1;
}

internal u8 is_key_released(GameInput* input, Keys key) {
    return input->keyboard_state.curr_state.keys[key] == 0;
}

internal u8 was_key_pressed(GameInput* input, Keys key) {
    return input->keyboard_state.prev_state.keys[key] == 1;
}

internal u8 was_key_released(GameInput* input, Keys key) {
    return input->keyboard_state.prev_state.keys[key] == 0;
}

//struct GameOffscreenBuffer {
//	void* memory;
//    BITMAPINFO info;
//    i32 width;
//    i32 height;
//    i32 bytes_per_pixel;
//};
//
//struct GameMemory {
//    b32 is_initialized;
//    void* permanent_storage;
//    void* transient_storage;
//    size_t permanent_storage_size;
//    size_t transient_storage_size;
//    is_key_pressed_fn* is_key_pressed;
//    is_key_just_pressed_fn* is_key_just_pressed;
//};
//
//#define GAME_UPDATE_AND_RENDER(name) void name(ThreadContext* thread_context, GameMemory* game_memory, GameInput* game_input, GameOffscreenBuffer* buffer)
//typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
//
//GAME_UPDATE_AND_RENDER(game_update_and_render_stub){ }