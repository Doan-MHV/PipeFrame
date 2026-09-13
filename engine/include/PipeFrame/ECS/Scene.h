#ifndef PIPEFRAME_ECS_SCENE_H
#define PIPEFRAME_ECS_SCENE_H
#include <PipeFrame/ECS/World.h>
#include <PipeFrame/Core/ServiceRegistry.h>
#include <PipeFrame/Core/Coroutine.h>
#include <PipeFrame/Entities/EntityArchetype.h>
#include <functional>
#include <exception>
#include <deque>
#include <string>
#include <PipeFrame/Foundation/MathTypes.h>

namespace pipeframe {
class SceneObject;
class BehaviourScene;
struct CollisionEvent2D {
    ecs::Entity other{};
    Vector2f point{}, normal{};
};
class Behaviour {
public:
    virtual ~Behaviour() = default;
    virtual void Start() {}
    virtual void Update(float) {}
    virtual void FixedUpdate(float) {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}
    virtual void OnCollisionEnter(const CollisionEvent2D &) {}
    virtual void OnCollisionExit(const CollisionEvent2D &) {}
    virtual void OnTriggerEnter(const CollisionEvent2D &) {}
    virtual void OnTriggerExit(const CollisionEvent2D &) {}
    virtual void OnBecameVisible() {}
    virtual void OnBecameInvisible() {}
    using CoroutineId=std::uint64_t;
    CoroutineId StartCoroutine(Coroutine routine) {
        const auto id=++nextCoroutine;
        tasks.push_back(std::make_unique<Task>(Task{id,std::move(routine)}));return id;
    }
    void StopCoroutine(CoroutineId id) { for(auto &task:tasks)if(task->id==id)task->cancelled=true; }
    void StopAllCoroutines() { for(auto &task:tasks)task->cancelled=true; }
    ecs::Entity GetEntity() const { return entity; }
    SceneObject GetObject() const;
    template<class T> T *GetService() const;
    template<class T> T *GetComponent() { return world->Get<T>(entity); }
    template<class T, class... Args> T &AddComponent(Args&&... args) {
        return world->Add<T>(entity, std::forward<Args>(args)...);
    }
private:
    friend class BehaviourScene;
    struct Task { CoroutineId id; Coroutine routine; bool cancelled{}; };
    std::vector<std::unique_ptr<Task>> tasks;
    CoroutineId nextCoroutine{};
    template<class Allowed> void AdvanceCoroutines(float delta,Allowed allowed) {
        const auto count=tasks.size();
        for(std::size_t i=0;i<count&&allowed();++i){auto &task=*tasks[i];if(!task.cancelled&&!task.routine.Tick(delta))task.cancelled=true;}
        std::erase_if(tasks,[](const auto &task){return task->cancelled;});
    }
    BehaviourScene *owner{};
    ecs::World *world{};
    ecs::Entity entity{};
};

// Owns attachment and lifecycle dispatch. Destruction requested from callbacks
// is applied after dispatch, so scripts cannot delete themselves mid-call.
class BehaviourScene {
    struct Entry {
        std::unique_ptr<Behaviour> value;
        bool enabled{true}, active{}, started{};
    };
public:
    BehaviourScene() = default;
    BehaviourScene(const BehaviourScene &) = delete;
    BehaviourScene &operator=(const BehaviourScene &) = delete;
    SceneObject CreateObject();
    SceneObject Instantiate(const EntityArchetype &recipe);
    SceneObject GetObject(ecs::Entity entity);
    ~BehaviourScene() { try { Clear(); } catch (...) { /* Destructors cannot propagate script exceptions. */ } }
    ServiceRegistry &Services() { return services; }
    const ServiceRegistry &Services() const { return services; }
    ecs::World &Components() { return world; }
    const ecs::World &Components() const { return world; }
    ecs::Entity Create() { if (clearing) throw std::logic_error("Cannot create during scene clear"); return world.Create(); }
    void Create(ecs::Entity entity) { if (clearing) throw std::logic_error("Cannot create during scene clear"); world.Create(entity); }
    template<class T, class... Args> T &Attach(ecs::Entity entity, Args&&... args) {
        if (!world.IsAlive(entity) || IsPending(entity)) throw std::invalid_argument("Cannot attach to missing or destroying entity");
        auto value = std::make_unique<T>(std::forward<Args>(args)...);
        value->entity = entity; value->world = &world; value->owner = this;
        T &result = *value;
        entries.push_back(std::make_unique<Entry>(Entry{std::move(value)}));
        return result;
    }
    void Register(std::string typeId, std::function<std::unique_ptr<Behaviour>()> factory) {
        if (typeId.empty() || !factory || !factories.emplace(std::move(typeId), std::move(factory)).second)
            throw std::invalid_argument("Invalid or duplicate behaviour registration");
    }
    Behaviour &Attach(ecs::Entity entity, const std::string &typeId) {
        if (!world.IsAlive(entity) || IsPending(entity)) throw std::invalid_argument("Cannot attach to missing or destroying entity");
        const auto factory = factories.find(typeId);
        if (factory == factories.end()) throw std::invalid_argument("Unregistered behaviour type");
        auto value = factory->second();
        if (!value) throw std::invalid_argument("Behaviour factory returned null");
        value->entity = entity; value->world = &world; value->owner = this;
        auto &result = *value;
        entries.push_back(std::make_unique<Entry>(Entry{std::move(value)}));
        return result;
    }
    ecs::Entity GetParent(ecs::Entity entity) const {
        const auto found = parents.find(entity);
        if (found == parents.end()) return ecs::InvalidEntity;
        const auto &link = found->second;
        return world.Revision(entity) == link.childRevision &&
               world.Revision(link.parent) == link.parentRevision ? link.parent : ecs::InvalidEntity;
    }
    std::vector<ecs::Entity> GetChildren(ecs::Entity entity) const {
        std::vector<ecs::Entity> result;
        for (const auto &[child, link] : parents)
            if (GetParent(child) == entity && entity != ecs::InvalidEntity) result.push_back(child);
        std::ranges::sort(result);
        return result;
    }
    void SetParent(ecs::Entity child, ecs::Entity parent = ecs::InvalidEntity) {
        if (!world.IsAlive(child) || IsPending(child) ||
            (parent && (!world.IsAlive(parent) || IsPending(parent))))
            throw std::invalid_argument("Cannot reparent missing or destroying objects");
        for (auto ancestor = parent; ancestor; ancestor = GetParent(ancestor))
            if (ancestor == child) throw std::invalid_argument("Scene hierarchy cannot contain cycles");
        if (!parent) parents.erase(child);
        else parents[child] = {parent, world.Revision(child), world.Revision(parent)};
    }
    bool IsActiveSelf(ecs::Entity entity) const { return world.IsAlive(entity) && !inactive.contains(entity); }
    bool IsActive(ecs::Entity entity) const {
        if (!world.IsAlive(entity)) return false;
        for (auto current = entity; current; current = GetParent(current))
            if (inactive.contains(current)) return false;
        return true;
    }
    bool HasStartedBehaviour(ecs::Entity entity) const {
        for(const auto &entry:entries)if(entry->value->GetEntity()==entity && entry->started)return true;
        return false;
    }
    void SetActive(ecs::Entity entity, bool active) {
        if (!world.IsAlive(entity) || IsPending(entity)) throw std::invalid_argument("Object does not exist or is being destroyed");
        if (active) inactive.erase(entity); else inactive.insert(entity);
    }
    void SetEnabled(Behaviour &value, bool enabled) {
        for (auto &entry : entries) if (entry->value.get() == &value) entry->enabled = enabled;
    }
    void Destroy(ecs::Entity entity) {
        QueueSubtree(entity);
        if (!dispatching && !flushing) Flush();
    }
    // Runtime services emit after finishing storage traversal. Dispatch defers destruction.
    void NotifyContact(ecs::Entity entity, const CollisionEvent2D &event, bool trigger, bool entering) {
        Notify(entity,[&](Behaviour &value){
            if(trigger){if(entering)value.OnTriggerEnter(event);else value.OnTriggerExit(event);}
            else {if(entering)value.OnCollisionEnter(event);else value.OnCollisionExit(event);}
        });
    }
    void NotifyVisibility(ecs::Entity entity,bool visible) {
        Notify(entity,[&](Behaviour &value){if(visible)value.OnBecameVisible();else value.OnBecameInvisible();});
    }
    void Update(float delta) { Dispatch(delta, false); }
    void FixedUpdate(float delta) { Dispatch(delta, true); }
    void Clear() {
        if (dispatching) throw std::logic_error("Cannot clear scene during lifecycle dispatch");
        if (flushing) throw std::logic_error("Cannot clear scene during destruction callbacks");
        // Keep callback-created objects from escaping a full scene clear.
        clearing = true;
        for (const auto entity : world.Entities()) QueueSubtree(entity);
        try { Flush(); } catch (...) { clearing = false; throw; }
        world.Clear(); inactive.clear(); parents.clear(); clearing = false;
    }
private:
    template<class Callback> void Notify(ecs::Entity entity,Callback callback) {
        if(dispatching || flushing)throw std::logic_error("Recursive lifecycle notification");
        if(!world.IsAlive(entity)||!IsActive(entity)||IsPending(entity))return;
        dispatching=true;
        try {
            const auto count=entries.size();
            for(std::size_t i=0;i<count;++i){auto &entry=*entries[i];
                if(entry.value->GetEntity()==entity && entry.started && entry.enabled && IsActive(entity) && !IsPending(entity))callback(*entry.value);
            }
        } catch(...){dispatching=false;Flush();throw;}
        dispatching=false;Flush();
    }
    void Dispatch(float delta, bool fixed) {
        if (!std::isfinite(delta))throw std::invalid_argument("Lifecycle delta must be finite");
        if (delta <= 0) return;
        if (dispatching || flushing) throw std::logic_error("Recursive lifecycle dispatch");
        dispatching = true;
        try {
            const auto count = entries.size();
            for (std::size_t i = 0; i < count; ++i) {
                auto &entry = *entries[i];
                const auto entity = entry.value->GetEntity();
                if (pendingIds.contains(entity)) continue;
                const bool shouldRun = entry.enabled && IsActive(entity);
                if (shouldRun != entry.active) {
                    entry.active = shouldRun;
                    if (entry.active) entry.value->OnEnable(); else entry.value->OnDisable();
                }
                if (!entry.active || !entry.enabled || !IsActive(entity) || IsPending(entity)) continue;
                if (!entry.started) { entry.started = true; entry.value->Start(); }
                if (!entry.enabled || !IsActive(entity) || IsPending(entity)) continue;
                if (fixed) {
                    entry.value->FixedUpdate(delta);
                    if(entry.enabled && IsActive(entity) && !IsPending(entity))entry.value->AdvanceCoroutines(delta,[&]{return entry.enabled && IsActive(entity) && !IsPending(entity);});
                } else entry.value->Update(delta);
            }
        } catch (...) { dispatching = false; Flush(); throw; }
        dispatching = false;
        Flush();
    }
    bool IsPending(ecs::Entity entity) const {
        return pendingIds.contains(entity) || destroying == entity;
    }
    void QueueSubtree(ecs::Entity root) {
        if (!world.IsAlive(root) || IsPending(root)) return;
        std::vector<std::pair<ecs::Entity, bool>> stack{{root, false}};
        while (!stack.empty()) {
            const auto [entity, visited] = stack.back(); stack.pop_back();
            if (IsPending(entity)) continue;
            if (visited) { pending.push_back(entity); pendingIds.insert(entity); continue; }
            stack.emplace_back(entity, true);
            const auto children = GetChildren(entity);
            for (auto it = children.rbegin(); it != children.rend(); ++it) stack.emplace_back(*it, false);
        }
    }
    void Flush() {
        if (flushing) return;
        flushing = true;
        std::exception_ptr failure;
        while (!pending.empty()) {
            const auto entity = pending.front();
            pending.pop_front();
            pendingIds.erase(entity);
            destroying = entity;
            // Detach entries before invoking user code: callbacks may append
            // scripts or queue destruction without invalidating our iteration.
            std::vector<std::unique_ptr<Entry>> detached;
            for (auto &entry : entries)
                if (entry->value->GetEntity() == entity) detached.push_back(std::move(entry));
            std::erase_if(entries, [](const auto &entry) { return !entry; });
            for (auto &entry : detached) {
                try { if (entry->active) entry->value->OnDisable(); }
                catch (...) { if (!failure) failure = std::current_exception(); }
                try { entry->value->OnDestroy(); }
                catch (...) { if (!failure) failure = std::current_exception(); }
            }
            world.Destroy(entity);
            inactive.erase(entity);
            parents.erase(entity);
            destroying = ecs::InvalidEntity;
        }
        flushing = false;
        if (failure) std::rethrow_exception(failure);
    }
    std::unordered_map<std::string, std::function<std::unique_ptr<Behaviour>()>> factories;
    ServiceRegistry services; // Borrowed scene-local services outlive attached behaviours.
    ecs::World world;
    std::vector<std::unique_ptr<Entry>> entries;
    std::deque<ecs::Entity> pending;
    std::unordered_set<ecs::Entity> pendingIds;
    friend class SceneObject;
    std::shared_ptr<int> lifetime = std::make_shared<int>(0);
    bool dispatching{}, flushing{}, clearing{};
    ecs::Entity destroying{};
    std::unordered_set<ecs::Entity> inactive;
    struct ParentLink { ecs::Entity parent; std::uint64_t childRevision, parentRevision; };
    std::unordered_map<ecs::Entity, ParentLink> parents;
};
template<class T> T *Behaviour::GetService() const { return owner ? owner->Services().Find<T>() : nullptr; }

// Non-owning authoring facade. Every access validates scene lifetime and the
// entity incarnation, including after reset or explicit numeric-ID reuse.
class SceneObject {
public:
    bool operator==(const SceneObject &other) const {
        return scene==other.scene && entity==other.entity && revision==other.revision;
    }

