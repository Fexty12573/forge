#pragma once
#ifdef __cplusplus

#include "Mt/MtDti.h"
#include "Mt/MtObject.h"
#include "switch/types.h"

enum class MtType : u16 {
    Undefined = 0,
    Class,
    ClassRef,
    Bool,
    U8,
    U16,
    U32,
    U64,
    S8,
    S16,
    S32,
    S64,
    F32,
    F64,
    String,
    Color,
    Point,
    Size,
    Rect,
    Matrix,
    Vector3,
    Vector4,
    Quaternion,
    Property,
    Event,
    Group,
    PageBegin,
    PageEnd,
    Event32,
    Array,
    PropertyList,
    GroupEnd,
    CString,
    Time,
    Float2,
    Float3,
    Float4,
    Float3x3,
    Float4x3,
    Float4x4,
    Easecurve,
    Line,
    LineSegment,
    Ray,
    Plane,
    Sphere,
    Capsule,
    Aabb,
    Obb,
    Cylinder,
    Triangle,
    Cone,
    Torus,
    Ellipsoid,
    Range,
    RangeF,
    RangeU16,
    Hermitecurve,
    EnumList,
    Float3x4,
    LineSegment4,
    Aabb4,
    Oscillator,
    Variable,
    Vector2,
    Matrix33,
    Rect3dXz,
    Rect3d,
    Rect3dCollision,
    PlaneXz,
    RayY,
    PointF,
    SizeF,
    RectF,
    Event64,
    Count,

    Custom = 0x80
};

struct MtProperty {
    const char* name;
    MtType type;
    u16 attr;
    MtObject* owner;
    union {
        void* ptr;
        void* (*getter)(const MtObject* obj);
        void* (*getter32)(const MtObject* obj, u32);
        void* (*getter64)(const MtObject* obj, u64);
    };
    union {
        u32 count;
        u64 count64;
        u32 (*get_count)(const MtObject* obj);
    };
    void (*setter)(MtObject* obj, void* value);
    void (*set_count)(MtObject* obj, u32 count);
    u32 index;
    MtProperty* next;
    MtProperty* prev;
};

#endif
