#pragma once
#include <PipeFrame/Components/CommonComponentSchemas.h>
#include <PipeFrame/Project/EntityRegistry.h>
#include <PipeFrame/Project/ProjectRuntime.h>
#include <functional>
#include <string>
#include <vector>

namespace pipeframe {
// Configuration contains types and unbound callbacks, never a live World pointer.
// Keep the completed definition immutable. World owners provide the current instance
// on every dispatch and bind entity factories again when constructing a new scene.
template<class World>
class ProjectDefinition {
public:
    struct PhaseEntry {
        ProjectSystemDescriptor descriptor;
        std::function<void(World&, float)> update;
    };
    template<class Panel> ProjectDefinition& UI(ui::PanelOptions placement = {}) {
        static_assert(std::is_base_of_v<ui::ViewPanel, Panel>, "Project UI panels must derive from ViewPanel");
        panels.push_back([placement = std::move(placement)](World& world, ui::ProjectUIHost& host) {
            if constexpr (std::is_constructible_v<Panel, World&, ui::ProjectUIHost&>)
                host.Add(std::make_unique<Panel>(world, host), placement);
            else if constexpr (std::is_constructible_v<Panel, World&>)
                host.Add(std::make_unique<Panel>(world), placement);
            else if constexpr (std::is_constructible_v<Panel, ui::ProjectUIHost&>)
                host.Add(std::make_unique<Panel>(host), placement);
            else
                host.Add(std::make_unique<Panel>(), placement);
        });
        return *this;
    }
    void InstallUI(World& world, ui::ProjectUIHost& host) const {
        for (const auto& panel : panels) panel(world, host);
    }
    template<class... T> ProjectDefinition& Components() {
        (AddComponent<T>(false), ...); return *this;
    }
    // Legacy object-property authoring menu; all registered components remain
    // discoverable through their schemas in the component Inspector.
    template<class... T> ProjectDefinition& AuthoringComponents() {
        (AddComponent<T>(true), ...); return *this;
    }
    ProjectDefinition& CommonComponents() {
        registrations.push_back([](auto& components, auto&) { RegisterCommonComponents(components); });
        return *this;
    }
    template<class T> ProjectDefinition& Entity(SceneObjectTypeDescriptor descriptor, bool editorVisible = true) {
        registrations.push_back([descriptor = std::move(descriptor), editorVisible](auto&, auto& entities) {
            entities.template Register<T>(descriptor, editorVisible);
        });
        return *this;
    }
    template<class T> ProjectDefinition& Behaviour(std::string id, std::string name) {
        registrations.push_back([id = std::move(id), name = std::move(name)](auto& components, auto& entities) {
            entities.template RegisterBehaviour<T>(components, id, name);
        });
        return *this;
    }
    template<class T> ProjectDefinition& Brush(std::string category = {}) {
        extensions.push_back([category = std::move(category)](ProjectRuntime& runtime) {
            runtime.template RegisterBrush<T>(category);
        });
        return *this;
    }
    template<class Factory, class Restored>
    ProjectDefinition& EntityFactory(SceneObjectTypeId id, Factory create, Restored restored) {
        factories.push_back([id = std::move(id), create, restored](EntityRegistry& registry, World& world) {
            registry.BindFactory(id,
                [&world, create](BehaviourScene& scene, const SceneObjectData& data) { return create(world, scene, data); },
                [&world, restored](SceneObject object, const SceneObjectData& data) { restored(world, object, data); });
        });
        return *this;
    }
    template<class Configure> ProjectDefinition& ConfigureWorld(Configure configure) {
        initializers.emplace_back(std::move(configure)); return *this;
    }
    ProjectDefinition& FixedStep() { return *this; }
    template<class Update> ProjectDefinition& Phase(std::string id, Update update) {
        return Phase(ProjectSystemDescriptor{.id=std::move(id)}, std::move(update));
    }
    template<class Update> ProjectDefinition& Phase(ProjectSystemDescriptor descriptor, Update update) {
        if (descriptor.id.empty() || std::ranges::any_of(phases, [&](const auto& phase) { return phase.descriptor.id == descriptor.id; }))
            throw std::invalid_argument("Empty or duplicate project phase");
        if (descriptor.execute) throw std::invalid_argument("Phase callback belongs in the project phase update argument");
        phases.push_back({std::move(descriptor), std::move(update)}); return *this;
    }
    // Source-generator bridge: legacy generated headers use these spellings.
    // They populate this same definition rather than another registration path.
    template<class T> void RegisterComponent() { AuthoringComponents<T>(); }
    template<class T> void RegisterEntity(SceneObjectTypeDescriptor descriptor) { Entity<T>(std::move(descriptor)); }
    template<class T> void RegisterBehaviour(std::string id, std::string name) { Behaviour<T>(std::move(id), std::move(name)); }
    template<class T> void RegisterBrush(std::string category = "Project") { Brush<T>(std::move(category)); }
    void Install(ComponentRegistry& components, EntityRegistry& entities) const {
        auto nextComponents = components;
        auto nextEntities = entities;
        for (const auto& registration : registrations) registration(nextComponents, nextEntities);
        components = std::move(nextComponents); entities = std::move(nextEntities);
    }
    void InstallExtensions(ProjectRuntime& runtime) const { for (const auto& extension : extensions) extension(runtime); }
    void InitializeWorld(World& world) const { for (const auto& initialize : initializers) initialize(world); }
    void BindFactories(EntityRegistry& entities, World& world) const { for (const auto& bind : factories) bind(entities, world); }
    void ExecutePhase(std::size_t index, World& world, float delta) const { phases.at(index).update(world, delta); }
    const auto& Phases() const { return phases; }
    const auto& AuthoringTypes() const { return authoringTypes; }
private:
    template<class T> void AddComponent(bool authoring) {
        registrations.push_back([](auto& components, auto&) { components.Register(T::Schema()); });
        if (authoring) authoringTypes.push_back(T::Schema().Describe());
    }
    std::vector<std::function<void(World&,ui::ProjectUIHost&)>> panels;
    std::vector<std::function<void(ComponentRegistry&, EntityRegistry&)>> registrations;
    std::vector<std::function<void(ProjectRuntime&)>> extensions;
    std::vector<std::function<void(EntityRegistry&, World&)>> factories;
    std::vector<std::function<void(World&)>> initializers;
    std::vector<PhaseEntry> phases;
    std::vector<SceneComponentTypeDescriptor> authoringTypes;
};
} // namespace pipeframe
