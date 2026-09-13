#include "InspectorPanel.h"

#include <PipeFrame/UI/SchemaInspector.h>
#include <PipeFrame/UI/SimulationTransportView.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ranges>
#include <sstream>
#include <type_traits>
#include <utility>
#include <tuple>

namespace {
const pipeframe::SceneComponentData *FindComponent(const pipeframe::SceneObjectData &object,
                                                   const std::string &typeId) {
    const auto found = std::ranges::find(object.components, typeId,
                                         &pipeframe::SceneComponentData::typeId);
    return found == object.components.end() ? nullptr : &*found;
}
}

InspectorPanel::InspectorPanel()  { SetPreferredSize({320,600}); }
void InspectorPanel::AppendRow() { propertyRows.emplace_back(); }
void InspectorPanel::SetOnSpeedSelected(SpeedSelectedCallback callback) { onSpeedSelected=std::move(callback); InvalidateView(); }
void InspectorPanel::SetOnPlayPause(ActionCallback callback) { onPlayPause=std::move(callback); InvalidateView(); }
void InspectorPanel::SetOnSingleStep(ActionCallback callback) { onSingleStep=std::move(callback); InvalidateView(); }
void InspectorPanel::SetOnReset(ActionCallback callback) { onReset=std::move(callback); InvalidateView(); }
void InspectorPanel::SetOnTransformCommitted(TransformCommittedCallback callback) { onTransformCommitted = std::move(callback); }
void InspectorPanel::SetOnPropertyCommitted(PropertyCommittedCallback callback) { onPropertyCommitted = std::move(callback); }
void InspectorPanel::SetOnComponentPropertyCommitted(ComponentPropertyCommittedCallback callback) { onComponentPropertyCommitted = std::move(callback); }
void InspectorPanel::SetLiveComponentProperties(
    const std::span<const pipeframe::ProjectRuntimeComponentEdit> properties) {
    liveComponentProperties.assign(properties.begin(), properties.end());
}
void InspectorPanel::SetSimulationState(bool value,bool active,SimulationSpeed rate) {
    if (playing==value && previewActive==active && speed==rate) return;
    playing=value; previewActive=active; speed=rate; InvalidateView();
}
void InspectorPanel::SetAuthoringEnabled(bool enabled) { if (authoringEnabled==enabled) return; authoringEnabled=enabled; InvalidateView(); }

void InspectorPanel::SetSelection(const pipeframe::SceneObjectData *object,
                                  const pipeframe::SceneObjectTypeDescriptor *type) {
    const pipeframe::SceneObjectData *items[]{object};
    SetSelection(object ? std::span<const pipeframe::SceneObjectData *const>{items, 1}
                        : std::span<const pipeframe::SceneObjectData *const>{}, type, {});
}

void InspectorPanel::SetSelection(std::span<const pipeframe::SceneObjectData *const> objects,
                                  const pipeframe::SceneObjectTypeDescriptor *type,
                                  std::span<const pipeframe::SceneComponentTypeDescriptor> componentTypes,
                                  const pipeframe::ExtensionRegistry *extensions) {
    const auto oldRows=propertyRows;
    const auto oldText=selectionText;
    const auto oldKey=selectionKey;
    ownedSelection.clear(); selectedObjects.clear();
    for (const auto *object : objects) if (object) {
        auto copy=*object;
        const auto live=std::ranges::find(liveObjects,object->id,&pipeframe::SceneObjectData::id);
        if (live!=liveObjects.end()) copy.components=live->components;
        ownedSelection.push_back(std::move(copy));
    }
    for (const auto &object : ownedSelection) selectedObjects.push_back(&object);
    selectedType = type;
    registeredComponentTypes.assign(componentTypes.begin(), componentTypes.end());
    inspectorExtensions=extensions;
    if (selectedObjects.empty()) { ClearSelection(); return; }
    selectionText=selectedObjects.size()==1 ? "SELECTED | "+selectedObjects.front()->name : "SELECTED | "+std::to_string(selectedObjects.size())+" OBJECTS";
    selectionKey.clear();
    for (const auto *object:selectedObjects) selectionKey+=std::to_string(object->id)+":";
    RefreshRows();
    // Commands read the current row model; only visible presentation changes
    // require rebuilding controls. Keep telemetry/schema queries authoritative.
    const auto presentation=[](const PropertyRow &row) {
        return std::tie(row.caption,row.textValue,row.choiceText,row.numericValue,
            row.choice,row.text,row.active,row.sectionName,row.componentTypeId,
            row.component,row.objectProperty,row.descriptor.key,row.descriptor.editable,
            row.descriptor.editorHint);
    };
    const bool sameRows=oldRows.size()==propertyRows.size() &&
        std::equal(oldRows.begin(),oldRows.end(),propertyRows.begin(),
            [&](const auto &a,const auto &b){return presentation(a)==presentation(b);});
    if (!sameRows || oldText!=selectionText || oldKey!=selectionKey) InvalidateView();
}
void InspectorPanel::ClearSelection() {
    if (selectedObjects.empty() && selectionKey=="none" && liveObjects.empty()) return;
    selectedObjects.clear(); ownedSelection.clear(); liveObjects.clear(); selectedType=nullptr;
    registeredComponentTypes.clear(); inspectorExtensions=nullptr;
    selectionText="NO SELECTION"; selectionKey="none"; HideRows(); InvalidateView();
}

