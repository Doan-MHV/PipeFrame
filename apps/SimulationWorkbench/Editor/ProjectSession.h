#ifndef PIPEFRAME_PROJECT_SESSION_H
#define PIPEFRAME_PROJECT_SESSION_H

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "ProjectManager.h"
#include "PrefabLibrary.h"
#include "SceneDocument.h"
#include "SceneHistory.h"
#include <PipeFrame/Project/AssetDatabase.h>
#include <PipeFrame/Editor/TilemapAssetEditor.h>
#include <PipeFrame/Environment/TilemapChunkCache.h>
#include <PipeFrame/Editor/VisualAssetEditor.h>
#include <PipeFrame/Environment/VisualAssetModule.h>

#include "../Runtime/ProjectRuntimeHost.h"

namespace pipeframe::editor {

class ProjectSession final {
  public:
    ~ProjectSession();
    explicit ProjectSession(
        std::filesystem::path defaultProjectsDirectory);

    bool OpenStartupProject(
        const std::optional<std::filesystem::path>
            &startupProject,
        std::string *errorMessage = nullptr);

    bool OpenProject(
    const std::filesystem::path &path,
    std::string *errorMessage = nullptr);

    bool CreateProjectAt(
        const std::filesystem::path &directory,
        std::string *errorMessage = nullptr);

    std::vector<std::filesystem::path>
    GetRecentProjects() const;

    bool CreateNewProject(
        std::string *errorMessage = nullptr);

    bool OpenNextRecentProject(
        std::string *errorMessage = nullptr);

    bool LoadBuiltRuntime(const std::filesystem::path &library,std::string *error=nullptr);

    const TilemapAssetEditor &GetTilemapEditor() const { return tilemapEditor; }
    TilemapAssetEditor &GetTilemapEditor() { return tilemapEditor; }
    bool BeginTilemapEditing(const std::string &assetId);
    VisualAssetEditor &GetVisualAssetEditor(){return visualAssetEditor;}
    const VisualAssetEditor &GetVisualAssetEditor()const{return visualAssetEditor;}
    bool CreateTilemapAsset();
    bool MakeSelectedTilemapUnique();
    bool CanMakeSelectedTilemapUnique() const;
    bool HandleTilemapEvent(const InputEvent &event, RenderContext &context);
    void RenderTilemapEditing(RenderContext &context);
    Vector2f GetTilemapWorldScale()const{return tilemapTransform.scale;}
    std::string GetTilemapTargetName()const{
        const auto *object=tilemapTarget?document.FindObject(*tilemapTarget):nullptr;
        return object?object->name:"Asset local space";
    }

    bool Save(
        std::string *errorMessage = nullptr);

    bool SaveAsCopy(
        std::string *errorMessage = nullptr);

    bool CanSwitchProject(
        std::string *errorMessage = nullptr) const;

    bool HasProject() const;

    const ProjectManifest *GetManifest() const;

    const std::filesystem::path &
    GetProjectDirectory() const;

    const SceneDocument &GetDocument() const;
    SceneDocument &GetDocument();

    ProjectRuntimeHost &GetRuntime();
    const ProjectRuntimeHost &GetRuntime() const;

    assets::AssetDatabase &GetAssetDatabase();
    const assets::AssetDatabase &GetAssetDatabase() const;
    PrefabLibrary &GetPrefabLibrary();
    const PrefabLibrary &GetPrefabLibrary() const;

    std::span<const SceneObjectTypeDescriptor>
    GetObjectTypes() const;

    std::span<const SceneComponentTypeDescriptor> GetComponentTypes() const;

    const SceneObjectTypeDescriptor *
    FindObjectType(
        const SceneObjectTypeId &typeId) const;

    const SceneComponentTypeDescriptor *FindComponentType(const std::string &typeId) const;

    std::optional<SceneObjectId>
    GetSelectedObjectId() const;

    std::span<const SceneObjectId>
    GetSelectedObjectIds() const;

    bool IsObjectSelected(SceneObjectId objectId) const;

    const SceneObjectData *
    GetSelectedObject() const;

    void SetSelectedObject(
        std::optional<SceneObjectId> objectId);

    void SetSelectedObjects(std::vector<SceneObjectId> objectIds);
    void AddSelectedObject(SceneObjectId objectId);
    void ToggleSelectedObject(SceneObjectId objectId);

