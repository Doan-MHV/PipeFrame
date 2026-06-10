

#ifndef PIPEFRAME_ENTITYSYSTEM_H
#define PIPEFRAME_ENTITYSYSTEM_H

#include <filesystem>
#include <type_traits>
#include <utility>
#include <vector>

#include <SDL3/SDL_rect.h>

#include "Component.h"
#include "Entity.h"
#include "EventBus/EventBus.h"
#include "Game/LevelLoadRequests.h"
#include "Signature.h"

struct SDL_Renderer;
class AssetRegistry;
class Registry;
class TileMap;

struct EntitySystemContext {
    Registry &registry;
    EventBus &eventBus;
    TileMap &tileMap;
    AssetRegistry &assetRegistry;
    SDL_Renderer *renderer = nullptr;
    SDL_FRect &camera;
    LevelLoadRequests &levels;
    double deltaTime = 0.0;
    int elapsedTime = 0;
};

class EntitySystem {
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

    virtual void Loaded(EntitySystemContext &context) {
        (void)context;
        Loaded();
    }
    virtual void Start(EntitySystemContext &context) {
        (void)context;
        Start();
    }
    virtual void SubscribeToEvents(EntitySystemContext &context) { (void)context; }
    virtual void Update(EntitySystemContext &context) { (void)context; }
    virtual void Stop(EntitySystemContext &context) {
        (void)context;
        Stop();
    }
    virtual void Unloaded(EntitySystemContext &context) {
        (void)context;
        Unloaded();
    }

    void AddEntityToSystem(Entity entity);
    void RemoveEntityFromSystem(Entity entity);
    bool HasEntity(Entity entity) const;
    const std::vector<Entity> &GetSystemEntities() const;
    const Signature &GetComponentSignature() const;

    template <typename TComponent> void RequireComponent();

    template <typename TQuery> void RequireQuery();

    template <typename TQuery> TQuery GetQuery(Entity entity);

    template <typename TQuery, typename TCallback> void ForEach(TCallback &&callback);

    template <typename TEvent, typename TSystem>
    void Listen(EntitySystemContext &context, void (TSystem::*callback)(TEvent &));

    template <typename TEvent, typename TSystem>
    void Listen(EntitySystemContext &context, void (TSystem::*callback)(EntitySystemContext &, TEvent &));

    template <typename TEvent, typename... TArgs> void Emit(EntitySystemContext &context, TArgs &&...args);

    void RequestLevelLoad(EntitySystemContext &context, const std::filesystem::path &levelPath) {
        context.levels.RequestLoad(levelPath);
    }
};

template <typename TComponent> void EntitySystem::RequireComponent() {
    const auto componentId = Component<TComponent>::GetId();
    componentSignature.set(componentId);
}

template <typename TQuery> void EntitySystem::RequireQuery() { TQuery::Require(*this); }

template <typename TQuery> TQuery EntitySystem::GetQuery(Entity entity) { return TQuery::Build(entity); }

template <typename TQuery, typename TCallback> void EntitySystem::ForEach(TCallback &&callback) {
    for (Entity entity : entities) {
        TQuery query = TQuery::Build(entity);

        if constexpr (std::is_invocable_v<TCallback &, Entity, TQuery &>) {
            callback(entity, query);
        } else {
            callback(query);
        }
    }
}

template <typename TEvent, typename TSystem>
void EntitySystem::Listen(EntitySystemContext &context, void (TSystem::*callback)(TEvent &)) {
    context.eventBus.SubscribeToEvent<TEvent>(static_cast<TSystem *>(this), callback);
}

template <typename TEvent, typename TSystem>
void EntitySystem::Listen(EntitySystemContext &context, void (TSystem::*callback)(EntitySystemContext &, TEvent &)) {
    context.eventBus.SubscribeToEvent<TEvent>(static_cast<TSystem *>(this), callback);
}

template <typename TEvent, typename... TArgs> void EntitySystem::Emit(EntitySystemContext &context, TArgs &&...args) {
    context.eventBus.EmitEventWithContext<TEvent>(context, std::forward<TArgs>(args)...);
}

#include "SystemQuery.h"

#endif // PIPEFRAME_ENTITYSYSTEM_H