void InspectorPanel::RefreshRows() {
    HideRows();
    if (selectedObjects.empty()) return;
    const auto &primary = *selectedObjects.front();
    std::size_t next = 0;
    std::vector<const pipeframe::SceneComponentData *> components;
    for (const auto &component : primary.components) components.push_back(&component);
    std::ranges::stable_sort(components, [](const auto *left, const auto *right) {
        return left->typeId == pipeframe::Transform2DComponentTypeId &&
               right->typeId != pipeframe::Transform2DComponentTypeId;
    });

    const auto addDescriptor = [&](const std::string &section,
                                   const pipeframe::PropertyDescriptor &descriptor,
                                   const std::string &componentTypeId, const bool objectProperty,
                                   bool &first) {
        const auto add = [&](const RowComponent component, const std::string &suffix) {
            if (next >= propertyRows.size()) AppendRow();
            ConfigureRow(propertyRows[next++], first ? section : "", descriptor,
                         componentTypeId, objectProperty, component, suffix);
            first = false;
        };
        if (descriptor.kind == pipeframe::PropertyKind::Vector2) {
            add(RowComponent::VectorX, "X"); add(RowComponent::VectorY, "Y");
        } else if (descriptor.kind == pipeframe::PropertyKind::Color) {
            add(RowComponent::ColorR, "R"); add(RowComponent::ColorG, "G");
            add(RowComponent::ColorB, "B"); add(RowComponent::ColorA, "A");
        } else add(RowComponent::Scalar, "");
    };

    for (const auto *component : components) {
        const auto registered = std::ranges::find(registeredComponentTypes, component->typeId,
                                                   &pipeframe::SceneComponentTypeDescriptor::typeId);
        bool first = true;
        if (registered != registeredComponentTypes.end()) {
            const std::string section = "COMPONENT  |  " + registered->displayName +
                                        (registered->editorOnly ? "  [EDITOR]" : "");
            if (registered->properties.empty()) {
                if (next>=propertyRows.size()) AppendRow();
                auto &row=propertyRows[next++]; row.active=true; row.componentTypeId=component->typeId;
                row.descriptor={}; row.sectionName=section; row.objectProperty=false;

            }
            for (const auto &descriptor : registered->properties)
                addDescriptor(section, descriptor, component->typeId, false, first);
        } else {
            std::vector<std::string> keys;
            for (const auto &[key, value] : component->properties) keys.push_back(key);
            std::ranges::sort(keys);
            for (const auto &key : keys)
                addDescriptor("COMPONENT  |  " + component->typeId + "  [UNREGISTERED]",
                              InferDescriptor(key, component->properties.at(key)), component->typeId, false, first);
        }
    }
    if (selectedType) {
        bool first = true;
        for (const auto &descriptor : selectedType->properties){
            const bool suppliedByComponent=std::ranges::any_of(components,[&](const auto *component){
                const auto type=std::ranges::find(registeredComponentTypes,component->typeId,
                                                  &pipeframe::SceneComponentTypeDescriptor::typeId);
                return type!=registeredComponentTypes.end()&&
                       std::ranges::find(type->properties,descriptor.key,&pipeframe::PropertyDescriptor::key)!=type->properties.end();
            });
            if(suppliedByComponent)continue;
            addDescriptor("OBJECT PROPERTIES", descriptor, "", true, first);
        }
    }
}

