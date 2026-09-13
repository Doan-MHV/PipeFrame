#pragma once
#include <PipeFrame/Environment/EnvironmentQueries.h>
#include <tuple>
#include <PipeFrame/Project/ProjectRuntime.h>
#include <PipeFrame/Components/CommonComponentSchemas.h>
#include <PipeFrame/Components/IdentityComponent.h>
#include <fstream>
#include <map>
#include <PipeFrame/Project/EntityRegistry.h>
#include <PipeFrame/Components/PlaygroundComponent.h>
#include <PipeFrame/Components/TilemapComponent.h>
#include <PipeFrame/Environment/TilemapAssetModule.h>
#include <PipeFrame/Environment/PlaygroundGeometry.h>
#include <PipeFrame/Environment/VisualAssetModule.h>
#include <PipeFrame/Environment/TilemapQueries.h>
#include <PipeFrame/Environment/TilemapSweep.h>
#include <PipeFrame/Components/KinematicBody2DComponent.h>
#include <PipeFrame/Components/EnvironmentCollider2DComponent.h>
#include <PipeFrame/Physics/GridCollision2D.h>
#include <PipeFrame/Render/RenderContext.h>

namespace pipeframe {
// Shared scene/lifecycle implementation for generated projects. Project code
// supplies component schemas and behaviour types, not another scene dispatcher.
class SceneProjectRuntime : public ProjectRuntime, public EnvironmentQueries {
    template<class T> struct BehaviourAttachment {};
public:
    explicit SceneProjectRuntime(std::string name) : name(std::move(name)) {
        scene.Services().Provide<EnvironmentQueries>(*this);
        RegisterCommonComponents(registry);
        RegisterComponent<IdentityComponent>();
        RegisterComponent<PlaygroundComponent>();
        RegisterComponent<TilemapComponent>();
        RegisterComponent<KinematicBody2DComponent>();
        RegisterComponent<EnvironmentCollider2DComponent>();
        RegisterEntity<ComponentEntity<Transform2DComponent,EnvironmentCollider2DComponent>>(
            {EnvironmentObstacleEntityTypeId,"Environment Obstacle",{}, {Transform2DComponentTypeId,EnvironmentCollider2DComponentTypeId}});
        RegisterEntity<ComponentEntity<Transform2DComponent,PlaygroundComponent,TilemapComponent>>(
            {PlaygroundEntityTypeId,"Playground",{}, {Transform2DComponentTypeId,PlaygroundComponentTypeId,TilemapComponentTypeId}});
    }
    const char *GetName() const override { return name.c_str(); }
    template<class T> void RegisterEntity(SceneObjectTypeDescriptor descriptor) {
        entities.Register<T>(std::move(descriptor));
        types=entities.Describe();
    }
    template<class T> void RegisterComponent() { registry.Register(T::Schema()); }
    template<class T> void RegisterBehaviour(std::string id, std::string displayName) {
        registry.Register(ComponentSchema<BehaviourAttachment<T>>(id,std::move(displayName)));
        behaviours.emplace(std::move(id),[](SceneObject object){object.template Attach<T>();});
    }
    bool Load(const ProjectRuntimeContext &context,std::string &error) override {
        tilemaps.Load(context,error);visuals.Load(context,error);
        types=entities.Describe();
        const auto directory=context.projectDirectory/"Assets/Prefabs";
        try {
            if(std::filesystem::is_directory(directory)) for(const auto &entry:std::filesystem::directory_iterator(directory)) {
                if(entry.path().extension()!=".pftype") continue;
                std::ifstream input(entry.path()); std::string header,title; unsigned version{};
                if(!(input>>header>>version>>title)||header!="PIPEFRAME_OBJECT_TYPE"||version!=1)
                    throw std::invalid_argument("Invalid object type: "+entry.path().string());
                SceneObjectTypeDescriptor type; type.typeId="project."+entry.path().stem().string();type.displayName=title;
                std::string component;
                while(input>>component) {
                    if(!registry.Contains(component)) throw std::invalid_argument("Unregistered component: "+component);
                    type.componentTypeIds.push_back(component);
                }
                if(std::ranges::any_of(types,[&](const auto &entry){return entry.typeId==type.typeId;}))
                    throw std::invalid_argument("Duplicate object type: "+type.typeId);
                types.push_back(std::move(type));
            }
            std::ranges::sort(types,{},&SceneObjectTypeDescriptor::typeId);
            error.clear();return true;
        } catch(const std::exception &exception) {error=exception.what();return false;}
    }
    std::span<const SceneObjectTypeDescriptor> GetSceneObjectTypes() const override {return types;}
    const ComponentRegistry *GetComponentRegistry() const override {return &registry;}
    SceneObject ResolveSceneObject(SceneObjectId id) const override {
        const auto found=objects.find(id);return found==objects.end()?SceneObject{}:found->second;
    }
    void SynchronizeScene(std::span<const SceneObjectData> source) override {
        PruneDestroyedObjects();
        std::map<SceneObjectId,SceneObjectData> next;
        for(const auto &data:source) {
            if(!next.emplace(data.id,data).second) throw std::invalid_argument("Duplicate authored entity ID");
            for(const auto &component:data.components)
                if(!registry.Contains(component.typeId)) throw std::invalid_argument("Unregistered component: "+component.typeId);
        }
        // Stage validation in an isolated scene before changing the live scene.
        BehaviourScene candidate;
        for(const auto &[id,data]:next) ApplyData(candidate.CreateObject(),data);
        for(auto it=objects.begin();it!=objects.end();) {
            if(!next.contains(it->first)) {it->second.Destroy();it=objects.erase(it);} else ++it;
        }
        for(const auto &[id,data]:next) {
            const auto previous=authored.find(id);
            bool recreate=!objects.contains(id);
            if(!recreate && previous!=authored.end()) {
                const auto topology=[](const SceneObjectData &value){
                    std::vector<std::pair<std::string,bool>> result;
                    for(const auto &c:value.components) result.emplace_back(c.typeId,c.enabled);
                    return result;
                };
                recreate=previous->second.typeId!=data.typeId || topology(previous->second)!=topology(data);
            }
            if(recreate) {
                if(objects.contains(id))objects.at(id).Destroy();
                auto object=entities.Contains(data.typeId) ? entities.Instantiate(scene,data,registry) : scene.CreateObject();
                if(!entities.Contains(data.typeId))ApplyData(object,data);objects[id]=object;
                for(const auto &c:data.components) if(c.enabled) {
                    const auto behaviour=behaviours.find(c.typeId);
                    if(behaviour!=behaviours.end()) behaviour->second(object);
                }
            } else if(previous==authored.end() || previous->second!=data) ApplyData(objects.at(id),data);
        }
        authored=std::move(next);
    }
    bool ApplyComponentEdits(std::span<const ProjectRuntimeComponentEdit> edits,ProjectRuntimeAuthoringState) override {
        std::vector<ComponentMutation> mutations;
        for(const auto &edit:edits) {
            const auto object=ResolveSceneObject(edit.objectId);if(!object.IsValid())return false;
            auto found=std::ranges::find_if(mutations,[&](const auto &m){return m.object==object && m.typeId==edit.componentTypeId;});
            if(found==mutations.end())mutations.push_back({object,edit.componentTypeId,{{edit.propertyKey,edit.value}}});
            else found->values.insert_or_assign(edit.propertyKey,edit.value);
        }
        std::string error;
        if(!registry.Apply(mutations,error))return false;
        for(const auto &edit:edits) {
            auto &data=authored.at(edit.objectId);
            auto component=std::ranges::find(data.components,edit.componentTypeId,&SceneComponentData::typeId);
            component->properties.insert_or_assign(edit.propertyKey,edit.value);
        }
        return true;
    }
    void SetSelectedObject(std::optional<SceneObjectId>) override {}
    std::optional<SceneObjectId> HitTest(Vector2f position) const override {
        // Small objects take selection priority over their containing playground.
        for(const auto &[id,object]:objects)if(!object.GetComponent<PlaygroundComponent>())if(const auto *transform=object.GetComponent<Transform2DComponent>()) {
            if(const auto *collider=object.GetComponent<EnvironmentCollider2DComponent>()){
                if(collider->visible&&tilemap_query_detail::Valid(*transform)&&OverlapShape({position,collider->shape=="Segment"?.05f:0},ColliderShape(*collider,*transform)))return id;
            }else{
                const auto delta=transform->position-position;
                if(delta.x*delta.x+delta.y*delta.y<=16)return id;
            }
        }
        for(const auto &[id,object]:objects)
            if(scene.IsActive(object.GetEntity()))
            if(const auto *ground=object.GetComponent<PlaygroundComponent>())
                if(const auto *transform=object.GetComponent<Transform2DComponent>();transform && PlaygroundContains(position,*transform,*ground))return id;
        return {};
    }
    // Uses the same assigned assets, transforms and Playground clipping as Render.
    // Visibility is visual only; tile layer collision flags/mask govern queries.
    std::optional<TilemapQueryHit> RaycastEnvironment(Vector2f origin,Vector2f direction,float distance,
                                                     std::uint32_t layerMask=~std::uint32_t{}) override {
        std::optional<TilemapQueryHit> nearest;
        VisitEnvironment([&](SceneObjectId id,const Tilemap2D &map,const Transform2DComponent &transform,
                             std::optional<Rectanglef> bounds){
            auto hit=RaycastTilemap(map,transform,origin,direction,distance,layerMask,id,bounds);
            if(hit&&(!nearest||hit->distance<nearest->distance))nearest=hit;
        });
        VisitShapes(layerMask,[&](SceneObjectId id,const auto &collider,const auto &transform){
            if(auto hit=RaycastShape(origin,direction,distance,ColliderShape(collider,transform));hit&&(!nearest||hit->fraction*distance<nearest->distance))
                nearest=ShapeResult(id,collider,*hit,hit->fraction*distance);
        });
        return nearest;
    }
    std::vector<TilemapQueryHit> OverlapEnvironment(Circle2D circle,std::uint32_t layerMask=~std::uint32_t{}) override {
        std::vector<TilemapQueryHit> hits;
        VisitEnvironment([&](SceneObjectId id,const Tilemap2D &map,const Transform2DComponent &transform,
                             std::optional<Rectanglef> bounds){
            auto found=OverlapCircleTilemap(map,transform,circle,layerMask,id,bounds);
            hits.insert(hits.end(),found.begin(),found.end());
        });
        VisitShapes(layerMask,[&](SceneObjectId id,const auto &collider,const auto &transform){
            if(auto hit=OverlapShape(circle,ColliderShape(collider,transform)))hits.push_back(ShapeResult(id,collider,*hit,Length(circle.center-hit->point)));
        });
        return hits;
    }
    bool HasEnvironmentClearance(Circle2D circle,std::uint32_t mask=~std::uint32_t{}) override {
        return Finite(circle.center)&&std::isfinite(circle.radius)&&circle.radius>=0&&OverlapEnvironment(circle,mask).empty();
    }
    std::optional<TilemapQueryHit> SweepEnvironment(Circle2D circle,Vector2f displacement,
        std::uint32_t mask=~std::uint32_t{},SceneObjectId ignoredObject=0,bool ignoreMoving=false) override {
        std::optional<TilemapQueryHit> nearest;
        VisitEnvironment([&](SceneObjectId id,const Tilemap2D &map,const Transform2DComponent &transform,std::optional<Rectanglef> bounds){
            if(id==ignoredObject||(ignoreMoving&&obstacleMotion.contains(id)))return;
            auto hit=SweepCircleTilemap(map,transform,circle,displacement,mask,id,bounds);
            if(hit&&(!nearest||hit->distance<nearest->distance))nearest=hit;
        });
        VisitShapes(mask,[&](SceneObjectId id,const auto &collider,const auto &transform){
            if(id==ignoredObject||(ignoreMoving&&obstacleMotion.contains(id)))return;
            if(auto hit=SweepShape(circle,displacement,ColliderShape(collider,transform));hit&&(!nearest||hit->fraction*Length(displacement)<nearest->distance))
                nearest=ShapeResult(id,collider,*hit,hit->fraction*Length(displacement));
        });return nearest;
    }
    void Start() override {playing=true;}
    void FixedUpdate(float delta) override {
        if(!playing||!std::isfinite(delta)||delta<=0)return;
        PruneDestroyedObjects();
        std::map<SceneObjectId,Transform2DComponent> previous;
        for(const auto &[id,object]:objects)if(const auto *pose=object.GetComponent<Transform2DComponent>())previous[id]=*pose;
        scene.FixedUpdate(delta);PruneDestroyedObjects();lifecycleStarted=true;obstacleMotion.clear();
        for(const auto &[id,object]:objects)if(const auto *pose=object.GetComponent<Transform2DComponent>()){
            const auto old=previous.find(id);
            if(old!=previous.end()&&old->second.rotation==pose->rotation&&old->second.scale==pose->scale&&old->second.position!=pose->position)
                obstacleMotion[id]=pose->position-old->second.position;
        }
        for(const auto &[id,object]:objects){
            if(!scene.IsActive(object.GetEntity()))continue;
            auto *body=object.GetComponent<KinematicBody2DComponent>();
            auto *transform=object.GetComponent<Transform2DComponent>();
            if(!body||!body->enabled||!transform||!std::isfinite(body->radius)||body->radius<=0)continue;
            auto remaining=body->velocity*delta;
            if(!std::isfinite(Length(remaining)))continue;
            // Relative-motion CCD also detects translating obstacles sweeping into
            // a stationary body. Rotation/scale edits use their current shape.
            const auto respond=[&](const ShapeHit2D &hit,Vector2f movement){
                if(LengthSquared(hit.normal)==0){remaining={};body->velocity={};return;}
                const float left=1-hit.fraction;
                const float into=ShapeDot((remaining-movement)*left,hit.normal);
                if(into<0)remaining-=hit.normal*into;
                const float speed=ShapeDot(body->velocity-movement/delta,hit.normal);
                if(speed<0)body->velocity-=hit.normal*speed;
            };
            VisitShapes(std::uint32_t(body->layerMask),[&](SceneObjectId target,const auto &collider,const auto &pose){
                if(target==id||!obstacleMotion.contains(target))return;
                const auto movement=obstacleMotion.at(target);
                if(auto hit=SweepShape({transform->position,body->radius},remaining-movement,ColliderShape(collider,previous.at(target))))respond(*hit,movement);
            });
            VisitEnvironment([&](SceneObjectId target,const auto &map,const auto &pose,auto bounds){
                if(target==id||!obstacleMotion.contains(target))return;
                const auto movement=obstacleMotion.at(target),relative=remaining-movement;
                if(auto hit=SweepCircleTilemap(map,previous.at(target),{transform->position,body->radius},relative,std::uint32_t(body->layerMask),target,bounds))
                    respond({Length(relative)>0?hit->distance/Length(relative):0,hit->point,hit->normal},movement);
            });
            const auto motion=MoveCircle({transform->position,body->radius},remaining,body->velocity,[&](auto circle,auto delta)->std::optional<ShapeHit2D>{
                const auto hit=SweepEnvironment(circle,delta,std::uint32_t(body->layerMask),id,true);
                return hit?std::optional{ShapeHit2D{hit->distance/Length(delta),hit->point,hit->normal}}:std::nullopt;
            });
            transform->position=motion.position;body->velocity=motion.velocity;
        }
        UpdateContacts(previous);
    }
    void Render(RenderContext &context) override {
        PruneDestroyedObjects();
        for(const auto &[id,object]:objects)
            if(scene.IsActive(object.GetEntity()))
            if(const auto *ground=object.GetComponent<PlaygroundComponent>())
                if(const auto *transform=object.GetComponent<Transform2DComponent>()){
                    auto display=*ground;RenderState state;Vector2f uv;
                    if(const auto *material=visuals.ResolveMaterial(ground->material);material&&material->error.empty()){
                        display.color=MultiplyTint(ground->color,material->value.tint);state=material->State();
                        uv={material->textureSize.x*material->value.uvScale.x,material->textureSize.y*material->value.uvScale.y};
                    }
                    DrawPlayground(context.GetCanvas(),*transform,display,state,uv);
                }
        for(const auto &[id,object]:objects) {
            if(!object.IsValid()||!scene.IsActive(object.GetEntity()))continue;
            const auto *tiles=object.GetComponent<TilemapComponent>();
            const auto *transform=object.GetComponent<Transform2DComponent>();
            if(!tiles || !tiles->visible || !transform)continue;
            const auto *resource=tilemaps.Resolve(tiles->asset);
            if(!resource || !resource->map)continue;
            const Tileset2D *atlas=nullptr;const VisualAssetModule::MaterialResource *material=nullptr;
            if(!resource->map->Tileset().empty()){
                const auto *resolved=visuals.ResolveTileset({resource->map->Tileset()});
                if(!resolved||!resolved->error.empty())continue;
                atlas=&resolved->value;material=visuals.ResolveMaterial(atlas->material);
            }
            const auto *geometry=tilemaps.Geometry(tiles->asset,atlas);
            if(const auto *ground=object.GetComponent<PlaygroundComponent>())
                ClipTilemapGeometry(geometry->vertices,{{},ground->Size()},transformedVertices);
            else transformedVertices=geometry->vertices;
            if(material)for(auto &vertex:transformedVertices)vertex.color=MultiplyTint(vertex.color,material->value.tint);
            for(auto &vertex:transformedVertices)vertex.position=PlaygroundToWorld(vertex.position,*transform);
            context.GetCanvas().Draw(transformedVertices.data(),transformedVertices.size(),PrimitiveTopology::Triangles,material?material->State():RenderState{});
        }
        for(const auto &[id,object]:objects){
            if(!object.IsValid()||!scene.IsActive(object.GetEntity()))continue;
            const auto *collider=object.GetComponent<EnvironmentCollider2DComponent>();
            const auto *transform=object.GetComponent<Transform2DComponent>();
            if(!collider||!collider->visible||!transform||!tilemap_query_detail::Valid(*transform))continue;
            const auto shape=ColliderShape(*collider,*transform);std::vector<Vertex2D> vertices;
            if(shape.segment)for(int i:{0,1})vertices.push_back({shape.points[i],collider->color,{}});
            else for(int i:{0,1,2,0,2,3})vertices.push_back({shape.points[i],collider->color,{}});
            context.GetCanvas().Draw(vertices.data(),vertices.size(),shape.segment?PrimitiveTopology::Lines:PrimitiveTopology::Triangles);
        }
        UpdateVisibility(context);
    }
    void Stop() override {playing=false;}
    void Reset() override {
        auto saved=authored;ClearNotifications();scene.Clear();objects.clear();authored.clear();
        std::vector<SceneObjectData> values;for(const auto &[id,data]:saved)values.push_back(data);
        SynchronizeScene(values);
    }
    void Unload() override {ClearNotifications();visuals.Unload();tilemaps.Unload();transformedVertices.clear();Stop();scene.Clear();objects.clear();authored.clear();types.clear();}
private:
    using ContactKey=std::tuple<SceneObjectId,SceneObjectId,bool>;
    struct ContactSnapshot { SceneObject first,second; CollisionEvent2D contact; };
    std::map<ContactKey,ContactSnapshot> contacts;
    struct VisibilitySnapshot { SceneObject object; bool visible{}; };
    std::map<SceneObjectId,VisibilitySnapshot> visibility;
    bool lifecycleStarted{};
    void ClearNotifications(){contacts.clear();visibility.clear();lifecycleStarted=false;}
    void PruneDestroyedObjects(){
        std::erase_if(objects,[](const auto &entry){return !entry.second.IsValid();});
        std::erase_if(visibility,[](const auto &entry){return !entry.second.object.IsValid();});
    }
    void UpdateContacts(const std::map<SceneObjectId,Transform2DComponent> &previous) {
        std::map<ContactKey,ContactSnapshot> next;
        for(const auto &[id,object]:objects){
            const auto *body=object.GetComponent<KinematicBody2DComponent>();
            const auto *pose=object.GetComponent<Transform2DComponent>();
            if(!body||!body->enabled||!pose||!scene.IsActive(object.GetEntity()))continue;
            const auto add=[&](SceneObjectId target,Vector2f point,Vector2f normal,bool trigger){
                if(target==id||!objects.contains(target))return;
                if(id<target)next.try_emplace(ContactKey{id,target,trigger},ContactSnapshot{object,objects.at(target),{objects.at(target).GetEntity(),point,normal}});
                else next.try_emplace(ContactKey{target,id,trigger},ContactSnapshot{objects.at(target),object,{object.GetEntity(),point,-normal}});
            };
            // A small contact tolerance retains touching contacts after swept movement.
            for(const auto &hit:OverlapEnvironment({pose->position,body->radius+0.0001f},std::uint32_t(body->layerMask)))
                add(hit.objectId,hit.point,hit.normal,false);
            for(const auto &[target,other]:objects){
                const auto *shape=other.GetComponent<EnvironmentCollider2DComponent>();
                const auto *transform=other.GetComponent<Transform2DComponent>();
                if(target==id||!shape||!shape->enabled||!shape->trigger||!transform||
                    !(std::uint32_t(shape->layerMask)&std::uint32_t(body->layerMask))||!scene.IsActive(other.GetEntity()))continue;
                const auto geometry=ColliderShape(*shape,*transform);
                auto hit=OverlapShape({pose->position,body->radius},geometry);
                if(!hit && previous.contains(id))hit=SweepShape({previous.at(id).position,body->radius},pose->position-previous.at(id).position,geometry);
                if(hit)add(target,hit->point,hit->normal,true);
            }
        }
        const auto notify=[&](const ContactKey &key,const ContactSnapshot &hit,bool entering){
            if(hit.first.IsValid()&&hit.second.IsValid()) {
                scene.NotifyContact(hit.first.GetEntity(),hit.contact,std::get<2>(key),entering);
                if(hit.second.IsValid()&&hit.first.IsValid())scene.NotifyContact(hit.second.GetEntity(),
                    {hit.first.GetEntity(),hit.contact.point,-hit.contact.normal},std::get<2>(key),entering);
            } else if(!entering && hit.first.IsValid()) {
                auto event=hit.contact;event.other=ecs::InvalidEntity;
                scene.NotifyContact(hit.first.GetEntity(),event,std::get<2>(key),false);
            } else if(!entering && hit.second.IsValid()) {
                scene.NotifyContact(hit.second.GetEntity(),{ecs::InvalidEntity,hit.contact.point,-hit.contact.normal},std::get<2>(key),false);
            }
        };
        auto old=std::move(contacts);contacts=next;
        for(const auto &[key,hit]:old)if(!next.contains(key)||next.at(key).first!=hit.first||next.at(key).second!=hit.second)notify(key,hit,false);
        for(const auto &[key,hit]:next)if(!old.contains(key)||old.at(key).first!=hit.first||old.at(key).second!=hit.second)notify(key,hit,true);
        PruneDestroyedObjects();
    }
    void UpdateVisibility(const RenderContext &context){
        if(!playing||!lifecycleStarted)return;
        const auto half=context.GetCameraSize()*.5f,center=context.GetCameraCenter();
        const Rectanglef view{center-half,half*2.f};
        std::vector<std::pair<SceneObject,bool>> changed;
        for(const auto &[id,object]:objects){
            const auto *pose=object.GetComponent<Transform2DComponent>();
            if(!pose||!tilemap_query_detail::Valid(*pose)||!scene.HasStartedBehaviour(object.GetEntity()))continue;
            bool drawn=false;Vector2f low{},high{};
            const auto include=[&](Vector2f point){if(!drawn){low=high=point;drawn=true;}else{low.x=std::min(low.x,point.x);low.y=std::min(low.y,point.y);high.x=std::max(high.x,point.x);high.y=std::max(high.y,point.y);}};
            if(const auto *shape=object.GetComponent<EnvironmentCollider2DComponent>();shape&&shape->visible)
                {const auto geometry=ColliderShape(*shape,*pose);
                for(std::size_t i=0;i<(geometry.segment?2u:4u);++i)include(geometry.points[i]);}
            if(const auto *ground=object.GetComponent<PlaygroundComponent>()){
                const auto size=ground->Size();for(auto point:{Vector2f{},Vector2f{size.x,0},size,Vector2f{0,size.y}})include(PlaygroundToWorld(point,*pose));
            }
            if(const auto *tiles=object.GetComponent<TilemapComponent>();tiles&&tiles->visible)
                if(const auto *resource=tilemaps.Resolve(tiles->asset);resource&&resource->map){
                    const auto &map=*resource->map;const Vector2f size{map.Columns()*map.CellSize(),map.Rows()*map.CellSize()};
                    for(auto point:{Vector2f{},Vector2f{size.x,0},size,Vector2f{0,size.y}})include(PlaygroundToWorld(map.Origin()+point,*pose));
                }
            const bool visible=drawn&&scene.IsActive(object.GetEntity())&&high.x>=view.position.x&&high.y>=view.position.y&&low.x<=view.position.x+view.size.x&&low.y<=view.position.y+view.size.y;
            const auto old=visibility.find(id);
            if((old==visibility.end()||old->second.object!=object)?visible:old->second.visible!=visible)changed.emplace_back(object,visible);
            visibility[id]={object,visible};
        }
        for(const auto &[object,visible]:changed)if(object.IsValid())scene.NotifyVisibility(object.GetEntity(),visible);
        PruneDestroyedObjects();
    }
    static TilemapQueryHit ShapeResult(SceneObjectId id,const EnvironmentCollider2DComponent &collider,const ShapeHit2D &hit,float distance){
        return {id,0,{-1,-1},0,hit.point,hit.normal,distance,collider.shape=="Segment"?EnvironmentGeometryKind::Segment:EnvironmentGeometryKind::Box};
    }
    template<class Visitor> void VisitShapes(std::uint32_t mask,Visitor visitor){
        for(const auto &[id,object]:objects){
            if(!object.IsValid()||!scene.IsActive(object.GetEntity()))continue;
            const auto *collider=object.GetComponent<EnvironmentCollider2DComponent>();
            const auto *transform=object.GetComponent<Transform2DComponent>();
            if(collider&&collider->enabled&&!collider->trigger&&(std::uint32_t(collider->layerMask)&mask)&&transform&&tilemap_query_detail::Valid(*transform))visitor(id,*collider,*transform);
        }
    }
    template<class Visitor> void VisitEnvironment(Visitor visitor) {
        for(const auto &[id,object]:objects){
            if(!object.IsValid()||!scene.IsActive(object.GetEntity()))continue;
            const auto *tiles=object.GetComponent<TilemapComponent>();
            const auto *transform=object.GetComponent<Transform2DComponent>();
            if(!tiles||!transform)continue;
            const auto *resource=tilemaps.Resolve(tiles->asset);
            if(!resource||!resource->map)continue;
            std::optional<Rectanglef> bounds;
            if(const auto *ground=object.GetComponent<PlaygroundComponent>())bounds=Rectanglef{{},ground->Size()};
            visitor(id,*resource->map,*transform,bounds);
        }
    }
    void ApplyData(SceneObject object,const SceneObjectData &data) const {
        EntityRegistry::Restore(object,data,registry);
    }
    TilemapAssetModule tilemaps;
    VisualAssetModule visuals;
    std::vector<Vertex2D> transformedVertices;
    std::string name;
    ComponentRegistry registry;
    EntityRegistry entities;
    BehaviourScene scene;
    std::map<std::string,std::function<void(SceneObject)>> behaviours;
    std::vector<SceneObjectTypeDescriptor> types;
    std::map<SceneObjectId,SceneObject> objects;
    std::map<SceneObjectId,SceneObjectData> authored;
    std::map<SceneObjectId,Vector2f> obstacleMotion;
    bool playing{};
};
}
