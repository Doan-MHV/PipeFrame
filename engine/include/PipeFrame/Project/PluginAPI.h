#ifndef PIPEFRAME_PROJECT_PLUGIN_API_H
#define PIPEFRAME_PROJECT_PLUGIN_API_H

#include <PipeFrame/Core/DeterministicRandom.h>
#include <PipeFrame/Core/ServiceRegistry.h>
#include <PipeFrame/Core/Profiler.h>
#include <PipeFrame/Project/ProjectTypes.h>
#include <PipeFrame/Render/RenderTypes.h>

#include <memory>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace pipeframe {
class BrushTool;

// Version 4 replaces native camera/render-context layout with a RenderSurface contract.
// Rebuild project libraries before loading them into this host.
// Version 5 adds the opaque declarative dashboard and neutral controller APIs.
// Version 6 separates declarative presenters from native panel containers.
// Version 13 adds Behaviour coroutine storage and service-driven lifecycle callbacks.
// Version 12 retains collision workspace in PhysicsWorld2D (layout change).
// Version 11 adds scene-local Behaviour services and the environment query interface.
// Version 10 adds environment geometry identity to shared query hits.
// Version 9 adds executable environment-brush factories to plugin extensions.
inline constexpr std::uint32_t ProjectPluginAbiVersion = 13;

struct ProjectPluginDescriptor {
    std::string id;
    std::string displayName;
    std::uint32_t abiVersion{ProjectPluginAbiVersion};
    std::uint32_t pluginVersion{1};
    std::uint32_t stateSchemaVersion{1};
    bool usesSystemScheduler{false};
};

enum class ExtensionPoint : std::uint8_t {
    Panel, Drawer, Tool, Gizmo, Menu, Command, Overlay, Importer, Setting,
    PartCategory, AttachmentCompatibility, ConnectionType, EnvironmentBrush,
    ValidationRule, TelemetryStream, SimulationDebugOverlay,
};

struct InspectorPresentationContext {
    SceneObjectId objectId{};
    std::string componentTypeId;
    std::string propertyKey;
    PropertyValue value{0.0};
    bool mixed{false};
    bool runtimeValue{false};
};

struct InspectorPresentation {
    std::string valueText;
    std::string detail;
    std::optional<double> normalizedValue;
    Color accent{74, 163, 255, 255};
};

using InspectorPresentationCallback =
    std::function<std::optional<InspectorPresentation>(const InspectorPresentationContext &)>;

struct ExtensionInvocation {
    std::string context;
    std::string argument;
};
using ExtensionCallback = std::function<void(const ExtensionInvocation &)>;

struct ExtensionDescriptor {
    std::string id;
    std::string owner;
    std::string displayName;
    ExtensionPoint point{ExtensionPoint::Panel};
    std::string context;
    std::vector<std::string> tags;
    std::uint32_t stateVersion{1};
    InspectorPresentationCallback inspect;
    ExtensionCallback invoke;
    std::function<std::shared_ptr<BrushTool>()> createBrush;
};

class ExtensionRegistry final {
public:
    bool Register(ExtensionDescriptor descriptor, std::string *error = nullptr);
    bool RemoveOwner(std::string_view owner);
    const ExtensionDescriptor *Find(std::string_view id) const;
    std::vector<const ExtensionDescriptor *> FindByPoint(ExtensionPoint point) const;
    bool Invoke(std::string_view id, const ExtensionInvocation &invocation = {},
                std::string *error = nullptr) const;
    std::span<const ExtensionDescriptor> All() const { return extensions; }
    std::vector<std::string> Validate() const;
    void Clear() { extensions.clear(); }
private:
    std::vector<ExtensionDescriptor> extensions;
};

struct ActionInvocation { std::string argument; };
using ActionCallback = std::function<void(const ActionInvocation &)>;

struct EditorActionDescriptor {
    std::string id;
    std::string owner;
    std::string displayName;
    std::string category;
    std::vector<std::string> contexts;
    std::string defaultShortcut;
    ActionCallback invoke;
};

class ActionRegistry final {
public:
    bool Register(EditorActionDescriptor descriptor, std::string *error = nullptr);
    bool Rebind(std::string_view id, std::string shortcut, std::string *error = nullptr);
    bool Invoke(std::string_view id, const ActionInvocation &invocation = {},
                std::string *error = nullptr) const;
    std::vector<const EditorActionDescriptor *> Search(std::string_view query,
                                                        std::string_view context = {}) const;
    std::vector<std::string> Conflicts() const;
    std::span<const EditorActionDescriptor> All() const { return actions; }
    void Clear() { actions.clear(); }
private:
    std::vector<EditorActionDescriptor> actions;
};

enum class SystemPhase : std::uint8_t {
    Load, Start, FixedPrePhysics, FixedPhysics, FixedPostPhysics,
    FixedBehavior, FixedCleanup, VariableUpdate, RenderPrepare, Render, Editor,
};

struct StructuralCommand {
    enum class Kind : std::uint8_t { CreateObject, DestroyObject, ReplaceObject, SetComponentEnabled };
    Kind kind{Kind::CreateObject};
    SceneObjectId objectId{};
    SceneObjectData object;
    std::string componentTypeId;
    bool enabled{true};
};

