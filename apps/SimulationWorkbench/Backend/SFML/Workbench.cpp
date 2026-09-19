#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <PipeFrame/Backend/SFML/RenderContextAdapter.h>
#include "../../Editor/ProjectBuildPlan.h"
#include "Workbench.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

#include "../../Editor/ProjectSession.h"
#include "../../Editor/ProjectScaffolder.h"
#include "../../Editor/WorkbenchInput.h"
#include "WorkbenchView.h"
#include "../../Platform/NativeProjectDialog.h"
#include "../../Runtime/SimulationSession.h"

#include <PipeFrame/Render/RenderContext.h>

namespace {

std::filesystem::path GetDefaultProjectsDirectory() {
    return std::filesystem::path(PIPEFRAME_ASSET_DIR).parent_path() / "projects";
}

void PrintError(const std::string &operation, const std::string &errorMessage) {

    std::cerr << operation << ": " << (errorMessage.empty() ? "Unknown error." : errorMessage) << '\n';
}

} // namespace

class Workbench::Implementation final {
  public:
    explicit Implementation(std::optional<std::filesystem::path> startupProject)
        : startupProject(std::move(startupProject)), project(GetDefaultProjectsDirectory()),
          simulation(project.GetRuntime()), input(view, project, simulation) {}

    void Load() {
        const std::filesystem::path fontPath =
            std::filesystem::path(PIPEFRAME_ASSET_DIR) / "fonts/roboto_mono_semi.ttf";

        if (!view.Load(fontPath)) {
            return;
        }

        ConfigureViewCallbacks();
        view.SetWorkspaceLayoutPath(GetDefaultProjectsDirectory() / ".workbench.layout");

        input.SetOnStateChanged([this](const bool rebuildHierarchy) { Refresh(rebuildHierarchy); });

        std::string errorMessage;

        if (!project.OpenStartupProject(startupProject, &errorMessage)) {

            PrintError("Unable to open project", errorMessage);

            ShowProjectBrowser(errorMessage);
        } else if (project.HasProject()) {
            ShowWorkspace();
        } else {
            ShowProjectBrowser({});
        }

        loaded = true;
    }

    void Start() {}

    void OnResize(const sf::Vector2u newSize, RenderContext &context) {

        if (!loaded) {
            return;
        }

        view.Layout(newSize, context);

        layoutInitialized = true;
    }

    void HandleEvent(const sf::Event &event, RenderContext &context) {

        if (!loaded) {
            return;
        }

        if (const auto neutral = pipeframe::backend::sfml::FromBackend(event)) input.HandleEvent(*neutral, context);
    }

    void FixedUpdate(const float fixedDeltaTime) {

        if (!project.HasProject() || !view.IsWorkspaceVisible()) {

            return;
        }

        simulation.FixedUpdate(fixedDeltaTime);
    }

    void Update(const float deltaTime) {
        if (!loaded) {
            return;
        }

        if(project.GetAssetDatabase().PumpOperations())Refresh(false);
        PollBuild(deltaTime);
        view.Update(deltaTime);

        if (project.HasProject() && view.IsWorkspaceVisible()) {

            project.GetRuntime().VariableUpdate(deltaTime);

            Refresh(false);
        }
    }

    void Render(RenderContext &context) {
        if (!loaded) {
            return;
        }

        if (!layoutInitialized) {
            view.Layout(pipeframe::backend::sfml::GetWindow(context).getSize(), context);

            layoutInitialized = true;
        }

        if (project.HasProject() && view.IsWorkspaceVisible()) {

            context.BeginWorld();

            view.RenderWorldBackground(context);

            project.GetRuntime().Render(context);
            project.GetRuntime().RenderDebug(context, view.GetWorldDebugOptions());

            project.RenderTilemapEditing(context);
            view.RenderWorldOverlay(context);
        }

        if (project.HasProject() && view.IsWorkspaceVisible()) {
            context.BeginScreen();
            project.GetRuntime().RenderScreen(context);
        }

        view.RenderScreen(context, simulation);
    }

    unsigned int GetFrameRateLimit() const {
        return simulation.IsPlaying() && simulation.IsFullSpeed() ? 0u : 60u;
    }

