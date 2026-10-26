#include <PipeFrame/Project/PluginAPI.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <ranges>
#include <unordered_set>

namespace pipeframe {
namespace {
bool Fail(std::string *error, std::string message) {
    if (error)
        *error = std::move(message);
    return false;
}
std::string Lower(std::string_view value) {
    std::string result(value);
    std::ranges::transform(result, result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}
bool SharesContext(const EditorActionDescriptor &a, const EditorActionDescriptor &b) {
    if (a.contexts.empty() || b.contexts.empty())
        return true;
    return std::ranges::any_of(
        a.contexts, [&](const auto &context) { return std::ranges::find(b.contexts, context) != b.contexts.end(); });
}
bool IsRenderPhase(SystemPhase phase) { return phase == SystemPhase::RenderPrepare || phase == SystemPhase::Render; }
int PhaseRank(SystemPhase phase) { return static_cast<int>(phase); }
} // namespace

bool ExtensionRegistry::Register(ExtensionDescriptor descriptor, std::string *error) {
    if (descriptor.id.empty() || descriptor.owner.empty() || descriptor.displayName.empty())
        return Fail(error, "Extension ID, owner, and display name are required.");
    if (descriptor.createBrush && descriptor.point != ExtensionPoint::EnvironmentBrush)
        return Fail(error, "Brush factory requires EnvironmentBrush extension point");
    if (descriptor.inspect && descriptor.point != ExtensionPoint::Drawer)
        return Fail(error, "Inspector presentation callbacks require the Drawer extension point.");
    if (!descriptor.inspect && !descriptor.invoke && !descriptor.createBrush)
        return Fail(error, "Extension must provide an executable callback: " + descriptor.id);
    if (Find(descriptor.id))
        return Fail(error, "Extension ID is already registered: " + descriptor.id);
    extensions.push_back(std::move(descriptor));
    return true;
}
bool ExtensionRegistry::RemoveOwner(std::string_view owner) {
    return std::erase_if(extensions, [&](const auto &item) { return item.owner == owner; }) > 0;
}
const ExtensionDescriptor *ExtensionRegistry::Find(std::string_view id) const {
    const auto it = std::ranges::find(extensions, id, &ExtensionDescriptor::id);
    return it == extensions.end() ? nullptr : &*it;
}
std::vector<const ExtensionDescriptor *> ExtensionRegistry::FindByPoint(ExtensionPoint point) const {
    std::vector<const ExtensionDescriptor *> result;
    for (const auto &item : extensions)
        if (item.point == point)
            result.push_back(&item);
    return result;
}
bool ExtensionRegistry::Invoke(std::string_view id, const ExtensionInvocation &invocation, std::string *error) const {
    const auto *extension = Find(id);
    if (!extension || !extension->invoke)
        return Fail(error, "Extension is not executable: " + std::string(id));
    try {
        extension->invoke(invocation);
        return true;
    } catch (const std::exception &exception) {
        return Fail(error, extension->id + " failed: " + exception.what());
    } catch (...) {
        return Fail(error, extension->id + " failed with an unknown exception.");
    }
}
std::vector<std::string> ExtensionRegistry::Validate() const {
    std::vector<std::string> errors;
    std::unordered_set<std::string> ids;
    for (const auto &item : extensions) {
        if (item.id.empty() || item.owner.empty() || item.displayName.empty())
            errors.push_back("Extension has incomplete identity.");
        if (!item.inspect && !item.invoke && !item.createBrush)
            errors.push_back("Extension has no executable callback: " + item.id);
        if (!ids.insert(item.id).second)
            errors.push_back("Duplicate extension ID: " + item.id);
    }
    return errors;
}

bool ActionRegistry::Register(EditorActionDescriptor descriptor, std::string *error) {
    if (descriptor.id.empty() || descriptor.owner.empty() || descriptor.displayName.empty() || !descriptor.invoke)
        return Fail(error, "Action identity and callback are required.");
    if (std::ranges::find(actions, descriptor.id, &EditorActionDescriptor::id) != actions.end())
        return Fail(error, "Action ID is already registered: " + descriptor.id);
    actions.push_back(std::move(descriptor));
    return true;
}
bool ActionRegistry::Rebind(std::string_view id, std::string shortcut, std::string *error) {
    auto found = std::ranges::find(actions, id, &EditorActionDescriptor::id);
    if (found == actions.end())
        return Fail(error, "Action is not registered.");
    const auto previous = found->defaultShortcut;
    found->defaultShortcut = std::move(shortcut);
    if (!Conflicts().empty()) {
        found->defaultShortcut = previous;
        return Fail(error, "Shortcut conflicts in an active context.");
    }
    return true;
}
bool ActionRegistry::Invoke(std::string_view id, const ActionInvocation &invocation, std::string *error) const {
    const auto found = std::ranges::find(actions, id, &EditorActionDescriptor::id);
    if (found == actions.end())
        return Fail(error, "Action is not registered: " + std::string(id));
    try {
        found->invoke(invocation);
        return true;
    } catch (const std::exception &exception) {
        return Fail(error, found->id + " failed: " + exception.what());
    } catch (...) {
        return Fail(error, found->id + " failed with an unknown exception.");
    }
}
std::vector<const EditorActionDescriptor *> ActionRegistry::Search(std::string_view query,
                                                                   std::string_view context) const {
    const auto needle = Lower(query);
    std::vector<const EditorActionDescriptor *> result;
    for (const auto &action : actions) {
        const bool contextMatch = context.empty() || action.contexts.empty() ||
                                  std::ranges::find(action.contexts, std::string(context)) != action.contexts.end();
        const auto haystack = Lower(action.displayName + " " + action.category + " " + action.id);
        if (contextMatch && haystack.find(needle) != std::string::npos)
            result.push_back(&action);
    }
    std::ranges::sort(result, {}, [](const auto *item) { return item->displayName; });
    return result;
}
std::vector<std::string> ActionRegistry::Conflicts() const {
    std::vector<std::string> result;
    for (std::size_t a = 0; a < actions.size(); ++a)
        for (std::size_t b = a + 1; b < actions.size(); ++b)
            if (!actions[a].defaultShortcut.empty() && actions[a].defaultShortcut == actions[b].defaultShortcut &&
                SharesContext(actions[a], actions[b]))
                result.push_back(actions[a].id + " conflicts with " + actions[b].id + " on " +
                                 actions[a].defaultShortcut);
    return result;
}

void StructuralCommandBuffer::Create(SceneObjectData object) {
    commands.push_back({StructuralCommand::Kind::CreateObject, object.id, std::move(object)});
}
void StructuralCommandBuffer::Destroy(SceneObjectId id) {
    commands.push_back({StructuralCommand::Kind::DestroyObject, id});
}
void StructuralCommandBuffer::Replace(SceneObjectData object) {
    commands.push_back({StructuralCommand::Kind::ReplaceObject, object.id, std::move(object)});
}
void StructuralCommandBuffer::SetComponentEnabled(SceneObjectId id, std::string componentTypeId, bool enabled) {
    StructuralCommand command;
    command.kind = StructuralCommand::Kind::SetComponentEnabled;
    command.objectId = id;
    command.componentTypeId = std::move(componentTypeId);
    command.enabled = enabled;
    commands.push_back(std::move(command));
}
std::vector<StructuralCommand> StructuralCommandBuffer::Consume() {
    auto result = std::move(commands);
    commands.clear();
    return result;
}

SceneObjectData *ComponentQuery::Find(SceneObjectId id) {
    auto objects = Objects();
    const auto found = std::ranges::find(objects, id, &SceneObjectData::id);
    return found == objects.end() ? nullptr : &*found;
}
const SceneObjectData *ComponentQuery::Find(SceneObjectId id) const {
    auto objects = Objects();
    const auto found = std::ranges::find(objects, id, &SceneObjectData::id);
    return found == objects.end() ? nullptr : &*found;
}
std::vector<SceneObjectData *> ComponentQuery::With(std::string_view type) {
    std::vector<SceneObjectData *> result;
    for (auto &object : Objects())
        if (std::ranges::find(object.components, type, &SceneComponentData::typeId) != object.components.end())
            result.push_back(&object);
    return result;
}

void DeterministicJobQueue::Submit(std::uint64_t key, std::function<void()> job) {
    if (job)
        jobs.push_back({key, nextSequence++, std::move(job)});
}
void DeterministicJobQueue::Execute() {
    std::ranges::sort(jobs, [](const auto &a, const auto &b) {
        return a.orderKey == b.orderKey ? a.sequence < b.sequence : a.orderKey < b.orderKey;
    });
    auto pending = std::move(jobs);
    jobs.clear();
    for (auto &job : pending)
        job.run();
}

bool SystemRegistry::Register(ProjectSystemDescriptor descriptor, std::string *error) {
    if (descriptor.id.empty() || descriptor.owner.empty() || !descriptor.execute)
        return Fail(error, "System ID, owner, and callback are required.");
    if (Find(descriptor.id))
        return Fail(error, "System ID is already registered: " + descriptor.id);
    systems.push_back(std::move(descriptor));
    return true;
}
bool SystemRegistry::RemoveOwner(std::string_view owner) {
    return std::erase_if(systems, [&](const auto &item) { return item.owner == owner; }) > 0;
}
const ProjectSystemDescriptor *SystemRegistry::Find(std::string_view id) const {
    const auto found = std::ranges::find(systems, id, &ProjectSystemDescriptor::id);
    return found == systems.end() ? nullptr : &*found;
}
std::vector<std::string> SystemRegistry::Validate() const {
    std::vector<std::string> errors;
    for (const auto &system : systems) {
        std::unordered_set<std::string> required;
        for (const auto &service : system.requiredServices)
            if (service.empty() || !required.insert(service).second)
                errors.push_back(system.id + " has an empty or duplicate service requirement.");
        if (system.editorOnly && system.phase != SystemPhase::Editor)
            errors.push_back(system.id + " is editor-only but uses a runtime phase.");
        if (system.usesStructuralCommands && IsRenderPhase(system.phase))
            errors.push_back(system.id + " requests structural changes during rendering.");
        for (const auto &dependency : system.dependencies) {
            const auto *other = Find(dependency);
            if (!other)
                errors.push_back(system.id + " depends on missing system " + dependency + ".");
            else if (!system.editorOnly && other->editorOnly)
                errors.push_back(system.id + " runtime system depends on editor-only system " + dependency + ".");
            else if (PhaseRank(other->phase) > PhaseRank(system.phase))
                errors.push_back(system.id + " depends on a later lifecycle phase " + dependency + ".");
        }
    }
    const auto dependsOn = [&](const ProjectSystemDescriptor &source, const std::string &target) {
        std::unordered_set<std::string> seen;
        std::function<bool(const ProjectSystemDescriptor &)> visit = [&](const auto &current) {
            if (!seen.insert(current.id).second)
                return false;
            for (const auto &id : current.dependencies) {
                if (id == target)
                    return true;
                if (const auto *dependency = Find(id); dependency && visit(*dependency))
                    return true;
            }
            return false;
        };
        return visit(source);
    };
    for (std::size_t a = 0; a < systems.size(); ++a)
        for (std::size_t b = a + 1; b < systems.size(); ++b) {
            if (systems[a].phase != systems[b].phase || dependsOn(systems[a], systems[b].id) ||
                dependsOn(systems[b], systems[a].id))
                continue;
            for (const auto &write : systems[a].writes)
                if (std::ranges::find(systems[b].writes, write) != systems[b].writes.end())
                    errors.push_back(systems[a].id + " and " + systems[b].id + " write " + write +
                                     " without an ordering dependency.");
        }
    for (const auto &system : systems) {
        std::unordered_set<std::string> active, done;
        std::function<bool(const ProjectSystemDescriptor &)> visit = [&](const auto &current) {
            if (active.contains(current.id))
                return false;
            if (done.contains(current.id))
                return true;
            active.insert(current.id);
            for (const auto &id : current.dependencies)
                if (const auto *dependency = Find(id); dependency && !visit(*dependency))
                    return false;
            active.erase(current.id);
            done.insert(current.id);
            return true;
        };
        if (!visit(system)) {
            errors.push_back("System dependency cycle includes " + system.id + ".");
            break;
        }
    }
    return errors;
}
std::vector<const ProjectSystemDescriptor *> SystemRegistry::Ordered(SystemPhase phase, std::string *error) const {
    const auto errors = Validate();
    if (!errors.empty()) {
        Fail(error, errors.front());
        return {};
    }
    std::vector<const ProjectSystemDescriptor *> result;
    std::unordered_set<std::string> emitted;
    std::function<void(const ProjectSystemDescriptor &)> emit = [&](const auto &system) {
        if (emitted.contains(system.id))
            return;
        for (const auto &id : system.dependencies)
            if (const auto *dependency = Find(id); dependency && dependency->phase == phase)
                emit(*dependency);
        emitted.insert(system.id);
        if (system.phase == phase)
            result.push_back(&system);
    };
    for (const auto &system : systems)
        if (system.phase == phase)
            emit(system);
    return result;
}
bool SystemRegistry::Execute(SystemPhase phase, SystemContext &context, std::string *error) const {
    const auto ordered = Ordered(phase, error);
    if (!Validate().empty())
        return false;
    for (const auto *system : ordered) {
        if ((!system->reads.empty() || !system->writes.empty()) && !context.components)
            return Fail(error, system->id + " requires a component query.");
        if (system->usesStructuralCommands && !context.commands)
            return Fail(error, system->id + " requires a structural command buffer.");
        for (const auto &service : system->requiredServices) {
            const bool available =
                context.services && context.services->Contains(service) &&
                (service != EventServiceId || context.events) && (service != RandomServiceId || context.random) &&
                (service != JobServiceId || context.jobs) && (service != LogServiceId || context.log) &&
                (service != ProfilingServiceId || context.profiler) && (service != RenderServiceId || context.render);
            if (!available)
                return Fail(error, system->id + " requires unavailable service " + service + ".");
        }
        try {
            if (context.profiler) {
                ProfileScope scope(*context.profiler, system->id);
                system->execute(context);
            } else
                system->execute(context);
            if (context.jobs)
                context.jobs->Execute();
        } catch (const std::exception &exception) {
            return Fail(error, system->id + " failed: " + exception.what());
        } catch (...) {
            return Fail(error, system->id + " failed with an unknown exception.");
        }
    }
    return true;
}

bool PluginRegistrar::Extension(ExtensionDescriptor descriptor, std::string *error) {
    descriptor.owner = owner;
    return extensions.Register(std::move(descriptor), error);
}
bool PluginRegistrar::Action(EditorActionDescriptor descriptor, std::string *error) {
    descriptor.owner = owner;
    return actions.Register(std::move(descriptor), error);
}
bool PluginRegistrar::System(ProjectSystemDescriptor descriptor, std::string *error) {
    descriptor.owner = owner;
    return systems.Register(std::move(descriptor), error);
}

} // namespace pipeframe
