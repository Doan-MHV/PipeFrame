#ifndef PIPEFRAME_EXAMPLEENTITYSYSTEM_H
#define PIPEFRAME_EXAMPLEENTITYSYSTEM_H

#include "Components/ExampleComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"

#include "Events/ExampleEvent.h"

class ExampleEntitySystem : public EntitySystem
{
public:
    void Loaded() override
    {
        RequireComponent<TransformComponent>();
        RequireComponent<ExampleComponent>();
    }

    void SubscribeToEvents(EntitySystemContext& context) override
    {
        Listen<ExampleEvent>(context, &ExampleEntitySystem::OnExampleEvent);
    }

    void Update(EntitySystemContext& context) override
    {
        for (Entity entity : GetSystemEntities())
        {
            auto& transform = entity.GetComponent<TransformComponent>();
            const auto& example = entity.GetComponent<ExampleComponent>();
            transform.rotation += example.value * static_cast<float>(context.deltaTime);
            // Emit<ExampleEvent>(context, entity, "Example entity updated");
        }
    }

private:
    void OnExampleEvent(ExampleEvent& event)
    {
        (void)event;
    }
};

#endif // PIPEFRAME_EXAMPLEENTITYSYSTEM_H