void InspectorPanel::ConfigureRow(PropertyRow &row, const std::string &section,
                                  const pipeframe::PropertyDescriptor &descriptor,
                                  const std::string &componentTypeId, const bool objectProperty,
                                  const RowComponent component, const std::string &suffix) {
    row.descriptor = descriptor; row.componentTypeId = componentTypeId;
    row.objectProperty = objectProperty; row.component = component; row.active = true;
    row.sectionName = section;
    row.choices = descriptor.enumOptions;

    const auto *primaryValue = FindValue(*selectedObjects.front(), row);
    const auto &value = primaryValue ? *primaryValue : descriptor.defaultValue;
    row.mixed = false;
    for (std::size_t index = 1; index < selectedObjects.size(); ++index) {
        const auto *other = FindValue(*selectedObjects[index], row);
        if (!other || *other != value) { row.mixed = true; break; }
    }
    row.presentation.reset();
    if(!descriptor.editorHint.empty()&&inspectorExtensions){
        const auto extensions=inspectorExtensions->All();
        const auto drawer=std::ranges::find_if(extensions,[&](const auto &extension){
            return extension.context==descriptor.editorHint;
        });
        if(drawer!=extensions.end()&&drawer->point==pipeframe::ExtensionPoint::Drawer&&drawer->inspect)try{
            row.presentation=drawer->inspect({selectedObjects.front()->id,componentTypeId,descriptor.key,value,
                                               row.mixed,descriptor.editorHint=="telemetry"||!descriptor.editable});
        }catch(...){row.presentation.reset();}
    }
    std::string caption = Caption(descriptor, suffix, row.mixed);
    if(row.presentation){
        if(!row.presentation->valueText.empty())caption+="  |  "+row.presentation->valueText;
        if(!row.presentation->detail.empty())caption+="  "+row.presentation->detail;
    }
    const bool choice = descriptor.kind == pipeframe::PropertyKind::Boolean || descriptor.kind == pipeframe::PropertyKind::Enum;
    const bool text = descriptor.kind == pipeframe::PropertyKind::String || descriptor.kind == pipeframe::PropertyKind::AssetReference ||
                      descriptor.kind == pipeframe::PropertyKind::Integer || descriptor.kind == pipeframe::PropertyKind::ObjectReference;
    row.choice=choice; row.text=text; row.caption=caption;
    if (text) row.textValue=row.mixed ? "<MIXED>" : ReadText(value);
    if (!choice && !text) {
        float scalar = ReadScalar(value);
        if (descriptor.kind == pipeframe::PropertyKind::Vector2) {
            const auto vector = ReadVector(value); scalar = component == RowComponent::VectorX ? vector.x : vector.y;
        } else if (descriptor.kind == pipeframe::PropertyKind::Color) {
            if (const auto *color = std::get_if<pipeframe::Color>(&value)) {
                if (component == RowComponent::ColorR) scalar = color->red;
                if (component == RowComponent::ColorG) scalar = color->green;
                if (component == RowComponent::ColorB) scalar = color->blue;
                if (component == RowComponent::ColorA) scalar = color->alpha;
            }
        }
        row.numericValue=scalar;
    }
    if (choice) {
        if (descriptor.kind == pipeframe::PropertyKind::Boolean) {
            row.choices = {"OFF", "ON"}; row.choiceIndex = ReadScalar(value) != 0 ? 1 : 0;
        } else {
            const auto found = std::ranges::find(row.choices, ReadText(value));
            row.choiceIndex = found == row.choices.end() ? 0 : static_cast<std::size_t>(found - row.choices.begin());
        }
        const std::string selected = row.choices.empty() ? "NO OPTIONS" : row.choices[row.choiceIndex];
        row.choiceText=caption + "  |  " + (row.mixed ? "<MIXED>" : selected);
    }
}

