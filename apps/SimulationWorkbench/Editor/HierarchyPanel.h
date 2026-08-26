#ifndef PIPEFRAME_HIERARCHY_PANEL_H
#define PIPEFRAME_HIERARCHY_PANEL_H

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "SceneTypes.h"

#include <SFML/Graphics/Font.hpp>

#include <PipeFrame/UI/Panel.h>

class Button;
class Label;
class StackPanel;
class TextButton;

class HierarchyPanel : public Panel {
  public:
    using ObjectId = pipeframe::editor::SceneObjectId;
    using ActionCallback = std::function<void()>;
    using SelectionCallback = std::function<void(ObjectId)>;

    struct Item {
        ObjectId id;
        std::string name;
    };

    explicit HierarchyPanel(const sf::Font &font);

    void SetOnAdd(ActionCallback callback);
    void SetOnDelete(ActionCallback callback);
    void SetOnSelectionChanged(SelectionCallback callback);

    void SetItems(const std::vector<Item> &items);
    void SetSelectedObject(std::optional<ObjectId> objectId);

    void SetAuthoringEnabled(bool enabled);

  protected:
    void OnGeometryChanged() override;

  private:
    struct Entry {
        ObjectId id;
        Button *button = nullptr;
        Label *label = nullptr;
    };

    Entry *FindEntry(ObjectId id);
    void CreateEntry(const Item &item);
    void RefreshSelection();
    void RefreshDeleteButton();

    const sf::Font &font;

    Panel *headerPanel = nullptr;
    StackPanel *listPanel = nullptr;

    TextButton *addButton = nullptr;
    TextButton *deleteButton = nullptr;

    std::vector<Entry> entries;

    std::optional<ObjectId> selectedObjectId;

    bool authoringEnabled = false;

    SelectionCallback onSelectionChanged;
};

#endif