#ifndef ANT_CONTACT_SYSTEM_H
#define ANT_CONTACT_SYSTEM_H

#include <cstddef>

#include <PipeFrame/Foundation/MathTypes.h>

#include "World/Runtime/AntQuery.h"
#include "World/Physics/CollisionGrid.h"

namespace ant_simulation {

class AntContactSystem {
public:
    static constexpr float ContactDistance{
        1.0f
    };

    AntContactSystem(
        AntQuery &antStore,
        pipeframe::Vector2i worldSize
    );

    std::size_t ProcessContacts();

private:
    [[nodiscard]]
    bool HasEnemyContact(
        const AntView &ant
    ) const;

    AntQuery &antStore;
    CollisionGrid collisionGrid;
};

} // namespace ant_simulation

#endif
