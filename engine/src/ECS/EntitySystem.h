

#ifndef PIPEFRAME_ENTITYSYSTEM_H
#define PIPEFRAME_ENTITYSYSTEM_H


#include <utility>
#include <vector>

#include <SDL3/SDL_rect.h>

#include "Component.h"
#include "Entity.h"
#include "EventBus/EventBus.h"
#include "Signature.h"

struct SDL_Renderer;
class AssetRegistry;
class Registry;
class TileMap;

struct EntitySystemContext
{
    Registry& registry;
    EventBus& eventBus;
    TileMap& tileMap;
    AssetRegistry& assetRegistry;
    SDL_Renderer* renderer = nullptr;
    SDL_FRect& camera;
    double deltaTime = 0.0;
    int elapsedTime = 0;
};

class EntitySystem
{
private:
    Signature componentSignature;
    std::vector<Entity> entities;

public:
    EntitySystem() = default;
    virtual ~EntitySystem() = default;

    virtual void Loaded() {}
    virtual void Start() {}
    virtual void Stop() {}
    virtual void Unloaded() {}

    virtual void Loaded(EntitySystemContext& context)
    {
        (void)context;
        Loaded();
    }
    virtual void Start(EntitySystemContext& context)
    {
        (void)context;
        Start();
    }
    virtual void SubscribeToEvents(EntitySystemContext& context) { (void)context; }
    virtual void Update(EntitySystemContext& context) { (void)context; }
    virtual void Stop(EntitySystemContext& context)
    {
        (void)context;
        Stop();
    }
    virtual void Unloaded(EntitySystemContext& context)
    {
        (void)context;
        Unloaded();
    }

    void AddEntityToSystem(Entity entity);
    void RemoveEntityFromSystem(Entity entity);
    std::vector<Entity> GetSystemEntities() const;
    const Signature& GetComponentSignature() const;

    template <typename TComponent>
    void RequireComponent();

    template <typename TEvent, typename TSystem>
    void Listen(EntitySystemContext& context, void (TSystem::*callback)(TEvent&));

    template <typename TEvent, typename... TArgs>
    void Emit(EntitySystemContext& context, TArgs&&... args);
};

template <typename TComponent>
void EntitySystem::RequireComponent()
{
    const auto componentId = Component<TComponent>::GetId();
    componentSignature.set(componentId);
}

template <typename TEvent, typename TSystem>
void EntitySystem::Listen(EntitySystemContext& context, void (TSystem::*callback)(TEvent&))
{
    context.eventBus.SubscribeToEvent<TEvent>(static_cast<TSystem*>(this), callback);
}

template <typename TEvent, typename... TArgs>
void EntitySystem::Emit(EntitySystemContext& context, TArgs&&... args)
{
    context.eventBus.EmitEvent<TEvent>(std::forward<TArgs>(args)...);
}

#endif //PIPEFRAME_ENTITYSYSTEM_H
