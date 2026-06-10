#ifndef FLAPPY_BIRD_SCORE_SYSTEM_H
#define FLAPPY_BIRD_SCORE_SYSTEM_H

#include <algorithm>

#include "Components/BirdComponent.h"
#include "Components/ScoreComponent.h"
#include "Components/ScoreZoneComponent.h"
#include "Components/TriggerComponent.h"
#include "ECS/EntitySystem.h"
#include "Events/CollisionEvent.h"

class ScoreSystem : public EntitySystem
{
    PF_SYSTEM_QUERY(PF_QUERY_FIELD(ScoreZoneComponent, scoreZone),
                    PF_QUERY_FIELD(TriggerComponent, trigger))

public:
    void Loaded() override { RequireSystemQuery(); }

    void SubscribeToEvents(EntitySystemContext& context) override
    {
        Listen<CollisionEvent>(context, &ScoreSystem::OnCollision);
    }

private:
    void OnCollision(EntitySystemContext& context, CollisionEvent& event)
    {
        (void)context;

        Entity scoreEntity(-1);
        Entity scoreZoneEntity(-1);

        if (!event.TryGetPair<ScoreComponent, ScoreZoneComponent>(scoreEntity, scoreZoneEntity))
        {
            return;
        }

        if (!HasEntity(scoreZoneEntity))
        {
            return;
        }

        if (scoreEntity.HasComponent<BirdComponent>() && !scoreEntity.GetComponent<BirdComponent>().isAlive)
        {
            return;
        }

        Query q = GetSystemQuery(scoreZoneEntity);

        if ((q.trigger.once && q.trigger.activated) || q.scoreZone.scored)
        {
            return;
        }

        auto& score = scoreEntity.GetComponent<ScoreComponent>();
        score.score += q.scoreZone.scoreValue;
        score.bestScore = std::max(score.bestScore, score.score);

        q.scoreZone.scored = true;
        q.trigger.activated = true;
    }
};

#endif // FLAPPY_BIRD_SCORE_SYSTEM_H
