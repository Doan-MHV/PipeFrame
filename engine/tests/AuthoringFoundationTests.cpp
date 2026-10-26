#include <PipeFrame/Project/Authoring.h>
#include <PipeFrame/Project/ComponentSchema.h>
#include <PipeFrame/Components/EnergySchema.h>
#include <cstdlib>
#include <iostream>
using namespace pipeframe;
using namespace pipeframe::authoring;
namespace {
void Require(bool value,const char *message){if(!value){std::cerr<<"FAILED: "<<message<<'\n';std::exit(1);}}
struct DecoratedSensor {
    PF_COMPONENT("robot.decorated-distance", "Decorated Distance Sensor") {
        return ComponentBuilder(PipeFrameComponentId, PipeFrameComponentName)
            .Field(PF_PROPERTY("range", "Range", FieldKind::Number, 4.0))
            .Build();
    }
};
}
int main(){
    {
        pipeframe::EnergyComponent energy;
        energy.Refill(10.0f);
        energy.Consume(2.5f);
        const auto data = pipeframe::EnergySchema().Serialize(energy);
        pipeframe::EnergyComponent restored;
        std::string error;
        if (!pipeframe::EnergySchema().Apply(restored, data.properties, error) || restored.current != 7.5f)
            return 1;
        if (pipeframe::EnergySchema().Apply(restored, {{"current", 1e100}}, error) || restored.current != 7.5f)
            return 1;
    }

    ComponentRegistry components; std::string error;
    struct BoundSensor { double range{4}; bool enabled{true}; };
    auto bound = pipeframe::ComponentSchema<BoundSensor>("robot.bound-sensor", "Sensor")
        .Field({"range", "Range", pipeframe::PropertyKind::Number, 4.0, true, "m", 0.1, 10}, &BoundSensor::range)
        .Field({"enabled", "Enabled", pipeframe::PropertyKind::Boolean, true}, &BoundSensor::enabled);
    BoundSensor value;
    Require(bound.Apply(value, {{"range", 7.0}, {"enabled", false}}, error) && value.range == 7 && !value.enabled,
            "Typed editor edits must reach C++ members");
    Require(!bound.Apply(value, {{"range", 99.0}, {"enabled", true}}, error) && value.range == 7 && !value.enabled,
            "Invalid multi-field edits must leave all members unchanged");
    const auto saved = bound.Serialize(value);
    BoundSensor restored;
    Require(bound.Apply(restored, saved.properties, error) && restored.range == 7 && !restored.enabled,
            "The same field bindings must serialize and restore component values");
    Require(!bound.Apply(value, {{"range", std::string("wrong type")}}, error),
            "Wrong typed values must be rejected without unsafe casts");
    auto transform=ComponentBuilder("pipeframe.transform2d","Transform")
        .Required().Field({"position","Position",FieldKind::Vector2,Vector2f{},"m"})
        .Field({"rotation","Rotation",FieldKind::Number,0.0,"deg",-360.0,360.0,1.0})
        .Field({"scale","Scale",FieldKind::Vector2,Vector2f{1,1}}).Build();
    Require(components.Register(std::move(transform),&error),"Shared transform schema must register");
    Require(!components.Register(ComponentBuilder("pipeframe.transform2d","Duplicate").Build(),&error),"Duplicate schema must be rejected");
    auto sensor=ComponentBuilder("robot.distance","Distance sensor")
        .Attachment({"signal","signal",{}, {"signal"},false})
        .Attachment({"mount","mechanical",{}, {"mechanical"},false})
        .Field({"range","Range",FieldKind::Number,4.0,"m",0.02,10.0,0.01}).Build();
    Require(components.Register(std::move(sensor)),"Project component schema must register without editor changes");
    Require(components.Register(DecoratedSensor::PipeFrameDescribeComponent()),
            "Optional component/property macros must produce canonical typed metadata");
    Require(components.Find("robot.decorated-distance")->fields.front().serializationId=="range",
            "Macro metadata must preserve stable serialization IDs");

    std::unordered_map<StableId,std::vector<AttachmentDescriptor>> points{
        {1,{{"sensor","mechanical",{}, {"mechanical"},false},{"pin","signal",{}, {"signal"},false}}},
        {2,{{"mount","mechanical",{}, {"mechanical"},false},{"input","signal",{}, {"signal"},false}}}};
    ConnectionGraph graph;
    Require(graph.Connect({1,ConnectionKind::Mechanical,{1,"sensor"},{2,"mount"}},points,&error),"Compatible attachments must connect");
    Require(graph.Connect({2,ConnectionKind::Signal,{1,"pin"},{2,"input"}},points,&error),"Signal endpoints must connect");
    Require(!graph.Connect({3,ConnectionKind::Signal,{1,"sensor"},{2,"input"}},points,&error),"Incompatible attachments must be rejected");
    graph.RemoveObject(1); Require(graph.GetConnections().empty(),"Deleting an object must remove dangling connections");

    AssetDatabase assets;
    Require(assets.Upsert({"wheel","parts/wheel.part","part","wheel",1,{},{"drive"}}),"Part asset must register");
    Require(assets.Upsert({"robot","prefabs/robot.prefab","prefab","assembly",1,{"wheel"},{}}),"Dependent asset must register");
    Require(assets.Move("wheel","parts/drive-wheel.part")&&assets.Find("wheel")->id=="wheel","Moving an asset must preserve its stable ID");
    Require(assets.Dependents("wheel")==std::vector<std::string>{"robot"},"Dependencies must remain queryable after moves");

    PrefabLibrary prefabs; Require(prefabs.Store({"robot.base",1,{1,2},{{2,"robot.distance","range",6.0}}}),"Assembly prefab must preserve overrides");
    Require(std::get<double>(prefabs.Find("robot.base")->overrides.front().value)==6.0,"Prefab override must round-trip");

    ExtensionRegistry extensions;
    Require(extensions.Register({"robot.parts","robot.plugin",ExtensionKind::Panel}),"Plugin panel must register");
    Require(extensions.Register({"robot.lidar","robot.plugin",ExtensionKind::Overlay}),"Plugin overlay must register");
    extensions.RemoveOwner("robot.plugin"); Require(extensions.Size()==0,"Plugin unload must remove owned extensions");

    WorkspaceLayout layout{"Robotics",{{"hierarchy",{{0,0},{300,800}},true,"left"},{"inspector",{{1200,0},{360,800}},true,"right"}},2.0f};
    Require(layout.panels.size()==2&&layout.dpiScale==2.0f,"Workspace layout must retain dock and DPI state");
    std::cout<<"All authoring foundation tests passed.\n";
}
