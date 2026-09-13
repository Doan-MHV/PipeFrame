#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <iostream>
#include <PipeFrame/Project/ProjectRuntimeLibrary.h>
using namespace pipeframe;
struct SoldierSettings {
    double speed{2};
    static auto Schema() {
        return ComponentSchema<SoldierSettings>("project.SoldierSettings","Soldier settings")
            .Editable({.key="speed",.displayName="Speed",.kind=PropertyKind::Number,.defaultValue=2.0,.minimum=0,.maximum=10},&SoldierSettings::speed);
    }
};
// Extension-mechanics fixture only: no Pezzza combat or soldier AI claim.
struct SoldierBehaviour : Behaviour {
    void FixedUpdate(float dt) override {
        auto *settings=GetComponent<SoldierSettings>();auto *transform=GetComponent<Transform2DComponent>();
        if(settings && transform)transform->position.x+=static_cast<float>(settings->speed)*dt;
    }
};
struct RegisteredSoldier : EntityArchetype {
    void Build(ecs::World &world,ecs::Entity entity) const override {
        world.Add<Transform2DComponent>(entity);world.Add<SoldierSettings>(entity);
    }
    void OnInstantiated(const SceneObject &object) const override {object.Attach<SoldierBehaviour>();}
};
struct OtherEntity : EntityArchetype {
    void Build(ecs::World &world,ecs::Entity entity) const override {world.Add<Transform2DComponent>(entity);}
};
struct SpawnWithArguments : EntityArchetype {
    explicit SpawnWithArguments(float x):x(x){}
    float x;
    void Build(ecs::World &world,ecs::Entity entity) const override {
        world.Add<Transform2DComponent>(entity).position.x=x;
    }
};
struct RecordingSurface final:RenderSurface {
    std::size_t triangleVertices{};
    Canvas GetCanvas()override{return {this,[](void *self,std::span<const Vertex2D> vertices,PrimitiveTopology topology,const RenderState &){
        if(topology==PrimitiveTopology::Triangles)static_cast<RecordingSurface *>(self)->triangleVertices+=vertices.size();
    }};}
    Vector2u GetSize()const override{return {800,600};}
    void SetScreenSize(Vector2u)override{}
    void BeginWorld(const Camera2D &)override{}
    void BeginScreen()override{}
    Rectanglei Viewport(const Camera2D &)const override{return {{0,0},{800,600}};}
    Vector2f PixelToWorld(Vector2i point,const Camera2D &)const override{return {float(point.x),float(point.y)};}
    Vector2i WorldToPixel(Vector2f point,const Camera2D &)const override{return {int(point.x),int(point.y)};}
};
void Check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv){try {
    SceneProjectRuntime playground("Blank project");
    auto ground=playground.CreateDefaultObject(PlaygroundEntityTypeId);ground.id=90;
    std::vector<SceneObjectData> groundScene{ground};playground.SynchronizeScene(groundScene);
    auto groundEntity=playground.ResolveSceneObject(90);
    Check(groundEntity.GetComponent<PlaygroundComponent>()!=nullptr,"Built-in recipe attaches actual Playground component");
    Check(playground.HitTest({300,200})==90 && !playground.HitTest({321,200}),"Playground selectable across its full bounds");
    const auto exposed=playground.InspectObjectComponents(90);
    Check(exposed && exposed->size()==3,"Transform and Playground schemas exposed through standard inspector path");
    auto schema=PlaygroundComponent::Schema();PlaygroundComponent settings;std::string validation;
    Check(!schema.Apply(settings,{{"rows",std::int64_t{0}}},validation)&&settings.rows==24,"Invalid dimensions leave component unchanged");
    Transform2DComponent pose;pose.position={10,20};pose.rotation=1.57079632679f;pose.scale={2,3};
    const auto inside=PlaygroundToWorld({40,30},pose);
    Check(PlaygroundContains(inside,pose,settings) && !PlaygroundContains(PlaygroundToWorld({400,30},pose),pose,settings),
          "Picking respects rotation and scale");
    struct DrawStats {std::size_t triangles{},lines{};} stats;
    Canvas canvas(&stats,[](void *target,std::span<const Vertex2D> vertices,PrimitiveTopology topology,const RenderState &){
        auto &stats=*static_cast<DrawStats *>(target);
        if(topology==PrimitiveTopology::Triangles)stats.triangles+=vertices.size();
        if(topology==PrimitiveTopology::Lines)stats.lines+=vertices.size();
    });
    DrawPlayground(canvas,pose,settings);
    Check(stats.triangles==6 && stats.lines==116,"Ground and grid draw through neutral engine Canvas");
    const auto assetRoot=std::filesystem::temp_directory_path()/"pipeframe-tilemap-runtime-test";
    std::filesystem::remove_all(assetRoot);std::filesystem::create_directories(assetRoot);
    assets::AssetDatabase database;std::string assetError;Check(database.Open(assetRoot,&assetError),"Open tilemap database");
    Tilemap2D painted(4,4);painted.DefineTile({1,{200,50,30,255},true});painted.AddLayer("Terrain");painted.SetTile(0,{1,1},1);
    const auto source=assetRoot/"painted.pftilemap";{std::ofstream out(source);Check(TilemapSerializer::Save(painted,out),"Save tilemap");}
    const auto asset=database.ImportNow({source},&assetError);Check(asset.has_value(),"Import tilemap");
    ServiceRegistry services;services.Provide(database);ProjectRuntimeContext resourceContext;resourceContext.services=&services;
    TilemapAssetModule module;Check(module.Load(resourceContext,assetError),"Load shared tilemap module");
    const auto *resource=module.Resolve({*asset});Check(resource && resource->map && resource->geometry.vertices.size()==6,"Resolve cached asset into runtime geometry");
    const auto cachedRevision=resource->revision;
    painted.SetTile(0,{2,1},1);
    {std::ofstream out(assetRoot/database.Find(*asset)->sourcePath);TilemapSerializer::Save(painted,out);}
    Check(module.Resolve({*asset})->geometry.vertices.size()==6,"Unimported disk changes do not replace cached geometry");
    Check(database.Reimport(*asset,&assetError),"Reimport changed tilemap");
    resource=module.Resolve({*asset});Check(resource->revision>cachedRevision && resource->geometry.vertices.size()==12,"Asset revision refreshes geometry");
    Check(module.Resolve({"missing"})->map==std::nullopt,"Missing assets produce a diagnostic state");
    const auto indexedSource=assetRoot/database.Find(*asset)->sourcePath;
    const auto parkedSource=assetRoot/"parked-map";
    const auto readyRevision=database.Find(*asset)->revision;
    std::filesystem::rename(indexedSource,parkedSource);database.RefreshMissingStates();
    Check(!module.Resolve({*asset})->map,"Missing source invalidates decoded runtime map");
    std::filesystem::rename(parkedSource,indexedSource);database.RefreshMissingStates();
    Check(database.Find(*asset)->revision==readyRevision && module.Resolve({*asset})->map.has_value(),
          "Restored source recovers without requiring another reimport or revision");
    Check(playground.Load(resourceContext,assetError),"Load generated-style runtime with host asset service");
    for(auto &component:groundScene[0].components)if(component.typeId==TilemapComponentTypeId)component.properties["asset"]=AssetReference{*asset};
    playground.SynchronizeScene(groundScene);
    auto surface=std::make_shared<RecordingSurface>();RenderContext renderContext(surface);playground.Render(renderContext);
    Check(surface->triangleVertices==18,"Runtime renders ground plus both painted cells from the actual attached component");
    auto sceneHit=playground.RaycastEnvironment({4,1.5f},{-1,0},10);
    Check(sceneHit&&sceneHit->objectId==90&&sceneHit->cell==GridCoordinate{2,1}&&sceneHit->distance==1,
          "Scene queries resolve assigned asset and object identity");
    auto *bounds=playground.ResolveSceneObject(90).GetComponent<PlaygroundComponent>();
    bounds->columns=2;bounds->rows=2;bounds->cellSize=1;
    surface->triangleVertices=0;playground.Render(renderContext);
    Check(surface->triangleVertices==12,"Shrinking Playground hides cells beyond its bounds");
    sceneHit=playground.RaycastEnvironment({4,1.5f},{-1,0},10);
    Check(sceneHit&&sceneHit->cell==GridCoordinate{1,1}&&sceneHit->distance==2,
          "Scene ray ignores cells clipped out by Playground");
    Check(playground.OverlapEnvironment({{2.5f,1.5f},0.2f}).empty(),"Scene overlaps honor clipped bounds");
    bounds->cellSize=.75f;
    sceneHit=playground.RaycastEnvironment({4,1.25f},{-1,0},10);
    Check(sceneHit&&sceneHit->distance==2.5f&&sceneHit->normal.x==1,"Partial tile hit begins at clipped Playground face");
    Check(!playground.RaycastEnvironment({1.5f,1.25f},{1,0},10),"Outgoing ray on clipped maximum face misses");
    Check(playground.OverlapEnvironment({{1.75f,1.25f},.2f}).empty(),"Partial tile overlap uses rendered rectangle");
    bounds->cellSize=1;
    bounds->columns=4;surface->triangleVertices=0;playground.Render(renderContext);
    Check(surface->triangleVertices==18,"Expanding Playground restores cells without modifying the shared asset");
    SceneObjectData mover;mover.id=91;mover.typeId="test.mover";
    mover.components={MakeTransform2DComponent({{0,1.1f},0,{1,1}}),
        {KinematicBody2DComponentTypeId,1,{{"velocity",Vector2f{100,2}},{"radius",.25}}}};
    auto movingScene=groundScene;movingScene.push_back(mover);playground.SynchronizeScene(movingScene);
    playground.Start();playground.FixedUpdate(.1f);
    auto moving=playground.ResolveSceneObject(91);
    auto *movingTransform=moving.GetComponent<Transform2DComponent>();
    auto *movingBody=moving.GetComponent<KinematicBody2DComponent>();
    Check(movingTransform&&std::abs(movingTransform->position.x-.75f)<.001f&&
          std::abs(movingTransform->position.y-1.3f)<.001f,"Fast kinematic body stops at wall and slides without tunneling");
    Check(movingBody->velocity.x==0&&movingBody->velocity.y==2,"Response removes inward velocity and retains tangent");
    playground.Stop();auto stopped=movingTransform->position;playground.FixedUpdate(.1f);
    Check(movingTransform->position==stopped,"Paused runtime does not move body");
    playground.Reset();moving=playground.ResolveSceneObject(91);
    Check(moving.GetComponent<Transform2DComponent>()->position==Vector2f{0,1.1f},"Reset restores authored transform");
    moving.GetComponent<KinematicBody2DComponent>()->layerMask=2;
    playground.Start();playground.FixedUpdate(.1f);
    Check(moving.GetComponent<Transform2DComponent>()->position.x==10,"Collision mask permits passage through excluded terrain");
    playground.Stop();playground.SynchronizeScene(groundScene);
    auto *queryTransform=playground.ResolveSceneObject(90).GetComponent<Transform2DComponent>();
    queryTransform->position={10,20};
    Check(!playground.RaycastEnvironment({4,1.5f},{-1,0},10),"Moving entity updates query placement immediately");
    Check(playground.RaycastEnvironment({14,21.5f},{-1,0},10).has_value(),"Moved asset remains queryable");
    painted.SetTile(0,{1,1},0);painted.SetTile(0,{2,1},0);
    {std::ofstream out(assetRoot/database.Find(*asset)->sourcePath);TilemapSerializer::Save(painted,out);}
    Check(database.Reimport(*asset,&assetError)&&!playground.RaycastEnvironment({14,21.5f},{-1,0},10),
          "Reimport invalidates query cache through same asset revision as rendering");
    playground.SynchronizeScene({});
    Check(playground.OverlapEnvironment({{11,21},10}).empty(),"Deleted scene objects leave no stale query colliders");
    playground.Unload();module.Unload();std::filesystem::remove_all(assetRoot);
    SceneProjectRuntime runtime("Fixture");runtime.RegisterComponent<SoldierSettings>();
    runtime.RegisterBehaviour<SoldierBehaviour>("project.SoldierBehaviour","Soldier behaviour");
    SceneObjectData object;object.id=42;object.typeId="project.SoldierAnt";object.name="SoldierAnt";
    object.components={{Transform2DComponentTypeId,1,{}},{"project.SoldierSettings",1,{{"speed",4.0}}},{"project.SoldierBehaviour",1,{}}};
    std::vector<SceneObjectData> objects{object};runtime.SynchronizeScene(objects);
    auto entity=runtime.ResolveSceneObject(42);Check(entity.IsValid(),"Authored object becomes a live ECS entity");
    Check(runtime.InspectObjectComponents(42)->size()==3,"Actual attached schemas appear in Inspector");
    runtime.Start();runtime.FixedUpdate(.5f);
    Check(entity.GetComponent<Transform2DComponent>()->position.x==2,"Attached generated-style behaviour receives fixed lifecycle");
    runtime.SynchronizeScene(objects);
    Check(entity.GetComponent<Transform2DComponent>()->position.x==2,"Unchanged authoring sync preserves simulation state");
    const std::vector<ProjectRuntimeComponentEdit> edits{{42,"project.SoldierSettings","speed",6.0}};
    std::string error;Check(runtime.ValidateComponentEdits(edits,error),"Live schema validates editor edit");
    Check(runtime.ApplyComponentEdits(edits,ProjectRuntimeAuthoringState::Paused),"Editor edit reaches live ECS storage");
    runtime.FixedUpdate(.5f);Check(entity.GetComponent<Transform2DComponent>()->position.x==5,"Behaviour consumes edited setting");
    auto invalid=objects;invalid[0].components[1].properties["speed"]=100.0;
    bool rejected=false;try{runtime.SynchronizeScene(invalid);}catch(const std::invalid_argument&){rejected=true;}
    Check(rejected && entity.IsValid() && entity.GetComponent<SoldierSettings>()->speed==6,"Invalid scene sync preserves live state");
    objects[0].components.pop_back();runtime.SynchronizeScene(objects);
    auto detached=runtime.ResolveSceneObject(42);runtime.FixedUpdate(1);
    Check(!entity.IsValid() && detached.GetComponent<Transform2DComponent>()->position.x==0,"Removing behaviour ends its lifecycle");
    runtime.Reset();Check(runtime.ResolveSceneObject(42).IsValid(),"Reset rebuilds authored scene");
    Check(runtime.ApplyComponentEdits(edits,ProjectRuntimeAuthoringState::Stopped),"Edit reset scene settings");
    runtime.Reset();Check(runtime.ResolveSceneObject(42).GetComponent<SoldierSettings>()->speed==6,"Reset preserves edited authored settings");
    runtime.Unload();Check(!runtime.ResolveSceneObject(42).IsValid(),"Unload clears scene handles");
    // A second archetype requires registration only, with no runtime type switch.
    SceneProjectRuntime registered("Registered archetypes");registered.RegisterComponent<SoldierSettings>();
    registered.RegisterEntity<RegisteredSoldier>({"fixture.soldier","Soldier",{}, {Transform2DComponentTypeId,"project.SoldierSettings"}});
    registered.RegisterEntity<OtherEntity>({"fixture.other","Other",{}, {Transform2DComponentTypeId}});
    Check(registered.GetSceneObjectTypes().size()==4,"Entity registration supplies editor entries including the built-in environment obstacle");
    auto soldier=registered.CreateDefaultObject("fixture.soldier");soldier.id=1;
    auto other=registered.CreateDefaultObject("fixture.other");other.id=2;
    std::vector<SceneObjectData> registeredData{soldier,other};registered.SynchronizeScene(registeredData);
    registered.Start();registered.FixedUpdate(.5f);
    Check(registered.ResolveSceneObject(1).GetComponent<Transform2DComponent>()->position.x==1,"Registered factory attaches behaviour");
    Check(!registered.ResolveSceneObject(2).GetComponent<SoldierSettings>(),"Distinct archetype has distinct composition");
    bool duplicate=false;try{registered.RegisterEntity<OtherEntity>({"fixture.other","Other",{},{}});}catch(const std::invalid_argument&){duplicate=true;}
    Check(duplicate,"Duplicate entity registration rejected");
    registered.Reset();Check(registered.ResolveSceneObject(1).GetComponent<Transform2DComponent>()->position.x==0,"Registered reset restores authored components");
    EntityRegistry runtimeTypes;runtimeTypes.Register<SpawnWithArguments>({"fixture.spawn","Spawn",{},{}},false);
    BehaviourScene spawnScene;
    const auto spawned=runtimeTypes.Spawn(spawnScene,SpawnWithArguments{7});
    Check(spawned.GetComponent<Transform2DComponent>()->position.x==7,"Typed spawning preserves domain constructor arguments");
    Check(runtimeTypes.Describe().empty() && runtimeTypes.Describe(true).size()==1,"Runtime-only registration is distinct from editor visibility");
    ComponentRegistry components;RegisterCommonComponents(components);components.Register(SoldierSettings::Schema());
    EntityRegistry entityTypes;entityTypes.Register<RegisteredSoldier>({"fixture.soldier","Soldier",{},{}});
    BehaviourScene shared;RegisteredEntityObjects mapped;mapped.Rebuild(shared,registeredData,entityTypes,components);
    const auto original=mapped.Resolve(1);Check(original.IsValid() && !mapped.Resolve(2).IsValid(),"Specialized runtime maps registered subset in shared scene");
    auto invalidBatch=registeredData;invalidBatch[0].components.push_back({"unknown",1,{}});
    bool invalidBatchRejected=false;try{mapped.Rebuild(shared,invalidBatch,entityTypes,components);}catch(const std::invalid_argument&){invalidBatchRejected=true;}
    Check(invalidBatchRejected && mapped.Resolve(1)==original && original.IsValid(),"Failed rebuild preserves prior entity map");
    mapped.Clear();Check(!original.IsValid(),"Engine mapping cleanup destroys registered objects");
    if(argc==3) {
        ProjectRuntimeLibrary library;Check(library.Load(argv[1],&error),error.c_str());
        auto *generated=library.GetRuntime();ProjectRuntimeContext context;context.projectDirectory=argv[2];
        Check(generated->Load(context,error),error.c_str());
        auto soldier=generated->CreateDefaultObject("project.SoldierAnt");soldier.id=1;
        std::vector<SceneObjectData> authored{soldier};generated->SynchronizeScene(authored);
        Check(generated->InspectObjectComponents(1)->size()==3,"Compiled generated registration attaches all declared schemas");
        generated->Start();generated->FixedUpdate(.016f);generated->Stop();generated->Reset();
        Check(generated->ResolveSceneObject(1).IsValid(),"Compiled generated runtime supports reset");
        generated->Unload();library.Unload();
    }
    std::cout<<"Scene project runtime lifecycle and authoring passed.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
