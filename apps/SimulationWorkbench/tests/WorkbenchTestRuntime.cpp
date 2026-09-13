#include "WorkbenchTestBrush.h"
#include <PipeFrame/Project/ProjectRuntime.h>
#include <cmath>
#include <PipeFrame/Components/TilemapComponent.h>
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <cstring>
class TestRuntime final : public pipeframe::ProjectRuntime {
    std::vector<pipeframe::SceneObjectData> objects;
    bool playing = false;
    std::size_t ticks = 0;
    bool tool = false, capture = false, uiFocus = false;
    std::size_t rightPresses = 0, capturedMoves = 0, tabs = 0;
    std::size_t authoredEdits = 0;
    std::size_t overlaySubmissions = 0;
    std::size_t extensionInvocations = 0;

  public:
    TestRuntime(){RegisterBrush<TestDensityBrush>();}
    const char *GetName() const override { return "Workbench test runtime"; }
    pipeframe::ProjectPluginDescriptor GetPluginDescriptor() const override {
        return {"test.workbench-runtime","Workbench test runtime",pipeframe::ProjectPluginAbiVersion,1,1,true};
    }
    bool RegisterPlugin(pipeframe::PluginRegistrar &registrar,std::string &error) override {
        const std::array points{pipeframe::ExtensionPoint::Panel,pipeframe::ExtensionPoint::Drawer,
            pipeframe::ExtensionPoint::Tool,pipeframe::ExtensionPoint::Gizmo,pipeframe::ExtensionPoint::Menu,
            pipeframe::ExtensionPoint::Command,pipeframe::ExtensionPoint::Overlay,pipeframe::ExtensionPoint::Importer,
            pipeframe::ExtensionPoint::Setting,pipeframe::ExtensionPoint::PartCategory,
            pipeframe::ExtensionPoint::AttachmentCompatibility,pipeframe::ExtensionPoint::ConnectionType,
            pipeframe::ExtensionPoint::EnvironmentBrush,pipeframe::ExtensionPoint::ValidationRule,
            pipeframe::ExtensionPoint::TelemetryStream,pipeframe::ExtensionPoint::SimulationDebugOverlay};
        for(std::size_t index=0;index<points.size();++index){
            pipeframe::ExtensionDescriptor extension{"test.extension."+std::to_string(index),{},
                "Test Extension "+std::to_string(index),points[index]};
            if(points[index]==pipeframe::ExtensionPoint::Drawer){
                extension.context="sensor-range";
                extension.inspect=[](const pipeframe::InspectorPresentationContext &context)
                    ->std::optional<pipeframe::InspectorPresentation>{
                    const auto *range=std::get_if<double>(&context.value);
                    if(!range)return std::nullopt;
                    return pipeframe::InspectorPresentation{std::to_string(*range).substr(0,4)+" m",
                                                             "PROJECT PREVIEW",*range/100.0,
                                                             {74,163,255,255}};
                };
            }
            extension.invoke=[this](const auto &){++extensionInvocations;};
            if(!registrar.Extension(std::move(extension),&error))return false;
        }
        if(!registrar.Action({"test.action",{},"Test Command","Tests",{"editor"},"Ctrl+T",
                              [](const auto &){}},&error))return false;
        pipeframe::ProjectSystemDescriptor system{"test.fixed-system",{},pipeframe::SystemPhase::FixedBehavior,{},
            {"test.sensor"},{},false,false,false,[this](auto &context){
                if(context.components)context.components->With("test.sensor");
                (void)context.random->Stream(context.tick);
                context.jobs->Submit(context.tick,[this]{if(playing)++ticks;});
                context.events->Publish({"test.tick",std::to_string(context.tick)});
                if(context.tick==1)context.log->Write("test","fixed system started");
                pipeframe::GeometryCommand command;command.topology=pipeframe::PrimitiveTopology::Lines;
                command.vertices={{{-10.0f,0.0f},{74,163,255,180}},{{10.0f,0.0f},{74,163,255,180}}};
                context.render->Submit(std::move(command));++overlaySubmissions;
            }};
        system.requiredServices={std::string(pipeframe::EventServiceId),std::string(pipeframe::RandomServiceId),
            std::string(pipeframe::JobServiceId),std::string(pipeframe::LogServiceId),
            std::string(pipeframe::ProfilingServiceId),std::string(pipeframe::RenderServiceId)};
        return registrar.System(std::move(system),&error);
    }
    pipeframe::ProjectReloadState CaptureReloadState() const override {
        pipeframe::ProjectReloadState state;state.payload.resize(sizeof(ticks));
        std::memcpy(state.payload.data(),&ticks,sizeof(ticks));return state;
    }
    bool RestoreReloadState(const pipeframe::ProjectReloadState &state,std::string &error) override {
        if(state.schemaVersion!=1||state.payload.size()!=sizeof(ticks)){error="Test state schema mismatch.";return false;}
        std::memcpy(&ticks,state.payload.data(),sizeof(ticks));return true;
    }
    bool Load(const pipeframe::ProjectRuntimeContext &, std::string &) override { return true; }
    std::span<const pipeframe::SceneObjectTypeDescriptor> GetSceneObjectTypes() const override {
        static const std::vector<pipeframe::SceneObjectTypeDescriptor> types{
            {"test.object", "Test Object", {}, {"test.sensor"}}
        };
        return types;
    }
    std::span<const pipeframe::SceneComponentTypeDescriptor> GetSceneComponentTypes() const override {
        static const std::vector<pipeframe::SceneComponentTypeDescriptor> types{
            pipeframe::TilemapComponent::Schema().Describe(),
            pipeframe::PlaygroundComponent::Schema().Describe(),
            {"test.sensor", "Test Sensor", 2, true, false,
            {{"range", "Range", pipeframe::PropertyKind::Number, 10.0, true, "m", 0.0, 100.0, 0.5, {}, "sensor-range"},
              {"enabled", "Enabled", pipeframe::PropertyKind::Boolean, true},
              {"samples", "Samples", pipeframe::PropertyKind::Integer, std::int64_t{8}},
              {"label", "Label", pipeframe::PropertyKind::String, std::string{"Sensor"}},
              {"offset", "Offset", pipeframe::PropertyKind::Vector2, pipeframe::Vector2f{}},
              {"tint", "Tint", pipeframe::PropertyKind::Color, pipeframe::Color{20,40,60,255}},
              {"texture", "Texture", pipeframe::PropertyKind::AssetReference, pipeframe::AssetReference{}},
              {"target", "Target", pipeframe::PropertyKind::ObjectReference, pipeframe::SceneObjectReference{}},
              {"mode", "Mode", pipeframe::PropertyKind::Enum, std::string{"Sweep"}, true, "", {}, {}, {}, {"Sweep","Track"}},
              {"reading", "Reading", pipeframe::PropertyKind::Number, 0.0, false, "m", {}, {}, {}, {}, "telemetry"}},
             {{"signal", "signal", {{20.0f,0.0f},0.0f,{1.0f,1.0f}}, {"signal"}, false}}}
        };
        return types;
    }
    void SynchronizeScene(std::span<const pipeframe::SceneObjectData> data) override {
        objects.assign(data.begin(), data.end());
        playing = false;
        tool = !objects.empty() && objects.front().properties.contains("test.tool");
    }
    bool ApplyComponentEdits(std::span<const pipeframe::ProjectRuntimeComponentEdit> edits,
                             pipeframe::ProjectRuntimeAuthoringState) override {
        for (const auto &edit : edits) {
            const auto object = std::ranges::find(objects, edit.objectId, &pipeframe::SceneObjectData::id);
            if (object == objects.end()) continue;
            const auto component = std::ranges::find(object->components, edit.componentTypeId,
                                                      &pipeframe::SceneComponentData::typeId);
            if (component == object->components.end()) continue;
            component->properties.insert_or_assign(edit.propertyKey, edit.value);
            if (edit.componentTypeId == pipeframe::Transform2DComponentTypeId) {
                if (edit.propertyKey == "position") object->transform.position = std::get<pipeframe::Vector2f>(edit.value);
                if (edit.propertyKey == "rotation") object->transform.rotation = static_cast<float>(std::get<double>(edit.value));
                if (edit.propertyKey == "scale") object->transform.scale = std::get<pipeframe::Vector2f>(edit.value);
            }
            ++authoredEdits;
        }
        return true;
    }
    std::vector<pipeframe::ProjectRuntimeComponentEdit> GetLiveComponentProperties() const override {
        std::vector<pipeframe::ProjectRuntimeComponentEdit> result;
        for (const auto &object : objects) {
            if (std::ranges::find(object.components,"test.sensor",&pipeframe::SceneComponentData::typeId)!=object.components.end())
                result.push_back({object.id,"test.sensor","reading",static_cast<double>(ticks)});
            result.push_back({object.id,"test.reload-proof","prefabLinkCount",
                              static_cast<std::int64_t>(object.prefabLinks.size())});
        }
        return result;
    }
    void SetSelectedObject(std::optional<pipeframe::SceneObjectId>) override {}
    std::optional<pipeframe::SceneObjectId> HitTest(pipeframe::Vector2f point) const override {
        for (const auto &object : objects)
            if (std::hypot(point.x - object.transform.position.x, point.y - object.transform.position.y) < 60)
                return object.id;
        return {};
    }
    bool UsesRightClickTool() const override { return tool; }
    bool HasWorldPointerCapture() const override { return capture; }
    bool HasUIFocus() const override { return uiFocus; }
    bool HandleUIEvent(const pipeframe::InputEvent &event, RenderContext &) override {
        if (const auto *focus = event.GetIf<pipeframe::FocusInput>(); focus && !focus->focused) {
            uiFocus = false;
            return false;
        }
        if (const auto *key = event.GetIf<pipeframe::KeyInput>();
            event.type == pipeframe::InputEventType::KeyPressed && key && key->key == pipeframe::InputKey::Tab && uiFocus) {
            ++tabs;
            return true;
        }
        return false;
    }
    void HandleEvent(const pipeframe::InputEvent &event, RenderContext &) override {
        if (const auto *focus = event.GetIf<pipeframe::FocusInput>(); focus && !focus->focused) {
            capture = false;
            return;
        }
        if (const auto *press = event.GetIf<pipeframe::PointerInput>();
            event.type == pipeframe::InputEventType::PointerPressed && press && press->button == pipeframe::PointerButton::Right && tool) {
            capture = true;
            uiFocus = true;
            ++rightPresses;
        }
        if (event.type == pipeframe::InputEventType::PointerMoved && capture)
            ++capturedMoves;
        if (event.type == pipeframe::InputEventType::PointerReleased)
            capture = false;
    }
    void Start() override { playing = true; }
    void Stop() override { playing = false; }
    void Reset() override { ticks = 0; }
    void Unload() override {}
    void FixedUpdate(float) override {
        if (playing)
            ++ticks;
    }
    void Render(RenderContext &) override {}
    pipeframe::ProjectRuntimeStatistics GetStatistics() const override {
        pipeframe::ProjectRuntimeStatistics result;
        result.available = true;
        result.vertexCount = rightPresses;
        result.geometryTimeMs = static_cast<float>(capturedMoves);
        result.spatialGridTimeMs = static_cast<float>(tabs);
        result.movementTimeMs = static_cast<float>(authoredEdits);
        result.usingQuads = overlaySubmissions > 0;
        result.candidateObjectCount = objects.size();
        result.visibleObjectCount = ticks;
        return result;
    }
};
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT pipeframe::ProjectRuntime *PipeFrameCreateProjectRuntime() {
    return new TestRuntime;
}
extern "C" PIPEFRAME_PROJECT_RUNTIME_EXPORT void PipeFrameDestroyProjectRuntime(pipeframe::ProjectRuntime *runtime) {
    delete runtime;
}
