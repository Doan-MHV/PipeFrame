#include "HierarchyPanel.h"

#include <algorithm>
#include <utility>

#include <SFML/Graphics/Color.hpp>

#include <PipeFrame/UI/Button.h>
#include <PipeFrame/UI/Label.h>
#include <PipeFrame/UI/StackPanel.h>
#include <PipeFrame/UI/TextButton.h>

HierarchyPanel::HierarchyPanel(const sf::Font &font) : font(font) {
    SetSize({220.0f, 560.0f});
    SetFillColor(sf::Color(24, 27, 34, 245));
    SetOutlineColor(sf::Color(78, 86, 104));
    SetOutlineThickness(1.0f);

    headerPanel = &CreateChild<Panel>();

    headerPanel->SetFillColor(sf::Color(42, 47, 59));
    headerPanel->SetOutlineColor(sf::Color(90, 100, 120));
    headerPanel->SetOutlineThickness(1.0f);

    Label &headerLabel = headerPanel->CreateChild<Label>(font);

    headerLabel.SetPosition({0.0f, 0.0f});
    headerLabel.SetSize({100.0f, 48.0f});
    headerLabel.SetText("HIERARCHY");
    headerLabel.SetCharacterSize(14);
    headerLabel.SetAlignment(LabelAlignment::Left);
    headerLabel.SetHorizontalPadding(12.0f);
    headerLabel.SetHitTestVisible(false);

    deleteButton = &headerPanel->CreateChild<TextButton>(font);

    deleteButton->SetSize({38.0f, 36.0f});
    deleteButton->SetText("-");
    deleteButton->SetTextCharacterSize(18);
    deleteButton->SetEnabled(false);

    addButton = &headerPanel->CreateChild<TextButton>(font);

    addButton->SetSize({42.0f, 36.0f});
    addButton->SetText("+");
    addButton->SetTextCharacterSize(18);
    addButton->SetEnabled(false);

    listPanel = &CreateChild<StackPanel>();

    listPanel->SetFillColor(sf::Color(31, 34, 43));
    listPanel->SetOutlineColor(sf::Color(65, 72, 88));
    listPanel->SetOutlineThickness(1.0f);
    listPanel->SetOrientation(StackOrientation::Vertical);
    listPanel->SetPadding(Thickness{8.0f});
    listPanel->SetSpacing(6.0f);

    OnGeometryChanged();
}

void HierarchyPanel::SetOnAdd(ActionCallback callback) { addButton->SetOnClick(std::move(callback)); }

void HierarchyPanel::SetOnDelete(ActionCallback callback) { deleteButton->SetOnClick(std::move(callback)); }

void HierarchyPanel::SetOnSelectionChanged(SelectionCallback callback) { onSelectionChanged = std::move(callback); }

void HierarchyPanel::SetItems(const std::vector<Item> &items) {
    for (Entry &entry : entries) {
        entry.button->SetVisible(false);
    }

    for (const Item &item : items) {
        Entry *entry = FindEntry(item.id);

        if (entry == nullptr) {
            CreateEntry(item);
            continue;
        }

        entry->button->SetVisible(true);
        entry->label->SetText(item.name);
    }

    listPanel->RefreshLayout();
    RefreshSelection();
}

void HierarchyPanel::SetSelectedObject(std::optional<ObjectId> objectId) {
    selectedObjectId = objectId;

    RefreshSelection();
    RefreshDeleteButton();
}

void HierarchyPanel::SetAuthoringEnabled(bool enabled) {
    authoringEnabled = enabled;

    addButton->SetEnabled(authoringEnabled);
    RefreshDeleteButton();
}

void HierarchyPanel::OnGeometryChanged() {
    if (headerPanel == nullptr || listPanel == nullptr) {
        return;
    }

    const sf::Vector2f size = GetSize();
    const float innerWidth = std::max(1.0f, size.x - 24.0f);
    const float listHeight = std::max(1.0f, size.y - 84.0f);

    headerPanel->SetPosition({12.0f, 12.0f});
    headerPanel->SetSize({innerWidth, 48.0f});

    deleteButton->SetPosition({innerWidth - 92.0f, 6.0f});
    addButton->SetPosition({innerWidth - 48.0f, 6.0f});

    listPanel->SetPosition({12.0f, 72.0f});
    listPanel->SetSize({innerWidth, listHeight});
}

HierarchyPanel::Entry *HierarchyPanel::FindEntry(ObjectId id) {
    for (Entry &entry : entries) {
        if (entry.id == id) {
            return &entry;
        }
    }

    return nullptr;
}

void HierarchyPanel::CreateEntry(const Item &item) {
    Button &button = listPanel->CreateChild<Button>();

    button.SetSize({0.0f, 38.0f});

    const ObjectId id = item.id;

    button.SetOnClick([this, id]() {
        if (onSelectionChanged) {
            onSelectionChanged(id);
        }
    });

    Label &label = button.CreateChild<Label>(font);

    label.SetSize(button.GetSize());
    label.SetText(item.name);
    label.SetCharacterSize(11);
    label.SetAlignment(LabelAlignment::Left);
    label.SetHorizontalPadding(10.0f);
    label.SetHitTestVisible(false);

    entries.push_back({id, &button, &label});
}

void HierarchyPanel::RefreshSelection() {
    for (Entry &entry : entries) {
        const bool selected = selectedObjectId.has_value() && entry.id == *selectedObjectId;

        entry.button->SetNormalColor(selected ? sf::Color(48, 73, 110) : sf::Color(37, 41, 51));

        entry.button->SetOutlineColor(selected ? sf::Color(245, 179, 103) : sf::Color(76, 84, 102));
    }
}

void HierarchyPanel::RefreshDeleteButton() {
    deleteButton->SetEnabled(authoringEnabled && selectedObjectId.has_value());
}