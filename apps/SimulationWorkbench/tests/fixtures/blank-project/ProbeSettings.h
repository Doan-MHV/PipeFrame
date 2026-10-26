#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
struct ProbeSettings {
    float speed{20}, range{80}, stopDistance{6};
    pipeframe::Vector2f direction{1,0};
    float nearest{-1};
    bool blocked{};
    static auto Schema() {
        using namespace pipeframe;
        return ComponentSchema<ProbeSettings>("project.ProbeSettings", "Probe Settings")
            .Editable({.key="speed",.displayName="Speed",.kind=PropertyKind::Number,.defaultValue=20.0,.minimum=0,.maximum=200},&ProbeSettings::speed)
            .Editable({.key="range",.displayName="Ray Range",.kind=PropertyKind::Number,.defaultValue=80.0,.minimum=1,.maximum=1000},&ProbeSettings::range)
            .Editable({.key="stopDistance",.displayName="Stop Distance",.kind=PropertyKind::Number,.defaultValue=6.0,.minimum=0,.maximum=1000},&ProbeSettings::stopDistance)
            .Editable({.key="direction",.displayName="Direction",.kind=PropertyKind::Vector2,.defaultValue=Vector2f{1,0}},&ProbeSettings::direction)
            .ReadOnly({.key="nearest",.displayName="Nearest Hit",.kind=PropertyKind::Number,.defaultValue=-1.0},&ProbeSettings::nearest)
            .ReadOnly({.key="blocked",.displayName="Blocked",.kind=PropertyKind::Boolean,.defaultValue=false},&ProbeSettings::blocked)
            .Validate("Stop distance must be within ray range and direction must be nonzero",[](const auto &v){return v.stopDistance<=v.range && (v.direction.x!=0 || v.direction.y!=0);});
    }
};
