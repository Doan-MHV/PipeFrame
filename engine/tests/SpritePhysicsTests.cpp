#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <PipeFrame/Resources/ImageData.h>
#include <iostream>
#include <cmath>
using namespace pipeframe;
void Check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct Surface final:RenderSurface {
    std::vector<Vertex2D> vertices;int calls{};
    Canvas GetCanvas()override{return {this,[](void *self,std::span<const Vertex2D> vertices,PrimitiveTopology,const RenderState &){
        auto &s=*static_cast<Surface*>(self);s.vertices.insert(s.vertices.end(),vertices.begin(),vertices.end());++s.calls;
    }};}
    Vector2u GetSize()const override{return {800,600};}void SetScreenSize(Vector2u)override{}
    void BeginWorld(const Camera2D&)override{}void BeginScreen()override{}
    Rectanglei Viewport(const Camera2D&)const override{return {{0,0},{800,600}};}
    Vector2f PixelToWorld(Vector2i p,const Camera2D&)const override{return {float(p.x),float(p.y)};}
    Vector2i WorldToPixel(Vector2f p,const Camera2D&)const override{return {int(p.x),int(p.y)};}
};
struct Contacts:Behaviour {int enters{},triggers{};void OnCollisionEnter(const CollisionEvent2D&)override{++enters;}void OnTriggerEnter(const CollisionEvent2D&)override{++triggers;}};
int main(){try{
    const auto root=std::filesystem::temp_directory_path()/"pipeframe-sprite-physics";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root/"Assets/Textures");
    assets::AssetDatabase database;std::string error;Check(database.Open(root,&error),"Open assets");
    ImageData image({2,2},{255,255,255,255});const auto path=root/"Assets/Textures/car.png";
    Check(SaveImageData(path,image,error),"Save texture fixture");
    const auto texture=database.ImportNow({path},&error);Check(bool(texture),"Import texture");
    ServiceRegistry services;services.Provide(database);ProjectRuntimeContext context;context.services=&services;
    SceneProjectRuntime runtime("Default sprite and physics");Check(runtime.Load(context,error),"Load runtime");
    auto car=runtime.CreateDefaultObject(SpriteEntityTypeId);car.id=1;
    car.components.push_back({EnvironmentCollider2DComponentTypeId,1,{{"size",Vector2f{4,2}},{"visible",false}}});
    car.components.push_back({KinematicBody2DComponentTypeId,1,{{"velocity",Vector2f{100,0}}}});
    for(auto &component:car.components)if(component.typeId==SpriteRendererComponentTypeId){
        component.properties["texture"]=AssetReference{*texture};component.properties["size"]=Vector2f{4,2};
    }
    auto wall=runtime.CreateDefaultObject(EnvironmentObstacleEntityTypeId);wall.id=2;wall.transform.position={10,0};
    for(auto &c:wall.components)if(c.typeId==EnvironmentCollider2DComponentTypeId)c.properties["size"]=Vector2f{1,20};
    SynchronizeTransformComponent(car);SynchronizeTransformComponent(wall);
    runtime.SynchronizeScene(std::array{car,wall});
    WorldDebugDraw disabled,physicsDebug,meshDebug;
    runtime.CollectWorldDebug(disabled,{});
    runtime.CollectWorldDebug(physicsDebug,{true,false});
    runtime.CollectWorldDebug(meshDebug,{false,true});
    Check(disabled.Vertices().empty(),"Debug off produces no geometry");
    Check(physicsDebug.Vertices().size()==16,"Physics shows both boxes, including hidden collider visualization");
    Check(meshDebug.Vertices().size()==10,"Sprite mesh includes perimeter and triangle diagonal");
    Check(runtime.ResolveSceneObject(1).GetComponent<Transform2DComponent>()->position==Vector2f{},"Debug collection does not advance physics");
    auto entity=runtime.ResolveSceneObject(1);
    auto &events=entity.Attach<Contacts>();
    Check(runtime.InspectObjectComponents(1)->size()==4,"Sprite and body/collider expose component schemas");
    auto surface=std::make_shared<Surface>();RenderContext rendering(surface);runtime.Render(rendering);
    Check(surface->vertices.size()==12,"Shared runtime draws box obstacle and textured car without project rendering code");
    Check(runtime.HitTest({1.9f,.9f})==1&&!runtime.HitTest({3,3}),"Sprite picking uses its size, not an arbitrary radius");
    runtime.Start();runtime.FixedUpdate(1);
    Check(std::abs(entity.GetComponent<Transform2DComponent>()->position.x-7.5f)<.001f,"Fast car stops at box edge, not fallback circle radius");
    Check(events.enters==1,"Box collision enters once");
    runtime.FixedUpdate(.1f);Check(events.enters==1,"Resting box does not repeat collision-enter");
    runtime.Stop();runtime.SynchronizeScene({});car.transform.rotation=90;SynchronizeTransformComponent(car);
    runtime.SynchronizeScene(std::array{car,wall});runtime.Start();runtime.FixedUpdate(1);
    Check(std::abs(runtime.ResolveSceneObject(1).GetComponent<Transform2DComponent>()->position.x-8.5f)<.001f,"Rotated box uses rotated extents");
    // Same sweep works against authored tiles, not just entity obstacles.
    Tilemap2D map(20,20);map.DefineTile({1,{80,80,80,255},true});map.AddLayer("Terrain");map.SetTile(0,{10,10},1);
    auto tilePath=root/"map.pftilemap";{std::ofstream out(tilePath);TilemapSerializer::Save(map,out);}
    const auto asset=database.ImportNow({tilePath},&error);Check(bool(asset),"Import obstacle map");
    auto ground=runtime.CreateDefaultObject(PlaygroundEntityTypeId);ground.id=3;
    for(auto &c:ground.components){if(c.typeId==TilemapComponentTypeId)c.properties["asset"]=AssetReference{*asset};
        if(c.typeId==PlaygroundComponentTypeId){c.properties["columns"]=std::int64_t{20};c.properties["rows"]=std::int64_t{20};c.properties["cellSize"]=1.;}}
    car.transform.rotation=0;car.transform.position={0,10.5f};SynchronizeTransformComponent(car);runtime.Stop();runtime.SynchronizeScene({});
    runtime.SynchronizeScene(std::array{car,ground});runtime.Start();runtime.FixedUpdate(1);
    Check(std::abs(runtime.ResolveSceneObject(1).GetComponent<Transform2DComponent>()->position.x-8)<.001f,"Box stops on solid map cell");
    // Trigger detection uses swept box geometry without blocking movement.
    for(auto &c:wall.components)if(c.typeId==EnvironmentCollider2DComponentTypeId)c.properties["trigger"]=true;
    car.transform.position={};SynchronizeTransformComponent(car);runtime.Stop();runtime.SynchronizeScene({});runtime.SynchronizeScene(std::array{car,wall});
    auto &trigger=runtime.ResolveSceneObject(1).Attach<Contacts>();runtime.Start();runtime.FixedUpdate(.2f);
    Check(trigger.triggers==1&&runtime.ResolveSceneObject(1).GetComponent<Transform2DComponent>()->position.x>10,"Box crosses trigger without blocking");
    // A trigger box attached to a moving entity does not replace its fallback circle.
    auto *carCollider=runtime.ResolveSceneObject(1).GetComponent<EnvironmentCollider2DComponent>();carCollider->trigger=true;
    WorldDebugDraw triggerGeometry;runtime.CollectWorldDebug(triggerGeometry,{true,false});
    Check(triggerGeometry.Vertices().size()==80,"Trigger outline and actual fallback body circle are both visible");
    // Custom rendering worlds can reuse the same resource resolver and sprite layer.
    VisualAssetModule visuals;Check(visuals.Load(context,error),"Load visual module");
    auto *first=visuals.ResolveTexture({*texture});Check(first&&first->error.empty(),"Resolve typed texture");
    const auto revision=first->revision;auto resource=first->resources;
    Check(visuals.ResolveTexture({*texture})->resources==resource,"Repeated resolve reuses GPU resources");
    image=ImageData({4,2},{255,0,0,255});Check(SaveImageData(path,image,error)&&database.Reimport(*texture,&error),"Reimport changed texture");
    Check(visuals.ResolveTexture({*texture})->revision>revision&&visuals.ResolveTexture({*texture})->size.x==4,"Reimport refreshes texture dimensions/resources");
    SpriteRenderLayer layer(visuals);SpriteRendererComponent sprite;sprite.texture={*texture};sprite.tint={255,0,0,255};
    sprite.sortingOrder=2;layer.Submit({},sprite);sprite.sortingOrder=1;sprite.tint={0,255,0,255};layer.Submit({},sprite);
    surface->vertices.clear();surface->calls=0;layer.Render(surface->GetCanvas());
    Check(surface->calls==1&&surface->vertices.size()==12&&surface->vertices[0].color.green==255,"Sorted adjacent sprites share one batch");
    runtime.Unload();visuals.Unload();std::filesystem::remove_all(root);
    std::cout<<"Sprite and box-body authoring passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
