#ifndef FlappyBoundsSystem_H
#define FlappyBoundsSystem_H

#include "Components/BirdComponent.h"
#include "Components/SpriteComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Events/GameOverEvent.h"
#include "Map/TileMap.h"

class FlappyBoundsSystem : public EntitySystem
{
    PF_SYSTEM_QUERY(PF_QUERY_FIELD(TransformComponent, transform),
                    PF_QUERY_FIELD(BirdComponent, bird),
                    PF_QUERY_FIELD(SpriteComponent, sprite))

public:
    void Loaded() override { RequireSystemQuery(); }

    void Start(EntitySystemContext& context) override { (void)context; }

    void SubscribeToEvents(EntitySystemContext& context) override
    {
        (void)context;
        // Listen<MyEvent>(context, &FlappyBoundsSystem::OnMyEvent);
    }

    void Update(EntitySystemContext& context) override
    {
        const float topBound = 0.0f;
        const float bottomBound = static_cast<float>(context.tileMap.GetWorldHeight());

        ForEachSystemQuery([&](Entity entity, Query q)
        {
            if (!q.bird.isAlive)
            {
                return;
            }

            if (q.transform.position.y < topBound)
            {
                Emit<GameOverEvent>(context, entity, "Hit the ceiling");
                return;
            }

            if (q.transform.position.y + q.sprite.height >= bottomBound)
            {
                Emit<GameOverEvent>(context, entity, "Hit the floor");
            }
        });
    }

    void Stop(EntitySystemContext& context) override { (void)context; }
};

#endif // FlappyBoundsSystem_H