void InspectorPanel::CommitNumeric(const std::size_t index, float value) {
    if (!authoringEnabled || index >= propertyRows.size()) return;
    const auto &row = propertyRows[index];
    if (!row.active || !row.descriptor.editable || row.descriptor.editorHint == "telemetry") return;
    if (row.descriptor.minimum) value = std::max(value, static_cast<float>(*row.descriptor.minimum));
    if (row.descriptor.maximum) value = std::min(value, static_cast<float>(*row.descriptor.maximum));
    if (row.descriptor.step && *row.descriptor.step > 0)
        value = static_cast<float>(std::round(value / *row.descriptor.step) * *row.descriptor.step);
    const auto *existing = FindValue(*selectedObjects.front(), row);
    const auto &current = existing ? *existing : row.descriptor.defaultValue;
    if (row.descriptor.kind == pipeframe::PropertyKind::Vector2) {
        auto vector = ReadVector(current); (row.component == RowComponent::VectorX ? vector.x : vector.y) = value;
        CommitValue(row, vector); return;
    }
    if (row.descriptor.kind == pipeframe::PropertyKind::Color) {
        auto color = std::get_if<pipeframe::Color>(&current) ? std::get<pipeframe::Color>(current) : pipeframe::Color{};
        const auto channel = static_cast<std::uint8_t>(std::clamp(std::lround(value), 0L, 255L));
        if (row.component == RowComponent::ColorR) color.red = channel;
        if (row.component == RowComponent::ColorG) color.green = channel;
        if (row.component == RowComponent::ColorB) color.blue = channel;
        if (row.component == RowComponent::ColorA) color.alpha = channel;
        CommitValue(row, color); return;
    }
    if (row.descriptor.kind == pipeframe::PropertyKind::Integer) CommitValue(row, static_cast<std::int64_t>(std::llround(value)));
    else if (row.descriptor.kind == pipeframe::PropertyKind::ObjectReference)
        CommitValue(row, pipeframe::SceneObjectReference{static_cast<pipeframe::SceneObjectId>(std::max(0.0f, value))});
    else CommitValue(row, static_cast<double>(value));
}

void InspectorPanel::CommitText(const std::size_t index, const std::string &value) {
    if (!authoringEnabled || index >= propertyRows.size() || value == "<MIXED>") return;
    const auto &row = propertyRows[index];
    if (!row.active || !row.descriptor.editable || row.descriptor.editorHint == "telemetry") return;
    try { CommitValue(row,pipeframe::ui::detail::ParseProperty(row.descriptor.kind,value)); }
    catch (const std::invalid_argument &) { /* Keep the model unchanged; next refresh restores its value. */ }
}

void InspectorPanel::CommitChoice(const std::size_t index) {
    if (!authoringEnabled || index >= propertyRows.size()) return;
    auto &row = propertyRows[index];
    if (!row.active || !row.descriptor.editable || row.descriptor.editorHint == "telemetry" || row.choices.empty()) return;
    row.choiceIndex = (row.choiceIndex + 1) % row.choices.size();
    if (row.descriptor.kind == pipeframe::PropertyKind::Boolean) CommitValue(row, row.choiceIndex != 0);
    else CommitValue(row, row.choices[row.choiceIndex]);
}

void InspectorPanel::ToggleSection(const std::size_t index) {
    if (index >= propertyRows.size() || !propertyRows[index].active) return;
    const auto &row = propertyRows[index];
    const std::string key = row.objectProperty ? "object-properties" : row.componentTypeId;
    if (!collapsedSections.erase(key)) collapsedSections.insert(key);
    RefreshRows();
    InvalidateView();
}

void InspectorPanel::CommitValue(const PropertyRow &row, pipeframe::PropertyValue value) {
    if (row.objectProperty) { if (onPropertyCommitted) onPropertyCommitted(row.descriptor.key, value); }
    else if (onComponentPropertyCommitted) onComponentPropertyCommitted(row.componentTypeId, row.descriptor.key, value);
}

const pipeframe::PropertyValue *InspectorPanel::FindValue(const pipeframe::SceneObjectData &object,
                                                          const PropertyRow &row) const {
    if (!row.objectProperty && (!row.descriptor.editable || row.descriptor.editorHint == "telemetry")) {
        const auto live = std::ranges::find_if(liveComponentProperties, [&](const auto &property) {
            return property.objectId == object.id && property.componentTypeId == row.componentTypeId &&
                   property.propertyKey == row.descriptor.key;
        });
        if (live != liveComponentProperties.end()) return &live->value;
    }
    const pipeframe::PropertyMap *properties = &object.properties;
    if (!row.objectProperty) {
        const auto *component = FindComponent(object, row.componentTypeId);
        if (!component) return nullptr;
        properties = &component->properties;
    }
    const auto found = properties->find(row.descriptor.key);
    return found == properties->end() ? nullptr : &found->second;
}

