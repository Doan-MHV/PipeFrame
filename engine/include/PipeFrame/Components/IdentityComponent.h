#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
namespace pipeframe {
struct IdentityComponent {
    std::string label;
    static auto Schema() {
        return ComponentSchema<IdentityComponent>("pipeframe.identity","Identity").Required()
            .Editable({.key="label",.displayName="Label",.kind=PropertyKind::String,.defaultValue=std::string{}},&IdentityComponent::label);
    }
};
}
