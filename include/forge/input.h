#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ForgeButton {
    ForgeButton_A = 0,
    ForgeButton_B = 1,
    ForgeButton_X = 2,
    ForgeButton_Y = 3,
    ForgeButton_StickL = 4, // left stick pressed in
    ForgeButton_StickR = 5, // right stick pressed in
    ForgeButton_L = 6,
    ForgeButton_R = 7,
    ForgeButton_ZL = 8,
    ForgeButton_ZR = 9,
    ForgeButton_Plus = 10,
    ForgeButton_Minus = 11,
    ForgeButton_Left = 12,
    ForgeButton_Up = 13,
    ForgeButton_Right = 14,
    ForgeButton_Down = 15,
} ForgeButton;

typedef enum ForgeKey {
    ForgeKey_A = 4,
    ForgeKey_B,
    ForgeKey_C,
    ForgeKey_D,
    ForgeKey_E,
    ForgeKey_F,
    ForgeKey_G,
    ForgeKey_H,
    ForgeKey_I,
    ForgeKey_J,
    ForgeKey_K,
    ForgeKey_L,
    ForgeKey_M,
    ForgeKey_N,
    ForgeKey_O,
    ForgeKey_P,
    ForgeKey_Q,
    ForgeKey_R,
    ForgeKey_S,
    ForgeKey_T,
    ForgeKey_U,
    ForgeKey_V,
    ForgeKey_W,
    ForgeKey_X,
    ForgeKey_Y,
    ForgeKey_Z,
    ForgeKey_1 = 30,
    ForgeKey_2,
    ForgeKey_3,
    ForgeKey_4,
    ForgeKey_5,
    ForgeKey_6,
    ForgeKey_7,
    ForgeKey_8,
    ForgeKey_9,
    ForgeKey_0,
    ForgeKey_Enter = 40,
    ForgeKey_Escape = 41,
    ForgeKey_Backspace = 42,
    ForgeKey_Tab = 43,
    ForgeKey_Space = 44,
    ForgeKey_Minus = 45,
    ForgeKey_Equals = 46,
    ForgeKey_LeftBracket = 47,
    ForgeKey_RightBracket = 48,
    ForgeKey_Backslash = 49,
    ForgeKey_Semicolon = 51,
    ForgeKey_Apostrophe = 52,
    ForgeKey_Grave = 53,
    ForgeKey_Comma = 54,
    ForgeKey_Period = 55,
    ForgeKey_Slash = 56,
    ForgeKey_CapsLock = 57,
    ForgeKey_F1 = 58,
    ForgeKey_F2,
    ForgeKey_F3,
    ForgeKey_F4,
    ForgeKey_F5,
    ForgeKey_F6,
    ForgeKey_F7,
    ForgeKey_F8,
    ForgeKey_F9,
    ForgeKey_F10,
    ForgeKey_F11,
    ForgeKey_F12,
    ForgeKey_Insert = 73,
    ForgeKey_Home = 74,
    ForgeKey_PageUp = 75,
    ForgeKey_Delete = 76,
    ForgeKey_End = 77,
    ForgeKey_PageDown = 78,
    ForgeKey_Right = 79,
    ForgeKey_Left = 80,
    ForgeKey_Down = 81,
    ForgeKey_Up = 82,
} ForgeKey;

void forge_input_init(void);
void forge_input_update(void);

bool forge_input_isDown(ForgeButton button); // held this frame
bool forge_input_isPressed(ForgeButton button); // went down this frame
bool forge_input_isReleased(ForgeButton button); // went up this frame

// Analog sticks, normalized to [-1, 1]. Up/right are positive.
void forge_input_getStickL(float* out_x, float* out_y);
void forge_input_getStickR(float* out_x, float* out_y);

// True if a supported controller is currently connected.
bool forge_input_isConnected(void);

// True if the screen is being touched, out_x/y will hold touched coordinates in pixels
bool forge_input_getTouch(float* out_x, float* out_y);

bool forge_input_isKeyDown(ForgeKey key);
bool forge_input_isKeyPressed(ForgeKey key);
bool forge_input_isKeyReleased(ForgeKey key);
bool forge_input_isShiftDown(void);
bool forge_input_isCtrlDown(void);
bool forge_input_isAltDown(void);

#ifdef __cplusplus
}
#endif