    float GetSimulationTimeScale() const {
        return simulation.GetController().GetTimeScale();
    }

    bool UseMaximumSimulationRate() const {
        return simulation.IsPlaying() && simulation.IsFullSpeed();
    }

    void Stop() {
        input.CancelDrag();
        simulation.Stop();
    }

    void Unload() {
        simulation.Stop();
        project.GetRuntime().Unload();
    }

  private:
    void ConfigureViewCallbacks() {
        pipeframe::editor::WorkbenchView::Callbacks callbacks;

        callbacks.newProject = [this]() { CreateNewProject(); };

        callbacks.openProject = [this]() { OpenProjectDialog(); };

        callbacks.openRecentProject = [this](const std::filesystem::path &path) { OpenProject(path); };

        callbacks.save = [this]() { SaveProject(); };

        callbacks.saveAs = [this]() { SaveProjectAsCopy(); };

        callbacks.undo = [this]() { Undo(); };

        callbacks.redo = [this]() { Redo(); };

        callbacks.buildRuntime=[this](){StartBuild();};
        callbacks.reloadRuntime = [this]() {
            if(buildPending)return;
            std::string error;
            if(!project.GetRuntime().Reload(project.GetDocument().GetObjects(),project.GetSelectedObjectId(),&error))
                PrintError("Unable to reload project runtime",error);
            Refresh(true);
        };

        callbacks.createObjectOfType=[this](const pipeframe::SceneObjectTypeId &typeId,std::optional<pipeframe::Vector2f> position) {
            if(!simulation.CanAuthorScene() || !project.HasProject())return;
            if(!project.CreateObjectOfType(typeId,position))PrintError("Unable to create object","Object type is no longer available.");
            Refresh(true);
        };
        callbacks.addObject = [this]() { AddObject(); };
        callbacks.placeObject = [this](const pipeframe::Vector2f position) { AddObject(position); };

        callbacks.deleteObject = [this]() { DeleteObject(); };

        callbacks.createPrefab = [this]() {
            const auto *selected = project.GetSelectedObject();
            if (!selected) return;
            std::string error;
            const std::string id = "prefab_" + std::to_string(selected->id);
            if (!project.CreatePrefabFromSelected(id, &error)) PrintError("Unable to create prefab", error);
            Refresh(false);
        };

        callbacks.applyPrefab = [this]() {
            std::string error;
            if (!project.ApplySelectedPrefab(&error)) PrintError("Unable to apply prefab", error);
            Refresh(true);
        };

        callbacks.revertPrefab = [this]() {
            std::string error;
            if (!project.RevertSelectedPrefab(&error)) PrintError("Unable to revert prefab", error);
            Refresh(true);
        };

        callbacks.unpackPrefab = [this]() {
            std::string error;
            if (!project.UnpackSelectedPrefab(&error)) PrintError("Unable to unpack prefab", error);
            Refresh(true);
        };

        callbacks.selectionChanged = [this](const pipeframe::SceneObjectId objectId) {
            project.SetSelectedObject(objectId);

            Refresh(false);
        };

        callbacks.toggleSimulation = [this]() {
            if(!simulation.IsPlaying())project.GetTilemapEditor().SetEnabled(false);
            simulation.Toggle(project.GetDocument().GetObjects());

            Refresh(false);
        };

        callbacks.singleStep = [this]() {
            project.GetTilemapEditor().SetEnabled(false);
            simulation.RequestSingleStep(project.GetDocument().GetObjects());

            Refresh(false);
        };

        callbacks.resetSimulation = [this]() {
            simulation.Reset(project.GetDocument().GetObjects());

            Refresh(false);
        };

        callbacks.setSimulationSpeed = [this](const SimulationSpeed speed) {
            simulation.SetSpeed(speed);
            Refresh(false);
        };

        callbacks.workspaceModeChanged = [this](const pipeframe::editor::WorkbenchWorkspaceMode mode) {
            using EditorMode = pipeframe::editor::WorkbenchWorkspaceMode;
            using RuntimeMode = pipeframe::ProjectRuntimeViewMode;

            switch (mode) {
            case EditorMode::Editor:
                project.GetRuntime().SetViewMode(RuntimeMode::Editor);
                break;
            case EditorMode::Simulation:
                project.GetRuntime().SetViewMode(RuntimeMode::Simulation);
                break;
            case EditorMode::Zen:
                project.GetRuntime().SetViewMode(RuntimeMode::Zen);
                break;
            }
        };

        callbacks.transformCommitted = [this](const pipeframe::SceneTransform &transform) {
            if (!simulation.CanAuthorScene()) {

                return;
            }

            if (project.SetSelectedTransform(transform)) {

                Refresh(false);
            }
        };

        callbacks.propertyCommitted = [this](const std::string &key, const pipeframe::PropertyValue &value) {
            if (!simulation.CanAuthorScene()) {

                return;
            }

            if (project.SetSelectedProperty(key, value)) {

                Refresh(false);
            }
        };

        callbacks.componentPropertyCommitted =
            [this](const std::string &componentTypeId, const std::string &key,
                   const pipeframe::PropertyValue &value) {
                if (!simulation.CanAuthorScene()) return;
                if (project.SetSelectedComponentProperty(componentTypeId, key, value)) Refresh(false);
            };

        callbacks.generateModule=[this](const std::string &kind,const std::string &name) -> std::string {
            if(!project.HasProject())return "Open a project first.";
            if(buildPending)return "Wait for the current build to finish.";
            if(!simulation.CanAuthorScene())return "Stop the simulation before generating source.";
            const auto type=kind=="Entity"?pipeframe::editor::GeneratedModuleKind::Entity:
                kind=="Component"?pipeframe::editor::GeneratedModuleKind::Component:
                kind=="Brush"?pipeframe::editor::GeneratedModuleKind::Brush:pipeframe::editor::GeneratedModuleKind::Behaviour;
            std::string error;
            if(!pipeframe::editor::ProjectScaffolder::AddModule(project.GetProjectDirectory(),type,name,&error))return error;
            if(kind=="Brush"){StartBuild();return "Created "+name+" and registered its brush. Build/reload progress appears in the editor.";}
            return "Created "+name+" and updated registration. Use BUILD & RELOAD to compile and load it.";
        };
        callbacks.componentAttachment=[this](const std::string &id,bool add) {
            if(!simulation.CanAuthorScene())return;
            const auto *type=project.FindComponentType(id);
            if(!type || !type->removable)return;
            pipeframe::SceneComponentData component;component.typeId=id;component.schemaVersion=type->schemaVersion;
            for(const auto &field:type->properties)component.properties.emplace(field.key,field.defaultValue);
            const bool changed=add?project.AddSelectedComponent(std::move(component)):project.RemoveSelectedComponent(id);
            if(!changed)PrintError("Unable to change component","Select an object and check its component requirements.");
            Refresh(true);
        };
        callbacks.assignAssetProperty=[this](const std::string &component,const std::string &key,const std::string &asset){
            if(!simulation.CanAuthorScene())return;
            if(!project.AssignAssetToSelectedProperty(component,key,asset))PrintError("Unable to assign asset","Select a compatible editable property.");
            Refresh(false);
        };
        callbacks.assignAsset = [this](const std::string &assetId) {
            if (!simulation.CanAuthorScene()) return;
            const auto *asset = project.GetAssetDatabase().Find(assetId);
            if (asset && asset->type == pipeframe::assets::AssetType::Prefab) {
                pipeframe::SceneTransform placement;
                const auto pointer = view.GetPointerWorldPosition();
                placement.position = {pointer.x, pointer.y};
                std::string error;
                if (!project.InstantiatePrefab(asset->sourcePath.stem().string(), 0, placement, &error))
                    PrintError("Unable to instantiate prefab", error);
                Refresh(true);
                return;
            }
            if (project.AssignAssetToFirstSelectedAssetProperty(assetId)) Refresh(false);
        };
        callbacks.visualAssets.create=[this](pipeframe::assets::AssetType type){if(!simulation.IsPlaying())project.GetVisualAssetEditor().Create(project.GetAssetDatabase(),type);Refresh(false);};
        callbacks.visualAssets.open=[this](const std::string &id){if(!simulation.IsPlaying())project.GetVisualAssetEditor().Open(project.GetAssetDatabase(),id);Refresh(false);};
        callbacks.visualAssets.set=[this](const std::string &key,const pipeframe::PropertyValue &value){if(!simulation.IsPlaying())project.GetVisualAssetEditor().Set(key,value);Refresh(false);};
        callbacks.visualAssets.save=[this]{if(!simulation.IsPlaying())project.GetVisualAssetEditor().Save();Refresh(false);};
        callbacks.visualAssets.undo=[this]{if(!simulation.IsPlaying())project.GetVisualAssetEditor().Undo();Refresh(false);};
        callbacks.visualAssets.redo=[this]{if(!simulation.IsPlaying())project.GetVisualAssetEditor().Redo();Refresh(false);};
        callbacks.visualAssets.tileset=[this](const std::string &id){if(!simulation.IsPlaying())project.GetTilemapEditor().AssignTileset(id);Refresh(false);};
        callbacks.visualAssets.previewResize=[this](int columns,int rows,float size,bool sample){if(!simulation.IsPlaying())project.GetTilemapEditor().PrepareResize(columns,rows,size,sample);Refresh(false);};
        callbacks.visualAssets.applyResize=[this]{if(!simulation.IsPlaying())project.GetTilemapEditor().ApplyResize();Refresh(false);};
        callbacks.makeMapUnique = [this] { if(!simulation.IsPlaying())project.MakeSelectedTilemapUnique(); Refresh(false); };
        callbacks.createMap = [this] { if(!simulation.IsPlaying())project.CreateTilemapAsset(); Refresh(false); };
        callbacks.editMap = [this](const std::string &id) { if(!simulation.IsPlaying())project.BeginTilemapEditing(id); Refresh(false); };
        callbacks.stopMap = [this] { project.GetTilemapEditor().SetEnabled(false); Refresh(false); };
        callbacks.environmentAssets.brush=[this](int radius,bool axis){if(!simulation.IsPlaying())project.GetTilemapEditor().SetBrushOptions(radius,axis);Refresh(false);};
        callbacks.environmentAssets.selectBrush=[this](const std::string &id){if(!simulation.IsPlaying())project.GetTilemapEditor().SelectBrush(id);Refresh(false);};
        callbacks.environmentAssets.brushSetting=[this](const std::string &key,const pipeframe::PropertyValue &value){if(!simulation.IsPlaying())project.GetTilemapEditor().SetBrushSetting(key,value);Refresh(false);};
        callbacks.environmentAssets.layer=[this](const std::string &action,std::size_t layer,const std::string &name){
            if(!simulation.IsPlaying())project.GetTilemapEditor().EditLayer(action,layer,name);Refresh(false);
        };
        callbacks.environmentAssets.tile=[this](pipeframe::TileId id,pipeframe::Color color,bool solid){if(!simulation.IsPlaying())project.GetTilemapEditor().DefineTile(id,color,solid);Refresh(false);};
        callbacks.environmentAssets.selection=[this](pipeframe::TileId tile,bool boundary){if(!simulation.IsPlaying())project.GetTilemapEditor().PaintSelection(tile,boundary);Refresh(false);};
        callbacks.environmentAssets.save=[this]{if(!simulation.IsPlaying())project.GetTilemapEditor().Save();Refresh(false);};
        callbacks.environmentAssets.undo=[this]{if(!simulation.IsPlaying())project.GetTilemapEditor().Undo();Refresh(false);};
        callbacks.environmentAssets.redo=[this]{if(!simulation.IsPlaying())project.GetTilemapEditor().Redo();Refresh(false);};
        callbacks.paintShape = [this](std::size_t layer,pipeframe::TileId tile,int shape) {
            if(simulation.IsPlaying())return;
            auto &editor=project.GetTilemapEditor();const auto *map=editor.Document();if(!map)return;
            editor.ConfigureGesture(layer,tile,
                static_cast<pipeframe::TilemapPaintShape>(shape));
            editor.SetEnabled(true); Refresh(false);
        };
        callbacks.discoverAssets = [this] {
            std::string error;
            project.GetAssetDatabase().DiscoverProjectAssets(&error);
            Refresh(false);
        };
        callbacks.importAsset = [this] {
            const auto source=pipeframe::editor::NativeProjectDialog::SelectAssetFile();
            if(!source)return;
            std::string error;
            project.GetAssetDatabase().QueueImport({*source});
            Refresh(false);
        };
        callbacks.reimportAsset = [this](const std::string &assetId) {
            std::string error;
            project.GetAssetDatabase().QueueReimport(assetId);
            Refresh(false);
        };
        callbacks.cancelAssetOperation = [this](const std::uint64_t operationId) {
            project.GetAssetDatabase().CancelOperation(operationId);
            Refresh(false);
        };

        view.SetCallbacks(std::move(callbacks));
    }

