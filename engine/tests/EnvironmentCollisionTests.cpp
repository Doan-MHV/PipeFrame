#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <PipeFrame/Editor/TilemapAssetEditor.h>
#include <iostream>
using namespace pipeframe;
void Check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
bool Near(float a,float b){return std::abs(a-b)<1e-4f;}
struct TranslateObstacle:Behaviour{void FixedUpdate(float dt)override{GetComponent<Transform2DComponent>()->position.x+=10*dt;}};
struct Surface:RenderSurface{
    std::vector<Vertex2D> vertices;
    Canvas GetCanvas()override{return {this,[](void *p,std::span<const Vertex2D> v,PrimitiveTopology,const RenderState &){auto &out=static_cast<Surface *>(p)->vertices;out.insert(out.end(),v.begin(),v.end());}};}
    Vector2u GetSize()const override{return {800,600};}void SetScreenSize(Vector2u)override{}
    void BeginWorld(const Camera2D &)override{}void BeginScreen()override{}
    Rectanglei Viewport(const Camera2D &)const override{return {{0,0},{800,600}};}
    Vector2f PixelToWorld(Vector2i p,const Camera2D &)const override{return {float(p.x),float(p.y)};}
    Vector2i WorldToPixel(Vector2f p,const Camera2D &)const override{return {int(p.x),int(p.y)};}
};
int main(){try{
    const auto box=AxisAlignedBox({2,-1},{3,1});
    auto hit=SweepShape({{0,0},.5f},{10,0},box);Check(hit&&Near(hit->fraction,.15f)&&hit->normal==Vector2f{-1,0},"Fast circle/box time and normal");
    Check(!SweepShape({{1.5f,0},.5f},{-1,0},box),"Touching body can move away");
    Check(!SweepShape({{1.5f,0},.5f},{0,.2f},box),"Touching body slides tangentially");
    Check(!RaycastShape({0,0},{0,0},10,box)&&!RaycastShape({0,0},{1,0},1,box),"Zero direction and range miss");
    hit=RaycastShape({2.5f,0},{1,0},10,box);Check(hit&&hit->fraction==0&&hit->normal==Vector2f{},"Inside ray begins at zero with no entry normal");
    Check(!RaycastShape({2,0},{-1,0},10,box),"Outgoing box face ray misses");
    hit=SweepShape({{0,2},.5f},{4,-2},box);Check(hit&&hit->normal.x<0&&hit->normal.y>0,"Rounded corner sweep normal");
    EnvironmentShape2D segment{{Vector2f{2,-1},Vector2f{2,1},{2,1},{2,-1}},true};
    hit=SweepShape({{0,0},.25f},{5,0},segment);Check(hit&&Near(hit->fraction,.35f),"Two-sided segment blocks circle");
    hit=SweepShape({{4,0},.25f},{-5,0},segment);Check(hit&&hit->normal.x==1,"Segment opposite side normal");
    Check(OverlapShape({{2,1.25f},.25f},segment).has_value(),"Segment endpoint overlap is inclusive");
    hit=RaycastShape({2,-3},{0,1},10,segment);Check(hit&&Near(hit->fraction,.2f),"Collinear ray returns nearest segment endpoint");
    EnvironmentCollider2DComponent collider;Transform2DComponent pose;pose.position={10,20};pose.rotation=.4f;pose.scale={-2,3};
    auto rotated=ColliderShape(collider,pose);Check(OverlapShape({{10,20},0},rotated).has_value(),"Mirrored, rotated, nonuniform box preserves inside semantics");
    Tilemap2D chunks(96,64);chunks.AddLayer("Walls");chunks.DefineTile({1,{100,100,100,255},true});
    TilemapChunkCache cache;cache.Update(chunks);Check(cache.RebuiltChunks()==6,"Initial render chunks");
    chunks.SetTile(0,{40,2},1);cache.Update(chunks);Check(cache.RebuiltChunks()==1,"One painted chunk rebuilt");
    chunks.AddDataLayer("moisture");chunks.SetData("moisture",{1,1},.5);cache.Update(chunks);Check(cache.RebuiltChunks()==0,"Numeric brush data does not rebuild visual geometry");
    auto changed=chunks;changed.SetTile(0,{40,2},0);changed.SetTile(0,{80,2},1);cache.Update(changed,nullptr,true);Check(cache.RebuiltChunks()==2,"Reimport refreshes only changed chunks");
    std::vector<Vertex2D> reused;reused.reserve(100);cache.Flatten(reused);const auto *storage=reused.data();
    changed.SetTile(0,{81,2},1);cache.Update(changed);cache.Flatten(reused);
    Check(reused.data()==storage&&reused.size()==12,"Paint reuses host geometry capacity and includes the new tile");
    const auto expected=BuildTilemapGeometry(changed,{{0,0},{95,63}});
    Check(reused.size()==expected.vertices.size(),"Reusable flatten preserves geometry size");
    for(std::size_t i=0;i<reused.size();++i)Check(reused[i].position==expected.vertices[i].position&&reused[i].color==expected.vertices[i].color&&reused[i].textureCoordinate==expected.vertices[i].textureCoordinate,"Reusable flatten preserves positions, colors and UVs");
    changed.SetTile(0,{80,2},0);changed.SetTile(0,{81,2},0);cache.Update(changed);cache.Flatten(reused);
    Check(reused.empty()&&reused.data()==storage,"Erase clears stale geometry without freeing reusable capacity");
    const auto root=std::filesystem::temp_directory_path()/"pipeframe-environment-collision";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root);
    assets::AssetDatabase database;std::string error;Check(database.Open(root,&error),"Open independent project");
    Tilemap2D map(16,16);map.AddLayer("Walls");map.DefineTile({1,{100,100,100,255},true});map.SetTile(0,{3,6},1);
    Check(!RaycastTilemap(map,{}, {3,6.5f},{-1,0},20),"Outgoing tile face matches primitive box semantics");
    {std::ofstream out(root/"Maze.pftilemap");TilemapSerializer::Save(map,out);}
    auto asset=database.ImportNow({root/"Maze.pftilemap"},&error);Check(bool(asset),"Import authored maze");
    ServiceRegistry services;services.Provide(database);ProjectRuntimeContext context;context.services=&services;
    SceneProjectRuntime runtime("Independent collision fixture");Check(runtime.Load(context,error),"Load fixture");
    auto ground=runtime.CreateDefaultObject(PlaygroundEntityTypeId);ground.id=1;
    for(auto &c:ground.components)if(c.typeId==TilemapComponentTypeId)c.properties["asset"]=AssetReference{*asset};
    auto obstacle=runtime.CreateDefaultObject(EnvironmentObstacleEntityTypeId);obstacle.id=2;obstacle.transform.position={5,2};
    for(auto &c:obstacle.components){if(c.typeId==EnvironmentCollider2DComponentTypeId)c.properties["size"]=Vector2f{1,4};if(c.typeId==Transform2DComponentTypeId)c.properties["position"]=Vector2f{5,2};}
    SceneObjectData body;body.id=3;body.typeId="fixture.body";body.transform.position={0,2};body.components={{Transform2DComponentTypeId,1,{{"position",Vector2f{0,2}}}},{KinematicBody2DComponentTypeId,1,{{"velocity",Vector2f{100,0}},{"radius",.25}}}};
    std::vector<SceneObjectData> scene{ground,obstacle,body};runtime.SynchronizeScene(scene);
    auto ray=runtime.RaycastEnvironment({0,2},{1,0},20);Check(ray&&ray->objectId==2&&ray->geometry==EnvironmentGeometryKind::Box&&Near(ray->distance,4.5f),"Authored obstacle identity and world-unit ray");
    Check(!runtime.RaycastEnvironment({0,2},{1,0},20,2),"Obstacle mask exclusion");
    Check(!runtime.HasEnvironmentClearance({{4.25f,2},.25f})&&runtime.HasEnvironmentClearance({{4,2},.2f}),"Clearance distinguishes touching and separated");
    auto surface=std::make_shared<Surface>();RenderContext render(surface);runtime.Render(render);
    Check(std::ranges::any_of(surface->vertices,[](auto v){return v.position==Vector2f{4.5f,0};}),"Rendered corner matches queried box boundary");
    runtime.Start();runtime.FixedUpdate(.1f);
    auto actor=runtime.ResolveSceneObject(3);Check(Near(actor.GetComponent<Transform2DComponent>()->position.x,4.25f),"Independent body collides with authored box");
    auto wall=runtime.ResolveSceneObject(2);wall.GetComponent<Transform2DComponent>()->position={0,2};wall.Attach<TranslateObstacle>();
    actor.GetComponent<Transform2DComponent>()->position={5,2};actor.GetComponent<KinematicBody2DComponent>()->velocity={};
    runtime.FixedUpdate(1);Check(Near(actor.GetComponent<Transform2DComponent>()->position.x,10.75f),"Fast translating obstacle pushes stationary body without tunneling");
    ray=runtime.RaycastEnvironment({8,2},{1,0},20);Check(ray&&ray->objectId==2&&Near(ray->distance,1.5f),"Queries follow moved obstacle");
    auto *shapeComponent=wall.GetComponent<EnvironmentCollider2DComponent>();
    shapeComponent->visible=false;Check(runtime.RaycastEnvironment({8,2},{1,0},20).has_value(),"Hidden geometry retains collision");
    shapeComponent->enabled=false;Check(!runtime.RaycastEnvironment({8,2},{1,0},20),"Disabled collider is excluded from queries");
    shapeComponent->enabled=true;shapeComponent->shape="Segment";shapeComponent->start={0,-2};shapeComponent->end={0,2};
    ray=runtime.RaycastEnvironment({8,2},{1,0},20);Check(ray&&ray->geometry==EnvironmentGeometryKind::Segment&&Near(ray->distance,2),"Scene segment shares query identity and transform");
    auto segmentSweep=runtime.SweepEnvironment({{8,2},.25f},{4,0});Check(segmentSweep&&Near(segmentSweep->distance,1.75f),"Scene segment sweep matches ray surface");
    runtime.Stop();TilemapAssetEditor editor;Check(editor.Open(database,*asset),"Open maze for painting");
    editor.SetEnabled(true);editor.Configure(0,std::make_shared<TileBrush>(1));
    editor.HandleEvent({InputEventType::PointerPressed,PointerInput{PointerButton::Left,{}}},GridCoordinate{5,6},true);
    editor.HandleEvent({InputEventType::PointerReleased,PointerInput{PointerButton::Left,{}}},GridCoordinate{5,6});Check(editor.Save(),"Save painted wall");
    ray=runtime.RaycastEnvironment({8,6.5f},{-1,0},20);Check(ray&&ray->objectId==1&&ray->cell==GridCoordinate{5,6}&&Near(ray->distance,2),"Painting/reimport updates canonical query geometry");
    Check(editor.Undo()&&editor.Save(),"Undo painted wall and reimport");ray=runtime.RaycastEnvironment({8,6.5f},{-1,0},20);Check(ray&&ray->cell==GridCoordinate{3,6}&&Near(ray->distance,4),"Undo restores queried boundary");
    SceneProjectRuntime reopened("Reopened fixture");Check(reopened.Load(context,error),"Load second runtime");reopened.SynchronizeScene(scene);
    ray=reopened.RaycastEnvironment({8,6.5f},{-1,0},20);Check(ray&&ray->cell==GridCoordinate{3,6},"Scene reload restores shared maze geometry");
    SceneProjectRuntime movingMap("Moving authored tilemap");Check(movingMap.Load(context,error),"Load moving map fixture");
    movingMap.SynchronizeScene(std::vector<SceneObjectData>{ground,body});
    auto tileActor=movingMap.ResolveSceneObject(3);tileActor.GetComponent<Transform2DComponent>()->position={5,6.5f};
    tileActor.GetComponent<KinematicBody2DComponent>()->velocity={};
    movingMap.ResolveSceneObject(1).Attach<TranslateObstacle>();movingMap.Start();movingMap.FixedUpdate(1);
    Check(Near(tileActor.GetComponent<Transform2DComponent>()->position.x,14.25f),"Translating tilemap pushes stationary body continuously");
    ray=movingMap.RaycastEnvironment({16,6.5f},{-1,0},20);Check(ray&&ray->cell==GridCoordinate{3,6}&&Near(ray->distance,2),"Moving map retains source cell and transformed boundary");
    auto grid= SweepCircleGrid({{0,6.5f},.25f},{10,0},{},1,16,16,[&](auto cell){return map.IsSolid(cell);});
    auto tile=SweepCircleTilemap(map,{},{{0,6.5f},.25f},{10,0});Check(grid&&tile&&Near(grid->contact.fraction*10,tile->distance),"Borrowed simulation grid and authored tile sweep agree");
    std::cout<<"Shared shape/grid collision, moving obstacles, identity, masks, clearance, render chunks, painting/undo/reload passed.\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