    SceneObject() = default;
    bool IsValid() const {
        return !lifetime.expired() && scene->Components().Revision(entity) == revision && revision != 0;
    }
    std::uint64_t StructureRevision() const { RequireValid(); return scene->Components().StructureRevision(); }
    ecs::Entity GetEntity() const { RequireValid(); return entity; }
    template<class T> T *GetComponent() const {
        RequireValid(); return scene->Components().Get<T>(entity);
    }
    template<class T, class... Args> T &AddComponent(Args&&... args) const {
        RequireValid();
        if (scene->IsPending(entity)) throw std::logic_error("Object is being destroyed");
        return scene->Components().Add<T>(entity, std::forward<Args>(args)...);
    }
    template<class T, class... Args> T &Attach(Args&&... args) const {
        RequireValid(); return scene->Attach<T>(entity, std::forward<Args>(args)...);
    }
    SceneObject GetParent() const {
        RequireValid(); const auto parent = scene->GetParent(entity);
        return parent ? scene->GetObject(parent) : SceneObject{};
    }
    std::vector<SceneObject> GetChildren() const {
        RequireValid(); std::vector<SceneObject> result;
        for (auto child : scene->GetChildren(entity)) result.push_back(scene->GetObject(child));
        return result;
    }
    void SetParent(const SceneObject &parent) const {
        RequireValid(); parent.RequireValid();
        if (scene != parent.scene) throw std::invalid_argument("Parent must belong to the same scene");
        scene->SetParent(entity, parent.entity);
    }
    void DetachFromParent() const { RequireValid(); scene->SetParent(entity); }
    bool IsActiveSelf() const { RequireValid(); return scene->IsActiveSelf(entity); }
    bool IsActive() const { RequireValid(); return scene->IsActive(entity); }
    void SetActive(bool active) const { RequireValid(); scene->SetActive(entity, active); }
    void Destroy() const { RequireValid(); scene->Destroy(entity); }
private:
    friend class BehaviourScene;
    template<class...> friend class ComponentView;
    std::uint64_t StorageVersion() const {
        if (lifetime.expired()) throw std::logic_error("Scene no longer exists");
        return scene->Components().StructureRevision();
    }
    SceneObject(BehaviourScene &owner, ecs::Entity id)
        : scene(&owner), entity(id), revision(owner.Components().Revision(id)), lifetime(owner.lifetime) {}
    void RequireValid() const {
        if (!IsValid()) throw std::logic_error("Scene object no longer exists");
    }
    BehaviourScene *scene{};
    ecs::Entity entity{};
    std::uint64_t revision{};
    std::weak_ptr<int> lifetime;
};
inline SceneObject Behaviour::GetObject() const {
    if (!owner) throw std::logic_error("Behaviour is not attached to a scene");
    return owner->GetObject(entity);
}
inline SceneObject BehaviourScene::Instantiate(const EntityArchetype &recipe) {
    if (clearing) throw std::logic_error("Cannot instantiate during scene clear");
    const auto object = GetObject(recipe.Instantiate(world));
    try {
        recipe.OnInstantiated(object);
        if (!object.IsValid()) throw std::logic_error("Recipe destroyed its instantiated object");
    }
    catch (...) { if (object.IsValid()) object.Destroy(); throw; }
    return object;
}
inline SceneObject BehaviourScene::CreateObject() { return GetObject(Create()); }
inline SceneObject BehaviourScene::GetObject(ecs::Entity entity) {
    if (!world.IsAlive(entity)) throw std::invalid_argument("Scene object does not exist");
    return SceneObject(*this, entity);
}
} // namespace pipeframe
#endif
