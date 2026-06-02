#pragma once

// Minimal nn::hid declarations for Forge.
//
// We deliberately avoid nnheaders' <nn/hid.h>: it contains a static_assert on
// the internal HID shared-memory layout (detail::SharedMemoryFormat) that is
// tuned for AArch64, and the sub-struct sizes don't sum to 0x40000 on MHGU's
// AArch32 build, so it fails to compile. None of that internal layout is
// needed to call the public Npad API, so we declare only the subset we use.
//
// Reusing the real nn::util::BitFlagSet template (which compiles fine) keeps
// the symbol mangling byte-identical to the game's statically-linked nnSdk, so
// these prototypes resolve against it at load time.

#include <nn/util/util_BitFlagSet.h> // pulls <nn/types.h> for u32/s32/u64

namespace nn::hid {

enum class NpadButton {
    A = 0,
    B = 1,
    X = 2,
    Y = 3,
    StickL = 4,
    StickR = 5,
    L = 6,
    R = 7,
    ZL = 8,
    ZR = 9,
    Plus = 10,
    Minus = 11,
    Left = 12,
    Up = 13,
    Right = 14,
    Down = 15,
};

enum class NpadAttribute {
    IsConnected = 0,
    IsWired = 1,
    IsLeftConnected = 2,
    IsLeftWired = 3,
    IsRightConnected = 4,
    IsRightWired = 5,
};

enum class NpadStyleTag {
    NpadStyleFullKey = 0,
    NpadStyleHandheld = 1,
    NpadStyleJoyDual = 2,
    NpadStyleJoyLeft = 3,
    NpadStyleJoyRight = 4,
};

typedef nn::util::BitFlagSet<32, NpadAttribute> NpadAttributeSet;
typedef nn::util::BitFlagSet<64, NpadButton> NpadButtonSet;
typedef nn::util::BitFlagSet<32, NpadStyleTag> NpadStyleSet;

struct AnalogStickState {
    s32 mX;
    s32 mY;
};

struct NpadBaseState {
    u64 mSamplingNumber;
    NpadButtonSet mButtons;
    AnalogStickState mAnalogStickL;
    AnalogStickState mAnalogStickR;
    NpadAttributeSet mAttributes;
};

// All concrete styles share NpadBaseState's layout; the distinct types only
// exist to select the matching GetNpadState overload.
struct NpadFullKeyState : NpadBaseState { };
struct NpadHandheldState : NpadBaseState { };
struct NpadJoyDualState : NpadBaseState { };

void InitializeNpad();
void SetSupportedNpadIdType(const u32*, u64);
void SetSupportedNpadStyleSet(NpadStyleSet);
NpadStyleSet GetNpadStyleSet(const u32& port);

void GetNpadState(NpadFullKeyState*, const u32& port);
void GetNpadState(NpadHandheldState*, const u32& port);
void GetNpadState(NpadJoyDualState*, const u32& port);

// ---------------------------------------------------------------------------
// Touch screen
//
// NOTE: InitializeTouchScreen/GetTouchScreenState are NOT present in the
// nnheaders RE headers, so these prototypes are best-effort and must match the
// game's nnSdk symbols to resolve at load time. If touch silently does nothing
// at runtime, suspect a mangling mismatch here.
// ---------------------------------------------------------------------------
enum class TouchAttribute {
    Transferable = 0,
    IsConnected = 1,
};
typedef nn::util::BitFlagSet<32, TouchAttribute> TouchAttributeSet;

struct TouchState {
    u64 mDeltaTime;
    TouchAttributeSet mAttributes;
    s32 mFingerId;
    s32 mX;
    s32 mY;
    s32 mDiameterX;
    s32 mDiameterY;
    s32 mRotationAngle;
};

template <u64 N>
struct TouchScreenState {
    u64 mSamplingNumber;
    s32 mCount;
    TouchState mTouches[N];
};

void InitializeTouchScreen();

template <u64 N>
void GetTouchScreenState(TouchScreenState<N>* outState);

// ---------------------------------------------------------------------------
// Keyboard
// ---------------------------------------------------------------------------
enum class KeyboardModifier {
    Control = 0,
    Shift = 1,
    LeftAlt = 2,
    RightAlt = 3,
    Gui = 4,
    CapsLock = 5,
    ScrollLock = 6,
    NumLock = 7,
    Katakana = 8,
    Hiragana = 9,
};

enum class KeyboardKey {
    A = 4,
    B = 5,
    C = 6,
    D = 7,
    E = 8,
    F = 9,
    G = 10,
    H = 11,
    I = 12,
    J = 13,
    K = 14,
    L = 15,
    M = 16,
    N = 17,
    O = 18,
    P = 19,
    Q = 20,
    R = 21,
    S = 22,
    T = 23,
    U = 24,
    V = 25,
    W = 26,
    X = 27,
    Y = 28,
    Z = 29,
    D1 = 30,
    D2 = 31,
    D3 = 32,
    D4 = 33,
    D5 = 34,
    D6 = 35,
    D7 = 36,
    D8 = 37,
    D9 = 38,
    D0 = 39,
    Return = 40,
    Escape = 41,
    Backspace = 42,
    Tab = 43,
    Space = 44,
    Minus = 45,
    Plus = 46,
    OpenBracket = 47,
    CloseBracket = 48,
    Pipe = 49,
    Tilde = 50,
    Semicolon = 51,
    Quote = 52,
    Backquote = 53,
    Comma = 54,
    Period = 55,
    Slash = 56,
    CapsLock = 57,
    F1 = 58,
    F2 = 59,
    F3 = 60,
    F4 = 61,
    F5 = 62,
    F6 = 63,
    F7 = 64,
    F8 = 65,
    F9 = 66,
    F10 = 67,
    F11 = 68,
    F12 = 69,
    Insert = 73,
    Home = 74,
    PageUp = 75,
    Delete = 76,
    End = 77,
    PageDown = 78,
    RightArrow = 79,
    LeftArrow = 80,
    DownArrow = 81,
    UpArrow = 82,
    NumLock = 83,
    LeftControl = 224,
    LeftShift = 225,
    LeftAlt = 226,
    LeftGui = 227,
    RightControl = 228,
    RightShift = 229,
    RightAlt = 230,
    RightGui = 231,
};

struct KeyboardState {
    u64 mSamplingNumber;
    nn::util::BitFlagSet<32, KeyboardModifier> mModifiers;
    nn::util::BitFlagSet<256, KeyboardKey> mKeys;
};

void InitializeKeyboard();
void GetKeyboardState(KeyboardState*);

} // namespace nn::hid