class StructuralCommandBuffer final {
public:
    void Create(SceneObjectData object);
    void Destroy(SceneObjectId id);
    void Replace(SceneObjectData object);
    void SetComponentEnabled(SceneObjectId id, std::string componentTypeId, bool enabled);
    std::vector<StructuralCommand> Consume();
    std::span<const StructuralCommand> Pending() const { return commands; }
private:
    std::vector<StructuralCommand> commands;
};

class ComponentQuery {
public:
    virtual ~ComponentQuery() = default;
    virtual std::span<SceneObjectData> Objects() = 0;
    virtual std::span<const SceneObjectData> Objects() const = 0;
    SceneObjectData *Find(SceneObjectId id);
    const SceneObjectData *Find(SceneObjectId id) const;
    std::vector<SceneObjectData *> With(std::string_view componentTypeId);
};


inline constexpr std::string_view SpatialServiceId="pipeframe.spatial-2d";
inline constexpr std::string_view PhysicsServiceId="pipeframe.physics-2d";
inline constexpr std::string_view ResourceServiceId="pipeframe.resources";
inline constexpr std::string_view RenderServiceId="pipeframe.render-2d";
inline constexpr std::string_view EventServiceId="pipeframe.events";
inline constexpr std::string_view RandomServiceId="pipeframe.random";
inline constexpr std::string_view JobServiceId="pipeframe.jobs";
inline constexpr std::string_view LogServiceId="pipeframe.log";
inline constexpr std::string_view ProfilingServiceId="pipeframe.profiling";

struct ProjectEvent { std::string type; std::string payload; };
class EventQueue final {
public:
    void Publish(ProjectEvent event) { events.push_back(std::move(event)); }
    std::vector<ProjectEvent> Consume() { auto result=std::move(events); events.clear(); return result; }
    std::span<const ProjectEvent> Pending() const { return events; }
private:
    std::vector<ProjectEvent> events;
};

class DeterministicJobQueue final {
public:
    void Submit(std::uint64_t orderKey, std::function<void()> job);
    void Execute();
    std::size_t Size() const { return jobs.size(); }
private:
    struct Job { std::uint64_t orderKey{}; std::uint64_t sequence{}; std::function<void()> run; };
    std::vector<Job> jobs;
    std::uint64_t nextSequence{};
};

class ProjectLogSink {
public:
    virtual ~ProjectLogSink() = default;
    virtual void Write(std::string_view category, std::string_view message) = 0;
};

class RenderSubmissionQueue final {
public:
    void Submit(GeometryCommand command) { commands.push_back(std::move(command)); }
    std::vector<GeometryCommand> Consume() { auto result=std::move(commands); commands.clear(); return result; }
    std::span<const GeometryCommand> Pending() const { return commands; }
private:
    std::vector<GeometryCommand> commands;
};

struct SystemContext {
    float deltaTime{};
    std::uint64_t tick{};
    ComponentQuery *components{};
    StructuralCommandBuffer *commands{};
    ServiceRegistry *services{};
    EventQueue *events{};
    RenderSubmissionQueue *render{};
    DeterministicRandom *random{};
    Profiler *profiler{};
    DeterministicJobQueue *jobs{};
    ProjectLogSink *log{};
};

using SystemCallback = std::function<void(SystemContext &)>;
struct ProjectSystemDescriptor {
    std::string id;
    std::string owner;
    SystemPhase phase{SystemPhase::FixedBehavior};
    std::vector<std::string> dependencies;
    std::vector<std::string> reads;
    std::vector<std::string> writes;
    bool parallelSafe{false};
    bool editorOnly{false};
    bool usesStructuralCommands{false};
    SystemCallback execute;
    std::vector<std::string> requiredServices;
};

class SystemRegistry final {
public:
    bool Register(ProjectSystemDescriptor descriptor, std::string *error = nullptr);
    bool RemoveOwner(std::string_view owner);
    const ProjectSystemDescriptor *Find(std::string_view id) const;
    std::span<const ProjectSystemDescriptor> All() const { return systems; }
    std::vector<std::string> Validate() const;
    std::vector<const ProjectSystemDescriptor *> Ordered(SystemPhase phase,
                                                         std::string *error = nullptr) const;
    bool Execute(SystemPhase phase, SystemContext &context, std::string *error = nullptr) const;
    void Clear() { systems.clear(); }
private:
    std::vector<ProjectSystemDescriptor> systems;
};

class PluginRegistrar final {
public:
    PluginRegistrar(std::string owner, ExtensionRegistry &extensions, ActionRegistry &actions,
                    SystemRegistry &systems)
        : owner(std::move(owner)), extensions(extensions), actions(actions), systems(systems) {}
    bool Extension(ExtensionDescriptor descriptor, std::string *error = nullptr);
    bool Action(EditorActionDescriptor descriptor, std::string *error = nullptr);
    bool System(ProjectSystemDescriptor descriptor, std::string *error = nullptr);
private:
    std::string owner;
    ExtensionRegistry &extensions;
    ActionRegistry &actions;
    SystemRegistry &systems;
};

struct ProjectReloadState {
    std::uint32_t schemaVersion{1};
    std::vector<std::uint8_t> payload;
};

} // namespace pipeframe

#endif
