#ifndef PIPEFRAME_SIMULATION_SYSTEM_H
#define PIPEFRAME_SIMULATION_SYSTEM_H

#include <string_view>

namespace pipeframe {

// PipeFrame owns scheduling. Projects implement only the domain rule for a step.
template <typename Result>
class FixedUpdateSystem {
public:
    virtual ~FixedUpdateSystem() = default;
    [[nodiscard]] virtual std::string_view GetSystemId() const = 0;
    virtual Result Update(float fixedDeltaTime) = 0;
};

template <typename Entity, typename Environment, typename Command>
class EntityUpdateSystem {
public:
    virtual ~EntityUpdateSystem() = default;
    [[nodiscard]] virtual std::string_view GetSystemId() const = 0;
    virtual void Update(Entity &entity, const Environment &environment,
                        const Command &command, float fixedDeltaTime) const = 0;
};

template <typename World, typename Result>
class PhysicsSolver {
public:
    virtual ~PhysicsSolver() = default;
    [[nodiscard]] virtual std::string_view GetSystemId() const = 0;
    virtual Result Solve(World &world) = 0;
};

} // namespace pipeframe

#endif
