#pragma once
#include <PipeFrame/Project/ComponentSchema.h>

#include <concepts>

namespace pipeframe {

// Compile-time authoring base. ECS storage still contains the concrete data type;
// there is no vtable, instance metadata or base-pointer ownership.
// Derived must expose public static BuildSchema() returning ComponentSchema<Derived>.
template <class Derived>
struct Component {
  static ComponentSchema<Derived> Schema() {
    constexpr bool hasSchema = requires {
      { Derived::BuildSchema() } -> std::same_as<ComponentSchema<Derived>>;
    };
    static_assert(hasSchema,
      "Component<T> requires public static BuildSchema() returning ComponentSchema<T>");
    if constexpr (hasSchema) return Derived::BuildSchema();
  }

  // Allows components to default their own equality operator.
  constexpr bool operator==(const Component&) const = default;

protected:
  static ComponentSchema<Derived> Define(std::string typeId, std::string displayName) {
    return ComponentSchema<Derived>(std::move(typeId), std::move(displayName));
  }
};

}  // namespace pipeframe
