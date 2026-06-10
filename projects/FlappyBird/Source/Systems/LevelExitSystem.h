#ifndef LevelExitSystem_H
#define LevelExitSystem_H

#include "Components/BirdComponent.h"
#include "Components/LevelExitComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Events/CollisionEvent.h"

class LevelExitSystem : public EntitySystem {
    PF_SYSTEM_QUERY(PF_QUERY_FIELD(LevelExitComponent, exit), PF_QUERY_FIELD(TransformComponent, transform))

  public:
    void Loaded() override { RequireSystemQuery(); }

    void SubscribeToEvents(EntitySystemContext &context) override {
        Listen<CollisionEvent>(context, &LevelExitSystem::OnCollision);
    }

  private:
    void OnCollision(EntitySystemContext &context, CollisionEvent &event) {
        Entity birdEntity(-1);
        Entity exitEntity(-1);

        if (!event.TryGetPair<BirdComponent, LevelExitComponent>(birdEntity, exitEntity)) {
            return;
        }

        if (!HasEntity(exitEntity)) {
            return;
        }

        Query q = GetSystemQuery(exitEntity);
        if (q.exit.targetLevel.empty()) {
            return;
        }

        RequestLevelLoad(context, q.exit.targetLevel);
    }
};

#endif // LevelExitSystem_H
