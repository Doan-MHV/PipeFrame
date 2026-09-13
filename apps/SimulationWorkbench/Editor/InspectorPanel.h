#ifndef PIPEFRAME_INSPECTOR_PANEL_H
#define PIPEFRAME_INSPECTOR_PANEL_H

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

#include <PipeFrame/Project/ProjectRuntime.h>
#include <PipeFrame/Project/ProjectTypes.h>
#include <PipeFrame/Simulation/SimulationController.h>
#include <PipeFrame/UI/ViewPanel.h>

class LabeledNumericField;
class SimulationTransport;

class InspectorPanel : public pipeframe::ui::ViewPanel {
public:
    using ActionCallback = std::function<void()>;
    using SpeedSelectedCallback = std::function<void(SimulationSpeed)>;
    using TransformCommittedCallback = std::function<void(const pipeframe::SceneTransform &)>;
    using PropertyCommittedCallback = std::function<void(const std::string &, const pipeframe::PropertyValue &)>;
    using ComponentPropertyCommittedCallback =
        std::function<void(const std::string &, const std::string &, const pipeframe::PropertyValue &)>;

    InspectorPanel();
    void SetOnSpeedSelected(SpeedSelectedCallback callback);
    void SetOnPlayPause(ActionCallback callback);
    void SetOnSingleStep(ActionCallback callback);
    void SetOnReset(ActionCallback callback);
    void SetOnTransformCommitted(TransformCommittedCallback callback);
    void SetOnPropertyCommitted(PropertyCommittedCallback callback);
    void SetOnComponentAttachment(std::function<void(const std::string &,bool)> callback) { onComponentAttachment=std::move(callback);InvalidateView(); }
    void SetOnComponentPropertyCommitted(ComponentPropertyCommittedCallback callback);
    void SetLiveObjects(std::vector<pipeframe::SceneObjectData> objects) { liveObjects=std::move(objects); }
    void SetLiveComponentProperties(
        std::span<const pipeframe::ProjectRuntimeComponentEdit> properties);
    void SetSimulationState(bool playing, bool previewActive, SimulationSpeed speed);
    void SetAuthoringEnabled(bool enabled);
    void SetSelection(const pipeframe::SceneObjectData *object,
                      const pipeframe::SceneObjectTypeDescriptor *type);
    void SetSelection(std::span<const pipeframe::SceneObjectData *const> objects,
                      const pipeframe::SceneObjectTypeDescriptor *type,
                      std::span<const pipeframe::SceneComponentTypeDescriptor> componentTypes,
                      const pipeframe::ExtensionRegistry *extensions = nullptr);
    void ClearSelection();
    std::vector<std::string> GetVisibleComponentSections() const;
    std::vector<std::string> GetVisiblePropertyKeys() const;
    bool IsPropertyMixed(const std::string &componentTypeId, const std::string &key) const;
    bool IsSectionCollapsed(const std::string &componentTypeId) const;
    std::optional<pipeframe::PropertyValue> GetDisplayedPropertyValue(
        const std::string &componentTypeId, const std::string &key) const;
    std::optional<pipeframe::InspectorPresentation> GetCustomPresentation(
        const std::string &componentTypeId, const std::string &key) const;

protected:
    pipeframe::ui::View BuildView() override;

private:
    enum class RowComponent { Scalar, VectorX, VectorY, ColorR, ColorG, ColorB, ColorA };
    struct PropertyRow {
        std::string caption, textValue, choiceText;
        float numericValue{};
        bool choice{}, text{};
        pipeframe::PropertyDescriptor descriptor;
        std::string componentTypeId;
        RowComponent component{RowComponent::Scalar};
        std::vector<std::string> choices;
        std::size_t choiceIndex{0};
        bool objectProperty{false};
        bool active{false};
        bool mixed{false};
        std::string sectionName;
        std::optional<pipeframe::InspectorPresentation> presentation;
    };

    void AppendRow();
    void RefreshRows();
    void HideRows();
    void ConfigureRow(PropertyRow &row, const std::string &section,
                      const pipeframe::PropertyDescriptor &descriptor,
                      const std::string &componentTypeId, bool objectProperty,
                      RowComponent component, const std::string &suffix = {});
    void CommitNumeric(std::size_t rowIndex, float value);
    void CommitText(std::size_t rowIndex, const std::string &value);
    void CommitChoice(std::size_t rowIndex);
    void ToggleSection(std::size_t rowIndex);
    void CommitValue(const PropertyRow &row, pipeframe::PropertyValue value);
    const pipeframe::PropertyValue *FindValue(const pipeframe::SceneObjectData &object,
                                               const PropertyRow &row) const;
    static float ReadScalar(const pipeframe::PropertyValue &value);
    static pipeframe::Vector2f ReadVector(const pipeframe::PropertyValue &value);
    static std::string ReadText(const pipeframe::PropertyValue &value);
    static std::string Caption(const pipeframe::PropertyDescriptor &descriptor,
                               const std::string &suffix, bool mixed);
    static pipeframe::PropertyDescriptor InferDescriptor(const std::string &key,
                                                          const pipeframe::PropertyValue &value);

    std::vector<pipeframe::SceneObjectData> ownedSelection, liveObjects;
    std::vector<const pipeframe::SceneObjectData *> selectedObjects;
    const pipeframe::SceneObjectTypeDescriptor *selectedType{nullptr};
    std::vector<pipeframe::SceneComponentTypeDescriptor> registeredComponentTypes;
    std::vector<pipeframe::ProjectRuntimeComponentEdit> liveComponentProperties;
    const pipeframe::ExtensionRegistry *inspectorExtensions{nullptr};
    std::unordered_set<std::string> collapsedSections;
    bool authoringEnabled{false};
    TransformCommittedCallback onTransformCommitted;
    PropertyCommittedCallback onPropertyCommitted;
    ComponentPropertyCommittedCallback onComponentPropertyCommitted;
    std::function<void(const std::string &,bool)> onComponentAttachment;
    bool choosingComponent{};
    ActionCallback onPlayPause,onSingleStep,onReset;
    SpeedSelectedCallback onSpeedSelected;
    bool playing{}, previewActive{};
    SimulationSpeed speed{SimulationSpeed::Realtime};
    std::string selectionText{"NO SELECTION"}, selectionKey{"none"};
    std::vector<PropertyRow> propertyRows;
};

#endif
