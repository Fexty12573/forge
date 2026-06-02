#include "forge/input.h"

#include "forge/nn/hid.h"

#include <algorithm>
#include <iterator>

namespace {

using namespace nn::hid;

// nn::hid::NpadIdType values.
constexpr u32 kNpadPlayer1 = 0;
constexpr u32 kNpadHandheld = 0x20;

constexpr u64 kMaxTouchPoints = 16;

NpadBaseState s_cur { };
NpadBaseState s_prev { };
bool s_connected = false;

TouchScreenState<kMaxTouchPoints> s_touch { };

KeyboardState s_kbCur { };
KeyboardState s_kbPrev { };

// Every Npad*State derives from NpadBaseState with an identical layout, so we
// read into the concrete type the active style requires, then slice down to
// the shared base. This is what lets one code path cover docked (FullKey),
// handheld, and detached Joy-Con (JoyDual) without caring which it is.
bool readNpad(u32 port, NpadBaseState& out)
{
    const NpadStyleSet style = GetNpadStyleSet(port);

    if (style.Test((int)NpadStyleTag::NpadStyleFullKey)) {
        NpadFullKeyState s { };
        GetNpadState(&s, port);
        out = s;
        return true;
    }
    if (style.Test((int)NpadStyleTag::NpadStyleHandheld)) {
        NpadHandheldState s { };
        GetNpadState(&s, port);
        out = s;
        return true;
    }
    if (style.Test((int)NpadStyleTag::NpadStyleJoyDual)) {
        NpadJoyDualState s { };
        GetNpadState(&s, port);
        out = s;
        return true;
    }

    return false;
}

float normalizeStick(s32 value)
{
    return std::clamp(value / 32767.0f, -1.0f, 1.0f);
}

} // namespace

extern "C" void forge_input_init(void)
{
    InitializeNpad();

    const u32 supportedIds[] = { kNpadHandheld, kNpadPlayer1 };
    SetSupportedNpadIdType(supportedIds, std::size(supportedIds));

    NpadStyleSet styles { };
    styles.Set((int)NpadStyleTag::NpadStyleFullKey);
    styles.Set((int)NpadStyleTag::NpadStyleHandheld);
    styles.Set((int)NpadStyleTag::NpadStyleJoyDual);
    SetSupportedNpadStyleSet(styles);

    InitializeTouchScreen();
    InitializeKeyboard();
}

extern "C" void forge_input_update(void)
{
    s_prev = s_cur;

    NpadBaseState state { };
    // Prefer handheld, fall back to player 1 (docked / Pro controller).
    s_connected = readNpad(kNpadHandheld, state) || readNpad(kNpadPlayer1, state);
    s_cur = s_connected ? state : NpadBaseState { };

    GetTouchScreenState(&s_touch);

    s_kbPrev = s_kbCur;
    GetKeyboardState(&s_kbCur);
}

extern "C" bool forge_input_isDown(ForgeButton button)
{
    return s_cur.mButtons.Test((int)button);
}

extern "C" bool forge_input_isPressed(ForgeButton button)
{
    return s_cur.mButtons.Test((int)button) && !s_prev.mButtons.Test((int)button);
}

extern "C" bool forge_input_isReleased(ForgeButton button)
{
    return !s_cur.mButtons.Test((int)button) && s_prev.mButtons.Test((int)button);
}

extern "C" void forge_input_getStickL(float* out_x, float* out_y)
{
    if (out_x) {
        *out_x = normalizeStick(s_cur.mAnalogStickL.mX);
    }
    if (out_y) {
        *out_y = normalizeStick(s_cur.mAnalogStickL.mY);
    }
}

extern "C" void forge_input_getStickR(float* out_x, float* out_y)
{
    if (out_x) {
        *out_x = normalizeStick(s_cur.mAnalogStickR.mX);
    }
    if (out_y) {
        *out_y = normalizeStick(s_cur.mAnalogStickR.mY);
    }
}

extern "C" bool forge_input_isConnected(void)
{
    return s_connected;
}

extern "C" bool forge_input_getTouch(float* out_x, float* out_y)
{
    if (s_touch.mCount <= 0) {
        return false;
    }

    if (out_x) {
        *out_x = static_cast<float>(s_touch.mTouches[0].mX);
    }
    if (out_y) {
        *out_y = static_cast<float>(s_touch.mTouches[0].mY);
    }
    return true;
}

extern "C" bool forge_input_isKeyDown(ForgeKey key)
{
    return s_kbCur.mKeys.Test((int)key);
}

extern "C" bool forge_input_isKeyPressed(ForgeKey key)
{
    return s_kbCur.mKeys.Test((int)key) && !s_kbPrev.mKeys.Test((int)key);
}

extern "C" bool forge_input_isKeyReleased(ForgeKey key)
{
    return !s_kbCur.mKeys.Test((int)key) && s_kbPrev.mKeys.Test((int)key);
}

extern "C" bool forge_input_isShiftDown(void)
{
    return s_kbCur.mModifiers.Test((int)KeyboardModifier::Shift);
}

extern "C" bool forge_input_isCtrlDown(void)
{
    return s_kbCur.mModifiers.Test((int)KeyboardModifier::Control);
}

extern "C" bool forge_input_isAltDown(void)
{
    return s_kbCur.mModifiers.Test((int)KeyboardModifier::LeftAlt)
        || s_kbCur.mModifiers.Test((int)KeyboardModifier::RightAlt);
}
