#pragma once
#include <PipeFrame/Project/ComponentSchema.h>
#include <PipeFrame/Components/Transform2DComponent.h>
#include <PipeFrame/Physics/ShapeQueries2D.h>
namespace pipeframe {
inline constexpr const char *EnvironmentCollider2DComponentTypeId="pipeframe.environment-collider2d";
inline constexpr const char *EnvironmentObstacleEntityTypeId="pipeframe.environment-obstacle";
struct EnvironmentCollider2DComponent {
    std::string shape{"Box"};
    Vector2f size{1,1},start{-.5f,0},end{.5f,0};
    std::int64_t layerMask{1};
    bool enabled{true},visible{true},trigger{};
    Color color{110,120,135,255};
    static auto Schema(){return ComponentSchema<EnvironmentCollider2DComponent>(EnvironmentCollider2DComponentTypeId,"Environment Collider")
        .Editable({.key="shape",.displayName="Shape",.kind=PropertyKind::Enum,.defaultValue=std::string{"Box"},.enumOptions={"Box","Segment"}},&EnvironmentCollider2DComponent::shape)
        .Editable({.key="size",.displayName="Box Size",.kind=PropertyKind::Vector2,.defaultValue=Vector2f{1,1}},&EnvironmentCollider2DComponent::size)
        .Editable({.key="start",.displayName="Segment Start",.kind=PropertyKind::Vector2,.defaultValue=Vector2f{-.5f,0}},&EnvironmentCollider2DComponent::start)
        .Editable({.key="end",.displayName="Segment End",.kind=PropertyKind::Vector2,.defaultValue=Vector2f{.5f,0}},&EnvironmentCollider2DComponent::end)
        .Editable({.key="layerMask",.displayName="Collision Layers",.kind=PropertyKind::Integer,.defaultValue=std::int64_t{1},.minimum=0,.maximum=4294967295.0},&EnvironmentCollider2DComponent::layerMask)
        .Editable({.key="enabled",.displayName="Collision Enabled",.kind=PropertyKind::Boolean,.defaultValue=true},&EnvironmentCollider2DComponent::enabled)
        .Editable({.key="trigger",.displayName="Is Trigger",.kind=PropertyKind::Boolean,.defaultValue=false},&EnvironmentCollider2DComponent::trigger)
        .Editable({.key="visible",.displayName="Visible",.kind=PropertyKind::Boolean,.defaultValue=true},&EnvironmentCollider2DComponent::visible)
        .Editable({.key="color",.displayName="Display Color",.kind=PropertyKind::Color,.defaultValue=Color{110,120,135,255}},&EnvironmentCollider2DComponent::color)
        .Validate("Box dimensions must be positive and segment endpoints distinct",[](const auto &v){return v.size.x>0&&v.size.y>0&&v.start!=v.end;});}
};
inline EnvironmentShape2D ColliderShape(const EnvironmentCollider2DComponent &collider,const Transform2DComponent &transform){
    auto shape=collider.shape=="Segment"?EnvironmentShape2D{{collider.start,collider.end,collider.end,collider.start},true}:
        AxisAlignedBox(-collider.size*.5f,collider.size*.5f);
    const float c=std::cos(transform.rotation),s=std::sin(transform.rotation);
    for(auto &p:shape.points){p={p.x*transform.scale.x,p.y*transform.scale.y};p=transform.position+Vector2f{p.x*c-p.y*s,p.x*s+p.y*c};}
    return shape;
}
}