    void CreateNewProject() {
        if (!simulation.CanAuthorScene()) {
            return;
        }

        const auto directory = pipeframe::editor::NativeProjectDialog::SelectNewProjectDirectory();

        if (!directory.has_value()) {
            return;
        }

        std::string errorMessage;

        input.CancelDrag(); simulation.Stop();
        if (!project.CreateProjectAt(*directory, &errorMessage)) {

            ShowProjectBrowser(errorMessage);

            return;
        }

        ShowWorkspace();
    }

    void OpenProjectDialog() {
        if (!simulation.CanAuthorScene()) {
            return;
        }

        const auto directory = pipeframe::editor::NativeProjectDialog::SelectProjectDirectory();

        if (!directory.has_value()) {
            return;
        }

        OpenProject(*directory);
    }

    void OpenProject(const std::filesystem::path &path) {

        if (!simulation.CanAuthorScene()) {
            return;
        }

        std::string errorMessage;

        input.CancelDrag(); simulation.Stop();
        if (!project.OpenProject(path, &errorMessage)) {

            ShowProjectBrowser(errorMessage);

            return;
        }

        ShowWorkspace();
    }

    void ShowProjectBrowser(const std::string &message) {

        input.CancelDrag();
        simulation.Stop();

        view.ShowProjectBrowser(project.GetRecentProjects(), message);
    }

