#include <PipeFrame/Components/CommonComponentSchemas.h>
#include <PipeFrame/UI/SchemaInspector.h>
#include <limits>
#include <iostream>

using namespace pipeframe;
void Check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
struct ValidatedRangeComponent {
    double lower{0}, upper{10};
    std::string label{"sensor"};
    int internalCache{42}; // Hidden: not in Schema().
    static auto Schema() {
        return ComponentSchema<ValidatedRangeComponent>("test.validated-range","Validated range")
            .Editable({.key="lower",.displayName="Lower",.kind=PropertyKind::Number,.defaultValue=0.0,.minimum=0}, &ValidatedRangeComponent::lower)
            .Editable({.key="upper",.displayName="Upper",.kind=PropertyKind::Number,.defaultValue=10.0,.maximum=100}, &ValidatedRangeComponent::upper)
            .Editable({.key="label",.displayName="Label",.kind=PropertyKind::String,.defaultValue=std::string{"sensor"}}, &ValidatedRangeComponent::label)
            .Validate("Lower must not exceed upper", [](const auto &c){return c.lower<=c.upper;})
            .Validate("Label is required", [](const auto &c){return !c.label.empty();});
    }
};
int main() {
    try {
        struct Sensor { double range{5}; std::string label{"front"}; float reading{}; };
        const auto schema=ComponentSchema<Sensor>("test.sensor","Sensor")
            .Field({"range","Range",PropertyKind::Number,5.0,true,"m",0,100},&Sensor::range)
            .Field({"label","New exposed label",PropertyKind::String,std::string{"front"}},&Sensor::label)
            .Field({"reading","Reading",PropertyKind::Number,0.0,false},&Sensor::reading);
        ComponentRegistry registry; RegisterCommonComponents(registry); registry.Register(schema);
        BehaviourScene scene; const auto first=scene.CreateObject(),second=scene.CreateObject();
        std::string error;
        const std::vector<ComponentMutation> restore{{first,"test.sensor",{}},{first,Transform2DComponentTypeId,{}},{second,"test.sensor",{{"range",8.0}}}};
        Check(registry.Apply(restore,error,false),"Restoring schemas creates typed components with defaults");
        Check(registry.Inspect(first).size()==2,"Only attached components are discovered");
        const std::vector<ComponentMutation> invalid{{first,"test.sensor",{{"range",20.0}}},{second,"test.sensor",{{"range",200.0}}}};
        Check(!registry.Apply(invalid,error) && first.GetComponent<Sensor>()->range==5 && second.GetComponent<Sensor>()->range==8,
              "Invalid multi-entity batch changes neither component");
        const std::vector<ComponentMutation> readonly{{first,"test.sensor",{{"reading",2.0}}}};
        Check(!registry.Apply(readonly,error),"Read-only fields cannot be edited");
        const std::vector<ComponentMutation> unknown{{first,"test.sensor",{{"unknown",2.0}}}};
        Check(!registry.Apply(unknown,error),"Unknown fields rejected");
        const std::vector<ComponentMutation> nan{{first,"test.sensor",{{"range",std::numeric_limits<double>::quiet_NaN()}}}};
        Check(!registry.Apply(nan,error),"Nonfinite values rejected");
        const std::vector<ComponentMutation> rotate{{first,Transform2DComponentTypeId,{{"rotation",90.0}}}};
        Check(registry.Apply(rotate,error) && std::abs(first.GetComponent<Transform2DComponent>()->rotation-std::numbers::pi/2)<1e-6,
              "Transform schema converts editor degrees to runtime radians");
        auto view=ui::SchemaInspector("sensor",schema.Describe(),schema.Serialize(*first.GetComponent<Sensor>()).properties,
            [&](const std::string &key,const PropertyValue &value) {
                const std::vector<ComponentMutation> edit{{first,"test.sensor",{{key,value}}}};
                Check(registry.Apply(edit,error),"Generated field edits actual ECS storage");
            });
        view.children[2].children[1].onCommitted("rear");
        Check(first.GetComponent<Sensor>()->label=="rear","A newly declared field needs no Inspector/property dispatcher");
        for(int i=0;i<512;++i) scene.CreateObject().AddComponent<Sensor>();
        Check(first.GetComponent<Sensor>()->label=="rear","Inspection survives pool relocation");
        first.Destroy(); Check(registry.Inspect(first).empty() && !registry.Apply(rotate,error),"Destroyed selections are rejected");
        registry.Register(ValidatedRangeComponent::Schema());
        const auto range=scene.CreateObject(); range.AddComponent<ValidatedRangeComponent>();
        auto editRange=[&](PropertyMap values){
            const std::vector<ComponentMutation> edits{{range,"test.validated-range",std::move(values)}};
            return registry.Apply(edits,error);
        };
        Check(!editRange({{"lower",20.0}}) && error=="Lower must not exceed upper" && range.GetComponent<ValidatedRangeComponent>()->lower==0,
              "Cross-field validation rejects edits atomically");
        Check(!editRange({{"upper",50.0},{"label",std::string{}}}) && error=="Label is required" && range.GetComponent<ValidatedRangeComponent>()->upper==10,
              "Required string validation rolls back the whole candidate");
        Check(editRange({{"lower",20.0},{"upper",30.0}}),"Validation sees all proposed fields together");
        Check(!editRange({{"upper",101.0}}),"Built-in ranges still apply before custom rules");
        Check(!editRange({{"internalCache",std::int64_t{1}}}) && registry.Inspect(range).front().properties.size()==3,
              "Unlisted runtime data is neither exposed nor editable");
        Check(range.GetComponent<ValidatedRangeComponent>()->internalCache==42,"Validated edits preserve hidden runtime data");
        std::cout << "Typed component registry acceptance passed.\n";
    } catch(const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
