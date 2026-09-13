#include "HierarchyPanel.h"
using namespace pipeframe::ui;
HierarchyPanel::HierarchyPanel()  {}
void HierarchyPanel::SetOnAdd(ActionCallback cb) { onAdd=std::move(cb); InvalidateView(); }
void HierarchyPanel::SetOnDelete(ActionCallback cb) { onDelete=std::move(cb); InvalidateView(); }
void HierarchyPanel::SetOnSelectionChanged(SelectionCallback cb) { onSelectionChanged=std::move(cb); }
void HierarchyPanel::SetItems(const std::vector<Item> &value) { items=value; InvalidateView(); }
void HierarchyPanel::SetSelectedObject(std::optional<ObjectId> id) { selectedObjectId=id; InvalidateView(); }
void HierarchyPanel::SetAuthoringEnabled(bool enabled) { authoringEnabled=enabled; InvalidateView(); }
View HierarchyPanel::BuildView() {
    std::vector<View> rows;
    for (const auto &item : items) rows.push_back(views::Button(std::to_string(item.id),item.name,
        [this,id=item.id] { if (onSelectionChanged) onSelectionChanged(id); }).Selected(selectedObjectId==item.id).Leading());
    return views::Column("hierarchy",{
        views::Row("header",{views::Text("title","HIERARCHY"),
            views::Button("delete","-",onDelete).Width(32).Enabled(authoringEnabled&&selectedObjectId.has_value()),
            views::Button("add","+",onAdd).Width(32).Enabled(authoringEnabled)}),
        views::Scroll("objects",views::Column("items",std::move(rows)).Spacing(2)).Expanded()
    }).Padding(8).FillHeight();
}