void InspectorPanel::HideRows() { for(auto &row:propertyRows) row=PropertyRow{}; }

pipeframe::ui::View InspectorPanel::BuildView() {
    using namespace pipeframe::ui;
    std::vector<View> content{
        SimulationTransportView("transport",{playing,previewActive,speed,onPlayPause,onSingleStep,onReset,onSpeedSelected}),
        views::Text("selection",selectionText).FitHeight()};
    for(std::size_t i=0;i<propertyRows.size();++i) {
        const auto &row=propertyRows[i]; if(!row.active)continue;
        const std::string section=row.objectProperty?"object-properties":row.componentTypeId;
        const bool collapsed=collapsedSections.contains(section);
        if(!row.sectionName.empty()) content.push_back(views::Button("section:"+section,
            (collapsed?">  ":"v  ")+row.sectionName,[this,i]{ToggleSection(i);}));
        if(collapsed)continue;
        if(row.descriptor.key.empty()) {
            content.push_back(views::Text("empty:"+section,"No exposed properties").FitHeight());
            continue;
        }
        const auto key=section+":"+row.descriptor.key+":"+std::to_string(static_cast<int>(row.component));
        View control= row.choice ? views::Button(key,row.choiceText,[this,i]{CommitChoice(i);}) :
            row.text ? views::TextField(key,row.caption,row.textValue,[this,i](const std::string &v){CommitText(i,v);}) :
            views::NumberField(key,row.caption,row.numericValue,[this,i](float v){CommitNumeric(i,v);});
        content.push_back(control.Enabled(authoringEnabled&&row.descriptor.editable&&row.descriptor.editorHint!="telemetry"));
    }
    if(onComponentAttachment && selectedObjects.size()==1) {
        content.push_back(views::Button("add-component",choosingComponent?"CLOSE COMPONENT LIST":"ADD COMPONENT / BEHAVIOUR",
            [this]{choosingComponent=!choosingComponent;InvalidateView();}).Enabled(authoringEnabled));
        if(choosingComponent)for(const auto &type:registeredComponentTypes) {
            if(!type.removable)continue;
            const bool attached=FindComponent(*selectedObjects.front(),type.typeId)!=nullptr;
            content.push_back(views::Button("attach:"+type.typeId,(attached?"REMOVE ":"ADD ")+type.displayName,
                [this,id=type.typeId,attached]{onComponentAttachment(id,!attached);}).Enabled(authoringEnabled));
        }
    }
    return views::Column("inspector",{views::Text("header","INSPECTOR").Height(36),
        views::Scroll("properties:"+selectionKey,views::Column("fields",std::move(content)).Padding(12).Spacing(8)).Expanded()
    }).FillHeight().Spacing(0);
}

float InspectorPanel::ReadScalar(const pipeframe::PropertyValue &value) {
    if (const auto *number = std::get_if<double>(&value)) return static_cast<float>(*number);
    if (const auto *integer = std::get_if<std::int64_t>(&value)) return static_cast<float>(*integer);
    if (const auto *boolean = std::get_if<bool>(&value)) return *boolean ? 1 : 0;
    if (const auto *reference = std::get_if<pipeframe::SceneObjectReference>(&value)) return static_cast<float>(reference->objectId);
    return 0;
}

pipeframe::Vector2f InspectorPanel::ReadVector(const pipeframe::PropertyValue &value) {
    if (const auto *vector = std::get_if<pipeframe::Vector2f>(&value)) return *vector;
    return {};
}

std::string InspectorPanel::ReadText(const pipeframe::PropertyValue &value) {
    if (const auto *text = std::get_if<std::string>(&value)) return *text;
    if (const auto *asset = std::get_if<pipeframe::AssetReference>(&value)) return asset->assetId;
    if (const auto *integer=std::get_if<std::int64_t>(&value)) return std::to_string(*integer);
    if (const auto *reference=std::get_if<pipeframe::SceneObjectReference>(&value)) return std::to_string(reference->objectId);
    return {};
}