    void ShowWorkspace() {
        view.ShowWorkspace();

        using EditorMode = pipeframe::editor::WorkbenchWorkspaceMode;
        using RuntimeMode = pipeframe::ProjectRuntimeViewMode;

        switch (view.GetWorkspaceMode()) {
        case EditorMode::Editor:
            project.GetRuntime().SetViewMode(RuntimeMode::Editor);
            break;
        case EditorMode::Simulation:
            project.GetRuntime().SetViewMode(RuntimeMode::Simulation);
            break;
        case EditorMode::Zen:
            project.GetRuntime().SetViewMode(RuntimeMode::Zen);
            break;
        }

        Refresh(true);
    }

    void SaveProject() {
        if (!simulation.CanAuthorScene() || !project.HasProject()) {

            return;
        }

        std::string errorMessage;

        if (!project.Save(&errorMessage)) {

            PrintError("Unable to save project", errorMessage);

            return;
        }

        Refresh(false);
    }

    void SaveProjectAsCopy() {
        if (!simulation.CanAuthorScene() || !project.HasProject()) {

            return;
        }

        std::string errorMessage;

        if (!project.SaveAsCopy(&errorMessage)) {

            PrintError("Unable to save project copy", errorMessage);

            return;
        }

        Refresh(true);
    }

