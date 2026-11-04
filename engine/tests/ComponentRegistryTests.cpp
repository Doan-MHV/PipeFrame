#include <PipeFrame/Components/CommonComponentSchemas.h>
#include <PipeFrame/Project/Component.h>
#include <PipeFrame/UI/SchemaInspector.h>
#include <limits>
#include <iostream>

using namespace pipeframe;
void Check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
struct InheritedComponent : Component<InheritedComponent> {
    double speed{2};
    int cache{42};
    bool operator==(const InheritedComponent&) const = default;
    static auto BuildSchema() {
        return Define("test.inherited", "Inherited")
            .Editable({.key="speed", .displayName="Speed", .kind=PropertyKind::Number,
                       .defaultValue=2.0, .minimum=0}, &InheritedComponent::speed)
            .Validate("Speed must not exceed five", [](const auto& value) { return value.speed<=5; });
    }
};
struct PlainComponentLayout { double speed{2}; int cache{42}; };
struct OwnedComponent : Component<OwnedComponent> {
    SceneObjectReference owner;
    static auto BuildSchema() {
        return Define("test.owned", "Owned").OnlyOn("test.rover").OwnerReference("target", &OwnedComponent::owner)
            .Validate("Owner must exist", [](const auto& value){return value.owner.objectId != 0;});
    }
};
static_assert(std::is_empty_v<Component<InheritedComponent>>);
static_assert(!std::is_polymorphic_v<InheritedComponent>);
static_assert(std::is_trivially_copyable_v<InheritedComponent>);
static_assert(std::is_standard_layout_v<InheritedComponent>);
static_assert(sizeof(InheritedComponent)==sizeof(PlainComponentLayout));
static_assert(InheritedComponent{}==InheritedComponent{});
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
        BehaviourScene ownerScene;
        ComponentRegistry ownerRegistry;
        ownerRegistry.Register(OwnedComponent::Schema());
        auto firstOwner = ownerScene.CreateObject(), copiedOwner = ownerScene.CreateObject();
        std::string ownerError;
        std::vector<ComponentMutation> owned{{firstOwner,"test.owned",{{"target",SceneObjectReference{999}}},42,"test.rover"}};
        Check(ownerRegistry.Apply(owned,ownerError,false) && firstOwner.GetComponent<OwnedComponent>()->owner.objectId==42,
              "Restoration binds legacy targets to the containing entity");
        Check(ownerRegistry.Inspect(firstOwner).front().properties.empty(), "Owner reference is not an editable or serialized field");
        owned={{copiedOwner,"test.owned",{},77,"test.other"}};
        Check(!ownerRegistry.Apply(owned,ownerError,false) && !copiedOwner.GetComponent<OwnedComponent>(),
              "Wrong entity type rejects component restoration atomically");
        owned.front().ownerType="test.rover";
        Check(ownerRegistry.Apply(owned,ownerError,false,true) && !copiedOwner.GetComponent<OwnedComponent>(),
              "Owner binding validation does not mutate the ECS");
        Check(ownerRegistry.Apply(owned,ownerError,false) && copiedOwner.GetComponent<OwnedComponent>()->owner.objectId==77,
              "Copied component binds to its new owner without an ID field");
        owned={{firstOwner,"test.owned",{{"target",SceneObjectReference{77}}}}};
        Check(!ownerRegistry.Apply(owned,ownerError), "Retired target cannot be edited to another owner");
        const auto inheritedSchema=InheritedComponent::Schema();
        InheritedComponent value;
        std::string inheritedError;
        Check(inheritedSchema.Describe().typeId=="test.inherited", "Base forwards typed schema metadata");
        Check(!inheritedSchema.Apply(value, {{"speed", 6.0}}, inheritedError) && value.speed==2,
              "Inherited schema retains atomic domain validation");
        Check(inheritedSchema.Apply(value, {{"speed", 3.0}}, inheritedError) && value.cache==42,
              "Inherited schema preserves unlisted runtime data");
        const auto serialized=inheritedSchema.Serialize(value);
        Check(serialized.properties.size()==1 && std::get<double>(serialized.properties.at("speed"))==3,
              "Inherited schema serializes only declared fields");
        BehaviourScene inheritedScene;
        const auto inheritedObject=inheritedScene.CreateObject();
        ComponentRegistry inheritedRegistry;
        inheritedRegistry.Register(inheritedSchema);
        const std::vector<ComponentMutation> restoreInherited{{inheritedObject,"test.inherited",serialized.properties}};
        Check(inheritedRegistry.Apply(restoreInherited,inheritedError,false) &&
              *inheritedObject.GetComponent<InheritedComponent>()==value,
              "Inherited component round trips through ECS registration and restore");
        struct Sensor { double range{5}; std::string label{"front"}; float reading{}; };
        const auto schema=ComponentSchema<Sensor>("test.sensor","Sensor")
            .Field({"range","Range",PropertyKind::Number,5.0,true,"m",0,100},&Sensor::range)
            .Field({"label","New exposed label",PropertyKind::String,std::string{"front"}},&Sensor::label)
            .Field({"reading","Reading",PropertyKind::Number,0.0,false},&Sensor::reading)
            .RetiredField("legacyStatus");
        ComponentRegistry registry; RegisterCommonComponents(registry); registry.Register(schema);
        BehaviourScene scene; const auto first=scene.CreateObject(),second=scene.CreateObject();
        std::string error;
        const std::vector<ComponentMutation> restore{{first,"test.sensor",{}},{first,Transform2DComponentTypeId,{}},{second,"test.sensor",{{"range",8.0}}}};
        Check(registry.Apply(restore,error,false),"Restoring schemas creates typed components with defaults");
        const std::vector<ComponentMutation> legacy{{first,"test.sensor",{{"legacyStatus",std::string{"Hit"}}}}};
        Check(registry.Apply(legacy,error,false),"Retired fields load through registry");
        Check(!registry.Apply(legacy,error,true),"Retired fields are not editable");
        Check(!schema.Serialize(*first.GetComponent<Sensor>()).properties.contains("legacyStatus"),"Retired fields are not serialized");
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

        struct ConciseComponent : Component<ConciseComponent> {
            float speed{1.5f};
            std::int64_t count{4};
            bool active{true};
            std::string mode{"Auto"};
            Vector2f offset{2.f, -3.f};
            Color tint{10, 20, 30, 255};
            AssetReference asset{"map-1"};
            SceneObjectReference target{9};
            double telemetryValue{42.0};
            static auto BuildSchema() {
                return Define("test.concise", "Concise")
                    .Editable("speed", "Speed", &ConciseComponent::speed, {.unit = "m/s", .minimum = 0, .maximum = 10})
                    .Editable("count", "Count", &ConciseComponent::count, {.minimum = 1})
                    .Editable("active", "Active", &ConciseComponent::active)
                    .Editable("mode", "Mode", &ConciseComponent::mode, {.enumOptions = {"Auto", "Manual"}})
                    .Editable("offset", "Offset", &ConciseComponent::offset)
                    .Editable("tint", "Tint", &ConciseComponent::tint)
                    .Editable("asset", "Asset", &ConciseComponent::asset)
                    .Editable("target", "Target", &ConciseComponent::target)
                    .ReadOnly("telemetryValue", "Telemetry", &ConciseComponent::telemetryValue);
            }
        };
        const auto conciseSchema = ConciseComponent::Schema();
        const auto& props = conciseSchema.Describe().properties;
        Check(props.size() == 9, "Concise schema registers all 9 fields");
        Check(props[0].kind == PropertyKind::Number && std::abs(std::get<double>(props[0].defaultValue) - 1.5) < 1e-6 && props[0].unit == "m/s",
              "Concise float field deduces Number kind and defaultValue");
        Check(props[1].kind == PropertyKind::Integer && std::get<std::int64_t>(props[1].defaultValue) == 4,
              "Concise int64 field deduces Integer kind and defaultValue");
        Check(props[2].kind == PropertyKind::Boolean && std::get<bool>(props[2].defaultValue) == true,
              "Concise bool field deduces Boolean kind and defaultValue");
        Check(props[3].kind == PropertyKind::Enum && std::get<std::string>(props[3].defaultValue) == "Auto",
              "Concise enumOptions field deduces Enum kind and defaultValue");
        Check(props[4].kind == PropertyKind::Vector2 && std::get<Vector2f>(props[4].defaultValue) == Vector2f{2.f, -3.f},
              "Concise Vector2f field deduces Vector2 kind and defaultValue");
        Check(props[5].kind == PropertyKind::Color && std::get<Color>(props[5].defaultValue) == Color{10, 20, 30, 255},
              "Concise Color field deduces Color kind and defaultValue");
        Check(props[6].kind == PropertyKind::AssetReference && std::get<AssetReference>(props[6].defaultValue).assetId == "map-1",
              "Concise AssetReference field deduces AssetReference kind and defaultValue");
        Check(props[7].kind == PropertyKind::ObjectReference && std::get<SceneObjectReference>(props[7].defaultValue).objectId == 9,
              "Concise SceneObjectReference field deduces ObjectReference kind and defaultValue");
        Check(!props[8].editable && props[8].editorHint == "telemetry" && std::get<double>(props[8].defaultValue) == 42.0,
              "Concise ReadOnly field defaults editorHint to telemetry");

        BehaviourScene resolverScene;
        auto resolvedObj = resolverScene.CreateObject();
        resolverScene.SetObjectResolver([&](SceneObjectId id) {
            return id == 77 ? resolvedObj : SceneObject{};
        });
        Check(resolverScene.FindObject(77) == resolvedObj && !resolverScene.FindObject(99).IsValid() &&
              resolverScene.FindObject(SceneObjectReference{77}) == resolvedObj,
              "BehaviourScene::FindObject resolves SceneObjectId and SceneObjectReference");

        std::cout << "Typed component registry acceptance passed.\n";
    } catch(const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
