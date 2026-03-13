#ifndef PIPEFRAME_ECS_H
#define PIPEFRAME_ECS_H

#include <bitset>
#include <format>
#include <set>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Logger/Logger.h"

const unsigned int MAX_COMPONENTS = 32;

typedef std::bitset<MAX_COMPONENTS> Signature;

struct IComponent
{
protected:
    static int nextId;
};

template <typename T>
class Component : public IComponent
{
public:
    static int GetId()
    {
        static auto id = nextId++;
        return id;
    }
};

class Entity
{
    int id;

public:
    Entity(int id) : id(id)
    {
    }

    int GetId() const;

    Entity& operator =(const Entity& other) = default;
    bool operator ==(const Entity& other) const { return id == other.id; }
    bool operator !=(const Entity& other) const { return id != other.id; }
    bool operator >(const Entity& other) const { return id > other.id; }
    bool operator <(const Entity& other) const { return id < other.id; }

    template <typename TComponent, typename... TArgs>
    void AddComponent(TArgs&&... args);
    template <typename TComponent>
    void RemoveComponent();
    template <typename TComponent>
    bool HasComponent() const;
    template <typename TComponent>
    TComponent& GetComponent() const;

    // Hold a pointer to the entity's owner registry
    class Registry* registry;
};

class System
{
    Signature componentSignature;
    std::vector<Entity> entities;

public:
    System() = default;
    ~System() = default;

    void AddEntityToSystem(Entity entity);
    void RemoveEntityFromSystem(Entity entity);
    std::vector<Entity> GetSystemEntities() const;
    const Signature& GetComponentSignature() const;

    template <typename TComponent>
    void RequirementComponent();
};

class IPool
{
public:
    virtual ~IPool()
    {
    }
};

template <typename T>
class Pool : public IPool
{
    std::vector<T> data;

public:
    Pool(int size = 100)
    {
        data.resize(size);
    }

    virtual ~Pool() = default;

    bool IsEmpty() const { return data.empty(); }

    int GetSize() const { return data.size(); }

    void Resize(int newSize)
    {
        data.resize(newSize);
    }

    void Clear() { data.clear(); }

    void Add(T object)
    {
        data.push_back(object);
    }

    void Set(int index, T object)
    {
        data[index] = object;
    }

    T& Get(int index)
    {
        return static_cast<T&>(data[index]);
    }

    T& operator[](unsigned int index)
    {
        return data[index];
    }
};

class Registry
{
    int numEntities = 0;
    std::vector<std::shared_ptr<IPool>> componentPools;
    std::vector<Signature> entityComponentSignatures;
    std::unordered_map<std::type_index, System*> systems;

    std::set<Entity> entitiesToBeAdded;
    std::set<Entity> entitiesToBeRemoved;

public:
    Registry()
    {
        PF_INFO("Registry created");
    };

    ~Registry()
    {
        PF_INFO("~Registry destroyed");
    }

    void Update();

    Entity CreateEntity();

    // Component management
    template <typename TComponent, typename... TArgs>
    void AddComponent(Entity entity, TArgs&&... args);
    template <typename TComponent>
    void RemoveComponent(Entity entity);
    template <typename TComponent>
    bool HasComponent(Entity entity) const;
    template <typename TComponent>
    TComponent& GetComponent(Entity entity) const;

    // System management
    template <typename TSystem, typename... TArgs>
    void AddSystem(TArgs&&... args);
    template <typename TSystem>
    void RemoveSystem();
    template <typename TSystem>
    bool HasSystem() const;
    template <typename TSystem>
    TSystem& GetSystem() const;

    void AddEntityToSystem(Entity entity);
};

template <typename TComponent>
void System::RequirementComponent()
{
    const auto componentId = Component<TComponent>::GetId();
    componentSignature.set(componentId);
}

template <typename TSystem, typename... TArgs>
void Registry::AddSystem(TArgs&&... args)
{
    std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);
    systems.insert(std::make_pair(std::type_index(typeid(TSystem)), newSystem));
}

template <typename TSystem>
void Registry::RemoveSystem()
{
    const auto system = systems.find(std::type_index(typeid(TSystem)));
    systems.erase(system);
}

template <typename TSystem>
bool Registry::HasSystem() const
{
    return systems.contains(std::type_index(typeid(TSystem)));
}

template <typename TSystem>
TSystem& Registry::GetSystem() const
{
    const auto system = systems.find(std::type_index(typeid(TSystem)));
    return *std::static_pointer_cast<TSystem>(system->second);
}

template <typename TComponent, typename... TArgs>
void Registry::AddComponent(Entity entity, TArgs&&... args)
{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    if (componentId >= componentPools.size())
    {
        componentPools.resize(componentId + 1, nullptr);
    }

    if (!componentPools[componentId])
    {
        std::shared_ptr<Pool<TComponent>> newComponentPool = std::make_shared<Pool<TComponent>>();
        componentPools[componentId] = newComponentPool;
    }

    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(
        componentPools[componentId]);

    if (entityId >= componentPool->GetSize())
    {
        componentPool->Resize(numEntities);
    }

    TComponent newComponent(std::forward<TArgs>(args)...);
    componentPool->Set(entityId, newComponent);
    entityComponentSignatures[entityId].set(componentId);

    PF_INFO("Component id = " + std::to_string(componentId) + " was added to entity id " + std::to_string(entityId));;
}

template <typename TComponent>
void Registry::RemoveComponent(Entity entity)
{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    entityComponentSignatures[entityId].set(componentId, false);
}

template <typename TComponent>
bool Registry::HasComponent(Entity entity) const
{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    return entityComponentSignatures[entityId].test(componentId);
}

template <typename TComponent>
TComponent& Registry::GetComponent(Entity entity) const
{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    auto componentPool = std::static_pointer_cast<TComponent>(componentPools[componentId]);
    return componentPool->Get(entityId);
}

template <typename TComponent, typename... TArgs>
void Entity::AddComponent(TArgs&&... args)
{
    registry->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}

template <typename TComponent>
void Entity::RemoveComponent()
{
    registry->RemoveComponent<TComponent>(*this);
}

template <typename TComponent>
bool Entity::HasComponent() const
{
    return registry->HasComponent<TComponent>(*this);
}

template <typename TComponent>
TComponent& Entity::GetComponent() const
{
    return registry->GetComponent<TComponent>(*this);
}

#endif //PIPEFRAME_ECS_H