std::string InspectorPanel::Caption(const pipeframe::PropertyDescriptor &descriptor,
                                    const std::string &suffix, const bool mixed) {
    std::ostringstream result; result << descriptor.displayName;
    if (!suffix.empty()) result << ' ' << suffix;
    if (!descriptor.unit.empty()) result << " (" << descriptor.unit << ')';
    if (descriptor.editable && (descriptor.minimum || descriptor.maximum) &&
        (!descriptor.minimum || std::abs(*descriptor.minimum)<1e30) &&
        (!descriptor.maximum || std::abs(*descriptor.maximum)<1e30)) {
        result << " ["; if (descriptor.minimum) result << *descriptor.minimum; result << "..";
        if (descriptor.maximum) result << *descriptor.maximum; result << ']';
    }
    if (descriptor.editorHint == "telemetry" || !descriptor.editable) result << "  LIVE";
    if (mixed) result << "  MIXED";
    return result.str();
}

pipeframe::PropertyDescriptor InspectorPanel::InferDescriptor(const std::string &key,
                                                              const pipeframe::PropertyValue &value) {
    pipeframe::PropertyDescriptor result{key, key}; result.defaultValue = value;
    result.kind = std::visit([](const auto &typed) {
        using T = std::decay_t<decltype(typed)>;
        if constexpr (std::is_same_v<T, bool>) return pipeframe::PropertyKind::Boolean;
        if constexpr (std::is_same_v<T, std::int64_t>) return pipeframe::PropertyKind::Integer;
        if constexpr (std::is_same_v<T, double>) return pipeframe::PropertyKind::Number;
        if constexpr (std::is_same_v<T, std::string>) return pipeframe::PropertyKind::String;
        if constexpr (std::is_same_v<T, pipeframe::Vector2f>) return pipeframe::PropertyKind::Vector2;
        if constexpr (std::is_same_v<T, pipeframe::Color>) return pipeframe::PropertyKind::Color;
        if constexpr (std::is_same_v<T, pipeframe::AssetReference>) return pipeframe::PropertyKind::AssetReference;
        return pipeframe::PropertyKind::ObjectReference;
    }, value);
    return result;
}

std::vector<std::string> InspectorPanel::GetVisibleComponentSections() const {
    std::vector<std::string> result;
    for (const auto &row : propertyRows)
        if (row.active && !row.sectionName.empty()) result.push_back(row.sectionName);
    return result;
}

std::vector<std::string> InspectorPanel::GetVisiblePropertyKeys() const {
    std::vector<std::string> result;
    for (const auto &row : propertyRows)
        if (row.active && !row.descriptor.key.empty() && std::ranges::find(result, row.descriptor.key) == result.end()) result.push_back(row.descriptor.key);
    return result;
}

bool InspectorPanel::IsPropertyMixed(const std::string &componentTypeId, const std::string &key) const {
    return std::ranges::any_of(propertyRows, [&](const auto &row) {
        return row.active && row.componentTypeId == componentTypeId && row.descriptor.key == key && row.mixed;
    });
}

bool InspectorPanel::IsSectionCollapsed(const std::string &componentTypeId) const {
    return collapsedSections.contains(componentTypeId);
}

std::optional<pipeframe::PropertyValue> InspectorPanel::GetDisplayedPropertyValue(
    const std::string &componentTypeId, const std::string &key) const {
    if (selectedObjects.empty()) return std::nullopt;
    const auto row = std::ranges::find_if(propertyRows, [&](const auto &item) {
        return item.active && item.componentTypeId == componentTypeId && item.descriptor.key == key;
    });
    if (row == propertyRows.end()) return std::nullopt;
    if (const auto *value = FindValue(*selectedObjects.front(), *row)) return *value;
    return row->descriptor.defaultValue;
}

std::optional<pipeframe::InspectorPresentation> InspectorPanel::GetCustomPresentation(
    const std::string &componentTypeId,const std::string &key) const {
    const auto row=std::ranges::find_if(propertyRows,[&](const auto &item){
        return item.active&&item.componentTypeId==componentTypeId&&item.descriptor.key==key;
    });
    return row==propertyRows.end()?std::nullopt:row->presentation;
}