    void AddObject(std::optional<pipeframe::Vector2f> position = std::nullopt) {
        if (!simulation.CanAuthorScene() || !project.HasProject()) {

            return;
        }

        if (!project.CreateObject(position)) {
            PrintError("Unable to create object", "The active project does not "
                                                  "provide an object type.");

            return;
        }

        Refresh(true);
    }

    void DeleteObject() {
        if (!simulation.CanAuthorScene() || !project.HasProject()) {

            return;
        }

        if (project.DeleteSelectedObject()) {
            Refresh(true);
        }
    }

    void Undo() {
        if (!simulation.CanAuthorScene() || !project.HasProject()) {

            return;
        }

        if (project.Undo()) {
            Refresh(true);
        }
    }

    void Redo() {
        if (!simulation.CanAuthorScene() || !project.HasProject()) {

            return;
        }

        if (project.Redo()) {
            Refresh(true);
        }
    }

    void Refresh(const bool rebuildHierarchy) {

        if (!project.HasProject() || !view.IsWorkspaceVisible()) {

            return;
        }

        view.Refresh(project, simulation, rebuildHierarchy);
    }

    void StartBuild(){
        if(buildPending){buildTask.Cancel();return;}
        if(!project.HasProject())return;
        if(!simulation.CanAuthorScene()){view.SetBuildProgress(false,"Stop the simulation before building.");return;}
        try {
            buildPlan=pipeframe::editor::ProjectBuildPlan::Create(project.GetProjectDirectory(),PIPEFRAME_SDK_DIR,
                PIPEFRAME_HOST_BUILD_DIR,PIPEFRAME_HOST_CONFIGURATION,PIPEFRAME_CMAKE_COMMAND,PIPEFRAME_ENGINE_LIBRARY);
            std::string error;
            if(!buildTask.Start(buildPlan.commands,buildPlan.project,buildPlan.project/".pipeframe/build.log",error)){
                view.SetBuildProgress(false,error);return;
            }
            buildPending=true;buildPollTime=0;view.SetBuildProgress(true,"Configuring project...");
        }catch(const std::exception &error){view.SetBuildProgress(false,error.what());}
    }
    void PollBuild(float delta){
        if(!buildPending)return;
        buildPollTime+=delta;if(buildPollTime<.1f)return;buildPollTime=0;
        auto progress=buildTask.Poll();
        view.SetBuildProgress(progress.running,progress.stage+"\n"+progress.output);
        if(progress.running)return;
        buildPending=false;
        std::string result;
        if(progress.cancelled)result="Build cancelled. The loaded runtime was preserved.";
        else if(progress.exitCode)result="Build failed (exit "+std::to_string(progress.exitCode)+"). The loaded runtime was preserved.";
        else if(project.GetProjectDirectory()!=buildPlan.project)result="Build succeeded. Original project is no longer active; runtime was not loaded.";
        else {
            std::string error;
            if(project.LoadBuiltRuntime(buildPlan.library,&error)){result="Build and reload succeeded.";Refresh(true);}
            else result="Build succeeded; runtime activation failed: "+error;
        }
        view.SetBuildProgress(false,result+"\n"+progress.output);
    }
    pipeframe::ProcessTask buildTask;
    pipeframe::editor::ProjectBuildPlan buildPlan;
    bool buildPending{};
    float buildPollTime{};

