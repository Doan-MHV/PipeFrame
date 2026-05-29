#ifndef PIPEFRAME_COMPONENTANNOTATIONS_H
#define PIPEFRAME_COMPONENTANNOTATIONS_H

namespace PF
{
struct ComponentFlag
{
};

struct PropertyVisibilityFlag
{
};

struct PropertyStorageFlag
{
};

struct NameOverride
{
    const char* value;
};

inline constexpr ComponentFlag Engine{};
inline constexpr ComponentFlag NotAddable{};
inline constexpr ComponentFlag NotRemovable{};
inline constexpr ComponentFlag NotSerializable{};
inline constexpr ComponentFlag Manual{};

inline constexpr PropertyVisibilityFlag Edit{};
inline constexpr PropertyVisibilityFlag ReadOnly{};
inline constexpr PropertyVisibilityFlag Hidden{};

inline constexpr PropertyStorageFlag Save{};
inline constexpr PropertyStorageFlag RuntimeOnly{};

constexpr NameOverride DisplayName(const char* value)
{
    return {value};
}

constexpr NameOverride JsonName(const char* value)
{
    return {value};
}
}

// PipeFrame reflection annotations.
//
// Recommended project syntax:
//
//   PF_COMPONENT()
//   struct HealthComponent
//   {
//       PF_PROPERTY(PF::Edit, PF::Save, 0, 100, 1)
//       int health = 100;
//
//       PF_PROPERTY(PF::ReadOnly, PF::RuntimeOnly)
//       int runtimeCounter = 0;
//   };
//
// Names are inferred by default:
//   HealthComponent -> "Health Component" in the editor.
//   health -> "Health" in the editor.
//
// Project JSON fields use the C++ field name by default. Engine JSON fields use
// snake_case for the existing asset format. Use PF::DisplayName("...") or
// PF::JsonName("...") only when the inferred name is not what you want.
//
// These macros are intentionally empty for the C++ compiler. PipeFrameHeaderTool
// reads them before compilation and generates the descriptor code used by
// serialization, the inspector, and add/remove menus.
#define PF_COMPONENT(...)
#define PF_PROPERTY(...)
#define PF_ENUM_PROPERTY(...)
#define PF_SECONDS_PROPERTY(...)

#endif // PIPEFRAME_COMPONENTANNOTATIONS_H
