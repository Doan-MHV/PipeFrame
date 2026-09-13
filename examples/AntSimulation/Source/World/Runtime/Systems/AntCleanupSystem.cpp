#include "World/Runtime/Systems/AntCleanupSystem.h"
namespace ant_simulation {
AntCleanupSystem::AntCleanupSystem(AntQuery &s, ColonyLifecycleSystem &c, AntBodySystem &p, pipeframe::Vector2i size)
    : store(s), colonies(c), physics(p), contacts(s, size) {}

AntCleanupResult AntCleanupSystem::Update(float) {
    AntCleanupResult result;
    result.enemyAlerts = contacts.ProcessContacts();
    physics.Synchronize();
    result.removedAnts = store.RemoveDead();
    colonies.RecountAntsAndUpdateRadii();
    return result;
}
} // namespace ant_simulation
