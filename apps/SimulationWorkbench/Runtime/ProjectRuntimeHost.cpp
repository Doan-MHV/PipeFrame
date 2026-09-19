#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include "ProjectRuntimeHost.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <utility>

#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/GeometryRenderer.h>

namespace pipeframe::editor {
namespace {
bool ValidateRegistration(const ProjectPluginDescriptor &descriptor,
                          const ExtensionRegistry &extensions,
                          const ActionRegistry &actions,
                          const SystemRegistry &systems,
                          const ServiceRegistry *services,
                          std::string *error) {
    const auto fail=[&](std::string message){if(error)*error=std::move(message);return false;};
    if(descriptor.id.empty()||descriptor.displayName.empty())return fail("Project plugin identity is incomplete.");
    if(descriptor.abiVersion!=ProjectPluginAbiVersion)return fail("Project plugin ABI is incompatible with this Workbench.");
    if(descriptor.pluginVersion==0||descriptor.stateSchemaVersion==0)return fail("Project plugin and state-schema versions must be non-zero.");
    if(descriptor.usesSystemScheduler&&systems.All().empty())return fail("Scheduled project plugins must register at least one system.");
    if(!descriptor.usesSystemScheduler&&!systems.All().empty())return fail("A project registering systems must enable scheduler ownership.");
    if(const auto values=extensions.Validate();!values.empty())return fail(values.front());
    if(const auto values=actions.Conflicts();!values.empty())return fail(values.front());
    if(const auto values=systems.Validate();!values.empty())return fail(values.front());
    if(services)for(const auto &system:systems.All())for(const auto &service:system.requiredServices)
        if(!services->Contains(service))return fail(system.id+" requires unavailable host service "+service+".");
    return true;
}
}

ProjectRuntimeHost::~ProjectRuntimeHost() { Unload(); }

void ProjectRuntimeHost::RegisterHostServices(){
    services.Clear();
    if(assetDatabase)services.Provide(*assetDatabase);
    spatialIndex.Initialize({{-4096.0f,-4096.0f},{8192.0f,8192.0f}},64.0f);
    services.Provide(spatialIndex,std::string(SpatialServiceId));
    services.Provide(physicsWorld,std::string(PhysicsServiceId));
    services.Provide(resources,std::string(ResourceServiceId));
    services.Provide(surfaces,std::string(RenderServiceId));
    services.Provide(events,std::string(EventServiceId));
    services.Provide(random,std::string(RandomServiceId));
    services.Provide(jobs,std::string(JobServiceId));
    services.Provide(log,std::string(LogServiceId));
    services.Provide(profiler,std::string(ProfilingServiceId));
    services.Provide(renderSubmissions);
}

bool ProjectRuntimeHost::ExecutePhase(const SystemPhase phase,SystemContext &context){
    std::string error;if(systems.Execute(phase,context,&error))return true;
    lastPluginError=std::move(error);log.Write("plugin",lastPluginError);return false;
}

bool ProjectRuntimeHost::Load(const std::filesystem::path &projectDirectory,
                              const std::filesystem::path &runtimeLibrary, std::string *errorMessage) {

    Unload();

    if (runtimeLibrary.empty()) {
        return true;
    }

    const std::filesystem::path requestedPath =
        runtimeLibrary.is_absolute() ? runtimeLibrary : projectDirectory / runtimeLibrary;

    if (!library.Load(requestedPath, errorMessage)) {

        return false;
    }

    runtime = library.GetRuntime();

    if (runtime == nullptr) {
        SetError(errorMessage, "The project runtime library "
                               "returned no runtime.");

        library.Unload();
        return false;
    }

    this->projectDirectory=projectDirectory;
    pluginDescriptor=runtime->GetPluginDescriptor();
    if(pluginDescriptor.abiVersion!=ProjectPluginAbiVersion){
        SetError(errorMessage,"Project plugin ABI is incompatible. Rebuild the project with this PipeFrame SDK.");
        library.Unload();runtime=nullptr;return false;
    }
    extensions.Clear();actions.Clear();systems.Clear();RegisterHostServices();lastPluginError.clear();
    PluginRegistrar registrar(pluginDescriptor.id,extensions,actions,systems);
    std::string registrationError;
    if(!runtime->RegisterPlugin(registrar,registrationError)||!runtime->RegisterEnvironmentBrushes(registrar,registrationError)||
       !ValidateRegistration(pluginDescriptor,extensions,actions,systems,&services,&registrationError)){
        SetError(errorMessage,registrationError.empty()?"Project plugin registration failed.":registrationError);
        extensions.Clear();actions.Clear();systems.Clear();library.Unload();runtime=nullptr;return false;
    }

    ProjectRuntimeContext context{.projectDirectory = projectDirectory,.services=&services,
        .events=&events,.renderSubmissions=&renderSubmissions};

    std::string runtimeError;

    if (!runtime->Load(context, runtimeError)) {

        SetError(errorMessage, runtimeError.empty() ? "The project runtime "
                                                      "failed to load."
                                                    : std::move(runtimeError));

        runtime->Unload();extensions.Clear();actions.Clear();systems.Clear();library.Unload();runtime = nullptr;

        return false;
    }

    runtimeLoaded = true;
    componentTypes = runtime->GetAllSceneComponentTypes();
    SystemContext systemContext{.services=&services,.events=&events,.render=&renderSubmissions,
        .random=&random,.profiler=&profiler,.jobs=&jobs,.log=&log};
    if(!systems.Execute(SystemPhase::Load,systemContext,errorMessage)){Unload();return false;}
    return true;
}

bool ProjectRuntimeHost::Reload(const std::span<const SceneObjectData> objects,
                                const std::optional<SceneObjectId> selection,
                                std::string *errorMessage) {
    return ReloadFrom(library.GetLibraryPath(),objects,selection,errorMessage);
}

bool ProjectRuntimeHost::ReloadFrom(const std::filesystem::path &candidateLibrary,
                                    const std::span<const SceneObjectData> objects,
                                    const std::optional<SceneObjectId> selection,
                                    std::string *errorMessage) {
    if(!HasRuntime())return false;
    const ProjectReloadState state=runtime->CaptureReloadState();
    const auto source=ProjectRuntimeLibrary::ResolveLibraryPath(candidateLibrary);
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory=std::filesystem::temp_directory_path()/"pipeframe-hot-reload";
    std::error_code filesystemError;std::filesystem::create_directories(directory,filesystemError);
    auto shadow=directory/(pluginDescriptor.id+"-"+std::to_string(stamp)+source.extension().string());
    std::filesystem::copy_file(source,shadow,std::filesystem::copy_options::overwrite_existing,filesystemError);
    if(filesystemError){SetError(errorMessage,"Could not stage the rebuilt runtime: "+filesystemError.message());return false;}

    ProjectRuntimeLibrary candidate;
    if(!candidate.Load(shadow,errorMessage)){std::filesystem::remove(shadow);return false;}
    ProjectRuntime *next=candidate.GetRuntime();
    auto nextDescriptor=next->GetPluginDescriptor();
    if(nextDescriptor.abiVersion!=ProjectPluginAbiVersion){
        SetError(errorMessage,"Project plugin ABI is incompatible. Rebuild the project with this PipeFrame SDK.");
        candidate.Unload();std::filesystem::remove(shadow);return false;
    }
    ExtensionRegistry nextExtensions;ActionRegistry nextActions;SystemRegistry nextSystems;
    PluginRegistrar registrar(nextDescriptor.id,nextExtensions,nextActions,nextSystems);
    std::string error;
    if(nextDescriptor.id!=pluginDescriptor.id||nextDescriptor.stateSchemaVersion!=state.schemaVersion||
       !next->RegisterPlugin(registrar,error)||!next->RegisterEnvironmentBrushes(registrar,error)||
       !ValidateRegistration(nextDescriptor,nextExtensions,nextActions,nextSystems,&services,&error)){
        SetError(errorMessage,error.empty()?"Reloaded plugin identity or state schema is incompatible.":error);
        nextExtensions.Clear();nextActions.Clear();nextSystems.Clear();candidate.Unload();std::filesystem::remove(shadow);return false;
    }
    ProjectRuntimeContext runtimeContext{.projectDirectory=projectDirectory,.services=&services,
        .events=&events,.renderSubmissions=&renderSubmissions};
    if(!next->Load(runtimeContext,error)){SetError(errorMessage,error);nextExtensions.Clear();nextActions.Clear();nextSystems.Clear();candidate.Unload();std::filesystem::remove(shadow);return false;}
    StructuralCommandBuffer stagedCommands;EventQueue stagedEvents;RenderSubmissionQueue stagedRender;
    DeterministicRandom stagedRandom(1);Profiler stagedProfiler;DeterministicJobQueue stagedJobs;
    SystemContext stagedContext{.components=&componentQuery,.commands=&stagedCommands,.services=&services,
        .events=&stagedEvents,.render=&stagedRender,.random=&stagedRandom,.profiler=&stagedProfiler,
        .jobs=&stagedJobs,.log=&log};
    if(!nextSystems.Execute(SystemPhase::Load,stagedContext,&error)){
        next->Unload();nextExtensions.Clear();nextActions.Clear();nextSystems.Clear();candidate.Unload();std::filesystem::remove(shadow);
        SetError(errorMessage,error);return false;
    }
    next->SynchronizeScene(objects);next->SetSelectedObject(selection);next->SetViewMode(viewMode);
    if(previewActive){
        next->Reset();next->Start();
        if(!nextSystems.Execute(SystemPhase::Start,stagedContext,&error)){
            next->Stop();next->Unload();nextExtensions.Clear();nextActions.Clear();nextSystems.Clear();candidate.Unload();std::filesystem::remove(shadow);
            SetError(errorMessage,error);return false;
        }
        if(!previewRunning)next->Stop();
    }
    if(!next->RestoreReloadState(state,error)){
        next->Unload();nextExtensions.Clear();nextActions.Clear();nextSystems.Clear();candidate.Unload();std::filesystem::remove(shadow);
        SetError(errorMessage,error.empty()?"Reloaded plugin rejected its saved state.":error);return false;
    }

    if(releaseBrushes)releaseBrushes();
    if(previewActive&&previewRunning)runtime->Stop();
    runtime->Unload();
    // std::function managers for registered actions and systems live in the
    // owning plugin. Destroy them while that library is still resident.
    extensions.Clear();actions.Clear();systems.Clear();
    const auto oldShadow=shadowLibraryPath;
    library.Swap(candidate);candidate.Unload();
    runtime=library.GetRuntime();pluginDescriptor=std::move(nextDescriptor);
    extensions=std::move(nextExtensions);actions=std::move(nextActions);systems=std::move(nextSystems);
    componentTypes=runtime->GetAllSceneComponentTypes();componentQuery.Assign(objects);
    selectedObject=selection;shadowLibraryPath=shadow;
    if(!oldShadow.empty())std::filesystem::remove(oldShadow,filesystemError);
    if(loadBrushes)loadBrushes();
    return true;
}

void ProjectRuntimeHost::Unload() {
    if(releaseBrushes)releaseBrushes();
    if (runtime != nullptr) {
        if (previewActive && previewRunning) {
            runtime->Stop();
        }

        if (runtimeLoaded) {
            runtime->Unload();
        }
    }

    previewActive = false;
    previewRunning = false;
    runtimeLoaded = false;
    runtime = nullptr;
    componentTypes.clear();
    extensions.Clear();actions.Clear();systems.Clear();services.Clear();resources.Clear();physicsWorld.Clear();log.Clear();
    componentQuery.Assign({});commands.Consume();events.Consume();renderSubmissions.Consume();screenRenderSubmissions.clear();
    pluginDescriptor={};tick=0;selectedObject.reset();

    library.Unload();
    if(!shadowLibraryPath.empty()){std::error_code error;std::filesystem::remove(shadowLibraryPath,error);shadowLibraryPath.clear();}
}

bool ProjectRuntimeHost::HasRuntime() const { return runtime != nullptr && runtimeLoaded; }

bool ProjectRuntimeHost::IsPreviewActive() const { return previewActive; }

std::span<const SceneObjectTypeDescriptor> ProjectRuntimeHost::GetSceneObjectTypes() const {

    if (runtime == nullptr) {
        return {};
    }

    return runtime->GetSceneObjectTypes();
}

std::span<const SceneComponentTypeDescriptor> ProjectRuntimeHost::GetSceneComponentTypes() const {
    return componentTypes;
}

SceneObjectData ProjectRuntimeHost::CreateDefaultObject(const SceneObjectTypeId &typeId) const {

    if (runtime == nullptr) {
        SceneObjectData object;

        object.name = typeId;
        object.typeId = typeId;

        return object;
    }

    return runtime->CreateDefaultObject(typeId);
}

void ProjectRuntimeHost::SynchronizeScene(std::span<const SceneObjectData> objects) {

    componentQuery.Assign(objects);
    std::vector<UniformSpatialIndex<SceneObjectId>::Entry> entries;entries.reserve(objects.size());
    if(!objects.empty()){
        Vector2f minimum=objects.front().transform.position,maximum=minimum;
        for(const auto &object:objects){minimum.x=std::min(minimum.x,object.transform.position.x);minimum.y=std::min(minimum.y,object.transform.position.y);maximum.x=std::max(maximum.x,object.transform.position.x);maximum.y=std::max(maximum.y,object.transform.position.y);}
        constexpr float margin=512.0f;spatialIndex.Initialize({{minimum.x-margin,minimum.y-margin},
            {std::max(1024.0f,maximum.x-minimum.x+margin*2),std::max(1024.0f,maximum.y-minimum.y+margin*2)}},64.0f);
    }
    for(const auto &object:objects)entries.push_back({object.id,object.transform.position});
    spatialIndex.Rebuild(entries);
    if (runtime != nullptr) {
        try {runtime->SynchronizeScene(objects);}
        catch(const std::exception &error){lastPluginError=error.what();log.Write("scene",lastPluginError);return;}
        if (previewActive) {
            if (previewRunning) runtime->Start(); else runtime->Stop();
        }
    }
}

bool ProjectRuntimeHost::ApplyComponentEdits(
    const std::span<const ProjectRuntimeComponentEdit> edits) {
    if (runtime == nullptr || edits.empty()) return false;
    const auto state = !previewActive
                           ? ProjectRuntimeAuthoringState::Stopped
                           : previewRunning ? ProjectRuntimeAuthoringState::Playing
                                            : ProjectRuntimeAuthoringState::Paused;
    return runtime->ApplyComponentEdits(edits, state);
}

std::vector<ProjectRuntimeComponentEdit> ProjectRuntimeHost::GetLiveComponentProperties() const {
    return runtime ? runtime->GetLiveComponentProperties()
                   : std::vector<ProjectRuntimeComponentEdit>{};
}

void ProjectRuntimeHost::SetSelectedObject(std::optional<SceneObjectId> objectId) {

    selectedObject=objectId;
    if (runtime != nullptr) {
        runtime->SetSelectedObject(objectId);
    }
}

void ProjectRuntimeHost::SetViewMode(const ProjectRuntimeViewMode mode) {
    viewMode=mode;
    if (runtime != nullptr) {
        runtime->SetViewMode(mode);
    }
}

std::optional<SceneObjectId> ProjectRuntimeHost::HitTest(const Vector2f worldPosition) const {

    if (runtime == nullptr) {
        return std::nullopt;
    }

    return runtime->HitTest(worldPosition);
}

ProjectRuntimeStatistics ProjectRuntimeHost::GetStatistics() const {
    if (runtime == nullptr) {
        return {};
    }

    return runtime->GetStatistics();
}

void ProjectRuntimeHost::BeginPreview() {
    if (runtime == nullptr || previewActive) {
        return;
    }

    runtime->Reset();
    runtime->Start();

    SystemContext context{.components=&componentQuery,.commands=&commands,.services=&services,
        .events=&events,.render=&renderSubmissions,.random=&random,.profiler=&profiler,.jobs=&jobs,.log=&log};
    ExecutePhase(SystemPhase::Start,context);

    previewActive = true;
    previewRunning = true;
}

void ProjectRuntimeHost::PausePreview() {
    if (
        runtime == nullptr ||
        !previewActive ||
        !previewRunning
    ) {
        return;
    }

    runtime->Stop();

    previewRunning = false;
}

void ProjectRuntimeHost::ResumePreview() {
    if (
        runtime == nullptr ||
        !previewActive ||
        previewRunning
    ) {
        return;
    }

    runtime->Start();

    previewRunning = true;
}

void ProjectRuntimeHost::EndPreview() {
    if (
        runtime == nullptr ||
        !previewActive
    ) {
        return;
    }

    if (previewRunning) {
        runtime->Stop();
    }

    previewActive = false;
    previewRunning = false;
}

void ProjectRuntimeHost::ResetPreview() {
    if (runtime == nullptr) {
        return;
    }

    runtime->Reset();

    previewActive = false;
    previewRunning = false;
}

bool ProjectRuntimeHost::HandleUIEvent(const InputEvent &event, RenderContext &context) {
    return runtime && runtime->HandleUIEvent(event,context);
}
bool ProjectRuntimeHost::HasUIFocus() const { return runtime && runtime->HasUIFocus(); }
bool ProjectRuntimeHost::HasWorldPointerCapture() const { return runtime && runtime->HasWorldPointerCapture(); }
bool ProjectRuntimeHost::UsesRightClickTool() const { return runtime && runtime->UsesRightClickTool(); }

void ProjectRuntimeHost::HandleEvent(const InputEvent &event, RenderContext &context) {

    if (runtime != nullptr) {
        runtime->HandleEvent(event, context);
    }
}

void ProjectRuntimeHost::FixedUpdate(const float fixedDeltaTime) {

    if (runtime != nullptr && previewActive) {
        if(!pluginDescriptor.usesSystemScheduler){runtime->FixedUpdate(fixedDeltaTime);return;}
        SystemContext context{.deltaTime=fixedDeltaTime,.tick=++tick,.components=&componentQuery,
            .commands=&commands,.services=&services,.events=&events,.render=&renderSubmissions,
            .random=&random,.profiler=&profiler,.jobs=&jobs,.log=&log};
        for(const auto phase:{SystemPhase::FixedPrePhysics,SystemPhase::FixedPhysics,
            SystemPhase::FixedPostPhysics,SystemPhase::FixedBehavior,SystemPhase::FixedCleanup})
            if(!ExecutePhase(phase,context))break;
        events.Consume();
    }
}

void ProjectRuntimeHost::VariableUpdate(const float deltaTime){
    if(runtime==nullptr)return;
    SystemContext context{.deltaTime=deltaTime,.tick=tick,.components=&componentQuery,
        .commands=&commands,.services=&services,.events=&events,.render=&renderSubmissions,
        .random=&random,.profiler=&profiler,.jobs=&jobs,.log=&log};
    if(pluginDescriptor.usesSystemScheduler)ExecutePhase(SystemPhase::VariableUpdate,context);
    ExecutePhase(SystemPhase::Editor,context);
}

void ProjectRuntimeHost::Render(RenderContext &context) {

    if (runtime != nullptr) {
        if(!pluginDescriptor.usesSystemScheduler){runtime->Render(context);return;}
        services.Provide(context);
        SystemContext systemContext{.tick=tick,.components=&componentQuery,.commands=&commands,
            .services=&services,.events=&events,.render=&renderSubmissions,.random=&random,
            .profiler=&profiler,.jobs=&jobs,.log=&log};
        ExecutePhase(SystemPhase::RenderPrepare,systemContext);
        ExecutePhase(SystemPhase::Render,systemContext);
        screenRenderSubmissions.clear();
        for(auto &command:renderSubmissions.Consume()){
            if(command.space==RenderSpace::World)backend::sfml::DrawGeometry(pipeframe::backend::sfml::GetTarget(context),command);
            else screenRenderSubmissions.push_back(std::move(command));
        }
    }
}

void ProjectRuntimeHost::RenderDebug(RenderContext &context, WorldDebugOptions options) {
    if(!runtime || (!options.physics&&!options.mesh))return;
    WorldDebugDraw draw;runtime->CollectWorldDebug(draw,options);draw.Draw(context.GetCanvas());
}

void ProjectRuntimeHost::RenderScreen(RenderContext &context) {
    for(const auto &command:screenRenderSubmissions)backend::sfml::DrawGeometry(pipeframe::backend::sfml::GetTarget(context),command);
    screenRenderSubmissions.clear();
    if (runtime != nullptr) {
        runtime->RenderScreen(context);
    }
}

bool ProjectRuntimeHost::ConsumesPointerAt(const Vector2i screenPosition,
                                           const RenderContext &context) const {
    return runtime != nullptr && runtime->ConsumesPointerAt(screenPosition, context);
}

std::vector<ProjectRuntimeSceneEdit> ProjectRuntimeHost::ConsumeSceneEdits() {
    auto result=runtime!=nullptr?runtime->ConsumeSceneEdits():std::vector<ProjectRuntimeSceneEdit>{};
    for(auto &command:commands.Consume()){
        ProjectRuntimeSceneEdit edit;
        switch(command.kind){
        case StructuralCommand::Kind::CreateObject:edit.kind=ProjectRuntimeSceneEditKind::CreateObject;edit.object=std::move(command.object);break;
        case StructuralCommand::Kind::DestroyObject:edit.kind=ProjectRuntimeSceneEditKind::RemoveObject;edit.object.id=command.objectId;break;
        case StructuralCommand::Kind::ReplaceObject:edit.kind=ProjectRuntimeSceneEditKind::ReplaceObject;edit.object=std::move(command.object);break;
        case StructuralCommand::Kind::SetComponentEnabled:edit.kind=ProjectRuntimeSceneEditKind::SetComponentEnabled;edit.object.id=command.objectId;edit.componentTypeId=std::move(command.componentTypeId);edit.componentEnabled=command.enabled;break;
        }
        result.push_back(std::move(edit));
    }
    return result;
}

void ProjectRuntimeHost::SetError(std::string *errorMessage, std::string message) {

    if (errorMessage != nullptr) {
        *errorMessage = std::move(message);
    }
}

} // namespace pipeframe::editor
