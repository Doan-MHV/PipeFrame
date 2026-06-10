#ifndef PipeSpawnerSystem_H
#define PipeSpawnerSystem_H

#include "Components/TransformComponent.h"
#include "ECS/ECS.h"

class PipeSpawnerSystem : public EntitySystem {
    PF_SYSTEM_QUERY(PF_QUERY_FIELD(TransformComponent, transform))

  public:
    void Loaded() override { RequireSystemQuery(); }

    void Start(EntitySystemContext &context) override { (void)context; }

    void SubscribeToEvents(EntitySystemContext &context) override {
        (void)context;
        // Listen<MyEvent>(context, &PipeSpawnerSystem::OnMyEvent);
    }

    void Update(EntitySystemContext &context) override {
        ForEachSystemQuery([&](Query q) { q.transform.position.x += 0.0f * static_cast<float>(context.deltaTime); });
    }

    void Stop(EntitySystemContext &context) override { (void)context; }
};

#endif // PipeSpawnerSystem_H
