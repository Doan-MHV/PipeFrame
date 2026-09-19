#ifndef PIPEFRAME_ECS_WORLD_H
#define PIPEFRAME_ECS_WORLD_H

#include <PipeFrame/ECS/Entity.h>

#include <algorithm>
#include <memory>
#include <span>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pipeframe::ecs {

// Typed, dense component storage. Entity identity belongs to the world, not to
// any particular component. Structural edits invalidate component references.
class World {
    struct Storage {
        virtual ~Storage() = default;
        virtual void Remove(Entity) = 0;
    };
    template <class T>
    struct Pool final : Storage {
        std::vector<T> values;
        std::vector<Entity> entities;
        std::unordered_map<Entity, std::size_t> indices;
        void Remove(Entity entity) override {
            const auto found = indices.find(entity);
            if (found == indices.end()) return;
            const auto index = found->second;
            const auto last = values.size() - 1;
            if (index != last) {
                values[index] = std::move(values[last]);
                entities[index] = entities[last];
                indices[entities[index]] = index;
            }
            values.pop_back();
            entities.pop_back();
            indices.erase(entity);
        }
    };

public:
    // Borrowed batch access: the world must outlive this view and any returned
    // references. Structural edits invalidate it; field edits do not. Acquire
    // before worker dispatch and join workers before changing scene structure.
    template <class T>
    class ComponentAccess {
    public:
        T* Get(Entity entity) const {
            if (owner->structureRevision != revision)
                throw std::logic_error("Component access invalidated by structural edit");
            if (!pool) return nullptr;
            const auto found = pool->indices.find(entity);
            return found == pool->indices.end() ? nullptr : &pool->values[found->second];
        }

    private:
        friend class World;
        ComponentAccess(World& world, Pool<T>* storage)
            : owner(&world), pool(storage), revision(world.structureRevision) {}
        World* owner;
        Pool<T>* pool;
        std::uint64_t revision;
    };
    template <class T>
    ComponentAccess<T> BorrowComponents() {
        return ComponentAccess<T>(*this, FindStorage<T>());
    }
    std::uint64_t StructureRevision() const { return structureRevision; }
    std::uint64_t Revision(Entity entity) const {
        const auto found = revisions.find(entity);
        return found == revisions.end() ? 0 : found->second;
    }
    Entity Create() {
        while (alive.contains(next))
            ++next;
        if (next == 0) throw std::overflow_error("Entity identity exhausted");
        if (!nextRevision) throw std::overflow_error("Entity revision exhausted");
        revisions[next] = nextRevision++;
        alive.insert(next);
        ++structureRevision;
        return next++;
    }
    void Create(Entity entity) {
        if (!entity || alive.contains(entity)) throw std::invalid_argument("Duplicate or invalid entity");
        if (!nextRevision) throw std::overflow_error("Entity revision exhausted");
        revisions[entity] = nextRevision++;
        alive.insert(entity);
        ++structureRevision;
    }
    bool IsAlive(Entity entity) const { return alive.contains(entity); }
    std::vector<Entity> Entities() const {
        std::vector<Entity> result(alive.begin(), alive.end());
        std::ranges::sort(result);
        return result;
    }
    std::size_t Size() const { return alive.size(); }
    bool Destroy(Entity entity) {
        if (!alive.erase(entity)) return false;
        ++structureRevision;
        revisions.erase(entity);
        for (auto& [type, pool] : pools)
            pool->Remove(entity);
        return true;
    }
    void Clear() {
        ++structureRevision;
        pools.clear();
        alive.clear();
        revisions.clear();
        next = 1;
    }

    template <class T, class... Args>
    T& Add(Entity entity, Args&&... args) {
        if (!IsAlive(entity)) throw std::invalid_argument("Entity does not exist");
        auto& pool = StorageFor<T>();
        if (pool.indices.contains(entity)) throw std::invalid_argument("Component already attached");
        ++structureRevision;
        pool.values.emplace_back(std::forward<Args>(args)...);
        pool.entities.push_back(entity);
        pool.indices.emplace(entity, pool.values.size() - 1);
        return pool.values.back();
    }
    template <class T>
    T* Get(Entity entity) {
        auto* pool = FindStorage<T>();
        if (!pool) return nullptr;
        const auto found = pool->indices.find(entity);
        return found == pool->indices.end() ? nullptr : &pool->values[found->second];
    }
    template <class T>
    const T* Get(Entity entity) const {
        return const_cast<World*>(this)->Get<T>(entity);
    }
    template <class T>
    void Remove(Entity entity) {
        if (auto* pool = FindStorage<T>()) {
            ++structureRevision;
            pool->Remove(entity);
        }
    }
    template <class T>
    std::span<T> Components() {
        auto* pool = FindStorage<T>();
        return pool ? std::span<T>(pool->values) : std::span<T>{};
    }
    template <class T>
    std::span<const T> Components() const {
        return const_cast<World*>(this)->Components<T>();
    }
    template <class T>
    std::vector<Entity> EntitiesWith() const {
        const auto* pool = const_cast<World*>(this)->FindStorage<T>();
        return pool ? pool->entities : std::vector<Entity>{};
    }
    template <class First, class... Rest, class Function>
    void Each(Function function) {
        // Snapshot identities so deletion is safe between callbacks. A callback
        // must not retain references after making structural changes.
        for (Entity entity : EntitiesWith<First>()) {
            auto* first = Get<First>(entity);
            if (first && (... && (Get<Rest>(entity) != nullptr))) function(entity, *first, *Get<Rest>(entity)...);
        }
    }

private:
    template <class T>
    Pool<T>* FindStorage() {
        const auto found = pools.find(typeid(T));
        return found == pools.end() ? nullptr : static_cast<Pool<T>*>(found->second.get());
    }
    template <class T>
    Pool<T>& StorageFor() {
        auto& storage = pools[typeid(T)];
        if (!storage) storage = std::make_unique<Pool<T>>();
        return *static_cast<Pool<T>*>(storage.get());
    }
    std::unordered_map<std::type_index, std::unique_ptr<Storage>> pools;
    std::unordered_set<Entity> alive;
    Entity next{1};
    std::uint64_t structureRevision{};
    std::unordered_map<Entity, std::uint64_t> revisions;
    std::uint64_t nextRevision{1};
};
}  // namespace pipeframe::ecs
#endif
