#ifndef PIPEFRAME_ASSET_BROWSER_PANEL_H
#define PIPEFRAME_ASSET_BROWSER_PANEL_H

#include <functional>
#include <string>
#include <vector>

#include <PipeFrame/Project/AssetDatabase.h>
#include <PipeFrame/UI/ViewPanel.h>
#include <PipeFrame/Editor/TilemapAssetEditor.h>
#include <PipeFrame/Editor/VisualAssetEditor.h>

namespace pipeframe::editor {

class AssetBrowserPanel : public pipeframe::ui::ViewPanel {
public:
    using AssetCallback = std::function<void(const assets::AssetId &)>;
    using OperationCallback = std::function<void(assets::AssetOperationId)>;
    using ActionCallback = std::function<void()>;

    AssetBrowserPanel();
    void SetMapActions(ActionCallback create,AssetCallback edit,ActionCallback stop,
                       std::function<void(std::size_t,TileId,int)> shape) { onCreateMap=std::move(create); onEditMap=std::move(edit); onStopMap=std::move(stop); onShape=std::move(shape); }
    void Refresh(const assets::AssetDatabase &database, const TilemapAssetEditor *editor=nullptr, bool canMakeUnique=false,const VisualAssetEditor *visual=nullptr);
    struct VisualActions {
        std::function<void(assets::AssetType)> create;
        AssetCallback open;
        std::function<void(const std::string &,const PropertyValue &)> set;
        ActionCallback save,undo,redo;
        AssetCallback tileset;
        std::function<void(int,int,float,bool)> previewResize;
        ActionCallback applyResize;
    };
    struct EnvironmentActions {
        std::function<void(const std::string &)> selectBrush;
        std::function<void(const std::string &,const PropertyValue &)> brushSetting;
        std::function<void(int,bool)> brush;
        std::function<void(const std::string &,std::size_t,const std::string &)> layer;
        std::function<void(TileId,Color,bool)> tile;
        std::function<void(TileId,bool)> selection;
        ActionCallback save,undo,redo;
    };
    void SetEnvironmentActions(EnvironmentActions actions){environmentActions=std::move(actions);}
    void SetEnvironmentContext(std::string target,Vector2f scale){paintTarget=std::move(target);paintScale=scale;}
    void SetVisualActions(VisualActions actions){visualActions=std::move(actions);}
    void SetOnMakeUnique(ActionCallback callback) { onMakeUnique=std::move(callback); }
    struct AssignmentTarget {std::string component,key,label,hint;};
    void SetAssignmentTargets(std::vector<AssignmentTarget> targets){assignmentTargets=std::move(targets);}
    void SetOnAssignProperty(std::function<void(const std::string &,const std::string &,const std::string &)> callback){onAssignProperty=std::move(callback);}
    void SetOnAssign(AssetCallback callback);
    void SetOnImport(ActionCallback callback);
    void SetOnReimport(AssetCallback callback);
    void SetOnCancel(OperationCallback callback);
    const assets::AssetId &GetSelectedAssetId() const;
    std::size_t GetVisibleAssetCount() const;
    std::optional<assets::AssetId> AssetAt(pipeframe::Vector2f screenPosition) const;
    void SetSearchText(std::string text);
    void SelectCategory(assets::AssetType type);

protected:
    pipeframe::ui::View BuildView() override;

private:
    void Select(std::size_t index);
    void UpdateRows();
    void UpdateStatus();
    void UpdatePreview();

    const assets::AssetDatabase *database{nullptr};
    assets::AssetSearchQuery query;
    std::vector<const assets::AssetRecord *> visibleAssets;
    assets::AssetId selectedAssetId;
    std::vector<assets::AssetType> typeFilters;
    assets::AssetOperationId cancellableOperation{};

    std::string statusText;
    std::string previewKey,previewInfo;
    std::shared_ptr<GraphicsResourceService> previewResources;
    std::vector<pipeframe::ui::View::MeshLayer> previewLayers;
    std::optional<Tileset2D> previewAtlas;Color previewTint{255,255,255,255};
    std::size_t palettePage{};
    ActionCallback onCreateMap,onStopMap,onMakeUnique;
    AssetCallback onEditMap;
    std::function<void(std::size_t,TileId,int)> onShape;
    const TilemapAssetEditor *paintEditor{};
    std::size_t paintLayer{};
    TileId paintTile{1};
    int paintShape{};
    bool canMakeUnique{};
    VisualActions visualActions;
    EnvironmentActions environmentActions;
    std::string paintTarget;Vector2f paintScale{1,1};
    bool erasing{};
    const VisualAssetEditor *visualEditor{};
    std::string sizingAsset;float resizeColumns{32},resizeRows{24},resizeCellSize{10};
    void ApplyPaintSettings() { if(onShape)onShape(paintLayer,erasing?0:paintTile,paintShape); }
    std::vector<AssignmentTarget> assignmentTargets;
    std::function<void(const std::string &,const std::string &,const std::string &)> onAssignProperty;
    std::string fieldError;
    AssetCallback onAssign;
    ActionCallback onImport;
    AssetCallback onReimport;
    OperationCallback onCancel;
};

} // namespace pipeframe::editor
#endif
