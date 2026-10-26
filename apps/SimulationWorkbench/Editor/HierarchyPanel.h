#ifndef PIPEFRAME_HIERARCHY_PANEL_H
#define PIPEFRAME_HIERARCHY_PANEL_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "SceneTypes.h"

#include <PipeFrame/UI/ViewPanel.h>

class HierarchyPanel : public pipeframe::ui::ViewPanel {
  public:
    using ObjectId = pipeframe::editor::SceneObjectId;

    using ActionCallback = std::function<void()>;

    using SelectionCallback = std::function<void(ObjectId)>;

    struct Item {
        ObjectId id;
        std::string name;
    };

    HierarchyPanel();

    void SetOnAdd(ActionCallback callback);
    void SetOnDelete(ActionCallback callback);

    void SetOnSelectionChanged(SelectionCallback callback);

    void SetItems(const std::vector<Item> &items);

    void SetSelectedObject(std::optional<ObjectId> objectId);

    void SetAuthoringEnabled(bool enabled);

  private:
    pipeframe::ui::View BuildView() override;
    std::vector<Item> items;
    ActionCallback onAdd, onDelete;
    std::optional<ObjectId> selectedObjectId;
    bool authoringEnabled = false;
    SelectionCallback onSelectionChanged;
};
#endif