    std::optional<std::filesystem::path> startupProject;

    pipeframe::editor::ProjectSession project;

    pipeframe::editor::SimulationSession simulation;

    pipeframe::editor::WorkbenchView view;
    pipeframe::editor::WorkbenchInput input;

    bool loaded = false;
    bool layoutInitialized = false;
};

Workbench::Workbench(std::optional<std::filesystem::path> startupProject)
    : implementation(std::make_unique<Implementation>(std::move(startupProject))) {}

Workbench::~Workbench() = default;

void Workbench::Load() { implementation->Load(); }

void Workbench::Start() { implementation->Start(); }

void Workbench::OnResize(const sf::Vector2u newSize, RenderContext &context) {

    implementation->OnResize(newSize, context);
}

void Workbench::HandleEvent(const sf::Event &event, RenderContext &context) {

    implementation->HandleEvent(event, context);
}

void Workbench::FixedUpdate(const float fixedDeltaTime) { implementation->FixedUpdate(fixedDeltaTime); }

void Workbench::Update(const float deltaTime) { implementation->Update(deltaTime); }

void Workbench::Render(RenderContext &context) { implementation->Render(context); }

unsigned int Workbench::GetFrameRateLimit() const { return implementation->GetFrameRateLimit(); }

float Workbench::GetSimulationTimeScale() const { return implementation->GetSimulationTimeScale(); }

bool Workbench::UseMaximumSimulationRate() const { return implementation->UseMaximumSimulationRate(); }

void Workbench::Stop() { implementation->Stop(); }

void Workbench::Unload() { implementation->Unload(); }

std::unique_ptr<Scene> CreateWorkbench(std::optional<std::filesystem::path> startupProject) {

    return std::make_unique<Workbench>(std::move(startupProject));
}
