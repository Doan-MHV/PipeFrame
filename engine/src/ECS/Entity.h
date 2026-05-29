

#ifndef PIPEFRAME_ENTITY_H
#define PIPEFRAME_ENTITY_H


#include <string>

class Registry;

class Entity
{
private:
    int id;

public:
    Entity(int id) : id(id)
    {
    }

    Entity(const Entity& entity) = default;
    Entity& operator=(const Entity& other) = default;

    void Kill();
    int GetId() const;

    void Tag(const std::string& tag);
    bool HasTag(const std::string& tag) const;
    void Group(const std::string& group);
    bool BelongsToGroup(const std::string& group) const;

    void RemoveGroup();
    void RemoveTag();

    bool operator==(const Entity& other) const { return id == other.id; }
    bool operator!=(const Entity& other) const { return id != other.id; }
    bool operator>(const Entity& other) const { return id > other.id; }
    bool operator<(const Entity& other) const { return id < other.id; }

    template <typename TComponent, typename... TArgs>
    void AddComponent(TArgs&&... args);

    template <typename TComponent>
    void RemoveComponent();

    template <typename TComponent>
    bool HasComponent() const;

    template <typename TComponent>
    TComponent& GetComponent() const;

    Registry* registry = nullptr;
};


#endif //PIPEFRAME_ENTITY_H