    bool CreateObject(std::optional<Vector2f> position = std::nullopt);
    bool CreateObjectOfType(const SceneObjectTypeId &typeId, std::optional<Vector2f> position = std::nullopt);
    bool ApplyRuntimeSceneEdits(std::vector<ProjectRuntimeSceneEdit> edits);
    bool DeleteSelectedObject();
    bool RenameSelectedObject(std::string name);
    bool ReparentSelectedObjects(SceneObjectId parentId, std::int32_t siblingOrder = -1);
    bool DuplicateSelectedObjects();
    bool GroupSelectedObjects(std::string groupName = "GROUP");
    bool SetSelectedObjectsVisible(bool visible);
    bool SetSelectedObjectsLocked(bool locked);
    bool SetSelectedObjectsLayer(std::string layer);
    bool SetSelectedObjectsTags(std::vector<std::string> tags);
    bool AddSelectedComponent(SceneComponentData component);
    bool RemoveSelectedComponent(const std::string &componentTypeId);
    bool SetSelectedComponentProperty(const std::string &componentTypeId,
                                      std::string key, PropertyValue value);
    bool ResetSelectedComponentProperty(const std::string &componentTypeId,
                                        const std::string &key);
    std::optional<SceneComponentData> CopySelectedComponent(
        const std::string &componentTypeId) const;
    bool PasteComponentToSelected(const SceneComponentData &component);
    bool SetSceneSettings(SceneSettings settings);
    bool AddSceneConnection(SceneConnectionData connection);
    bool RemoveSceneConnection(std::uint64_t connectionId);

    bool CreatePrefabFromSelected(const std::string &prefabId,
                                  std::string *errorMessage = nullptr);
    bool InstantiatePrefab(const std::string &prefabId, SceneObjectId parentId = 0,
                           SceneTransform placement = {}, std::string *errorMessage = nullptr);
    bool ApplySelectedPrefab(std::string *errorMessage = nullptr);
    bool RevertSelectedPrefab(std::string *errorMessage = nullptr);
    bool UnpackSelectedPrefab(std::string *errorMessage = nullptr);

    bool SetSelectedTransform(
        const SceneTransform &transform);

    bool SetSelectedProperty(
        const std::string &key,
        const PropertyValue &value);

    bool ResetSelectedProperty(const std::string &key);
    bool AssignAssetToSelectedProperty(const std::string &componentTypeId,
                                       const std::string &key,
                                       const assets::AssetId &assetId);
    bool AssignAssetToFirstSelectedAssetProperty(const assets::AssetId &assetId);
    std::vector<assets::AssetId> FindMissingAssetReferences() const;
    bool RepairAssetReferences(const assets::AssetId &missingAssetId,
                               const assets::AssetId &replacementAssetId);

    bool BeginContinuousEdit();

    bool UpdateSelectedTransform(
        const SceneTransform &transform);

    bool UpdateSelectedTransforms(
        const std::vector<std::pair<SceneObjectId, SceneTransform>> &transforms);

    bool CommitContinuousEdit();
    bool CancelContinuousEdit();

    bool Undo();
    bool Redo();

    bool CanUndo() const;
    bool CanRedo() const;

    void SynchronizeRuntime();

  private:
    void ActivateDocument(
        SceneDocument document);

    void LoadActiveRuntime();
    void LoadAuthoredObjectTypes();
    void MigrateDocumentToRuntimeComponents();

    void ValidateSelection();

    void DocumentChanged();

    std::string MakeUniqueObjectName(
        const std::string &baseName) const;

    static void SetError(
        std::string *errorMessage,
        std::string message);

    std::filesystem::path defaultProjectsDirectory;

    ProjectManager projectManager;
    assets::AssetDatabase assetDatabase;
    ProjectRuntimeHost runtimeHost;
    PrefabLibrary prefabLibrary;
    std::vector<SceneObjectTypeDescriptor> authoredObjectTypes;
    std::vector<SceneComponentTypeDescriptor> commonComponentTypes;
    std::vector<SceneComponentTypeDescriptor> authoredComponentTypes;
    mutable std::vector<SceneComponentTypeDescriptor> combinedComponentTypes;

    SceneDocument document;
    SceneHistory history;
    VisualAssetEditor visualAssetEditor;
    VisualAssetModule editingVisuals;
    std::optional<Tileset2D> tilemapAtlas;
    Color tilemapTint{255,255,255,255};
    TilemapAssetEditor tilemapEditor;
    SceneTransform tilemapTransform;
    std::optional<Rectanglef> tilemapBounds;
    std::optional<SceneObjectId> tilemapTarget;
    bool RefreshTilemapTarget();
    std::vector<Vertex2D> tilemapVertices;
    std::uint64_t tilemapGeometryRevision{};
    TilemapChunkCache tilemapChunks;
    std::uint64_t dataOverlayRevision{};
    std::string dataOverlayTarget;
    std::vector<std::pair<GridCoordinate,double>> dataOverlayCells;

    std::optional<SceneObjectId> selectedObjectId;
    std::vector<SceneObjectId> selectedObjectIds;
};

} // namespace pipeframe::editor

#endif
