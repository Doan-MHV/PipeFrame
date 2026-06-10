#ifndef FLAPPY_BIRD_FLAPPY_DEATH_SYSTEM_H
#define FLAPPY_BIRD_FLAPPY_DEATH_SYSTEM_H

#include "Components/BirdComponent.h"
#include "Components/HazardComponent.h"
#include "ECS/EntitySystem.h"
#include "Events/CollisionEvent.h"
#include "Events/GameOverEvent.h"

class FlappyDeathSystem : public EntitySystem
{
public:
    void SubscribeToEvents(EntitySystemContext& context) override
    {
        Listen<CollisionEvent>(context, &FlappyDeathSystem::OnCollision);
    }

private:
    void OnCollision(EntitySystemContext& context, CollisionEvent& event)
    {
        Entity birdEntity(-1);
        Entity hazardEntity(-1);

        if (!event.TryGetPair<BirdComponent, HazardComponent>(birdEntity, hazardEntity))
        {
            return;
        }

        const auto& hazard = hazardEntity.GetComponent<HazardComponent>();
        if (!hazard.deadly)
        {
            return;
        }

        Emit<GameOverEvent>(context, birdEntity, "Bird hit hazard");
    }
};

#endif // FLAPPY_BIRD_FLAPPY_DEATH_SYSTEM_H
