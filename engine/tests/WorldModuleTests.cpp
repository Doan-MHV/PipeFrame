#include <type_traits>
#include <PipeFrame/World/World.h>
#include <PipeFrame/Render/WorldDebugView.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using namespace pipeframe;
namespace {
struct Script:Behaviour { explicit Script(std::vector<int> &log):log(log){} void Start() override{log.push_back(1);} void FixedUpdate(float) override{log.push_back(2);} std::vector<int> &log; };
struct System:FixedUpdateSystem<void> {explicit System(std::vector<int> &log):log(log){} std::string_view GetSystemId() const override{return "test";} void Update(float) override{log.push_back(3);} std::vector<int> &log;};
struct Physics:PhysicsWorld { explicit Physics(std::vector<int> &log){AddStep([&](float){log.push_back(4);});AddStep([&](float){log.push_back(5);});} };
struct Layer:RenderLayer {Layer(std::vector<int> &log,int id):log(log),id(id){} void Draw(Canvas,RenderState) const override{log.push_back(id);}std::vector<int> &log;int id;};
struct Rendering:RenderingWorld { Rendering(Layer &a,Layer &b,Layer &c){AddLayer(a);AddLayer(b,1);AddLayer(c,1);} };
struct Presentation : RenderSystem {
 Presentation(std::vector<int>& log, int id) : log(log), id(id) {}
 void Draw(Canvas, BehaviourScene&) override { log.push_back(id); }
 std::vector<int>& log; int id;
};
struct Presentations : RenderingWorld {
 Presentations(Presentation& a, Presentation& b, Presentation& c) { AddSystem(a); AddSystem(b); AddSystem(c); }
};
struct Root:World { using World::RunPhase;using World::ResetSchedule;Root():World(2){} };
}
static_assert(std::is_same_v<pipeframe::RuntimeWorld, pipeframe::SimulationWorld>);
int main(){
 std::vector<int> sharedLog;
 BehaviourScene sharedScene;
 { SimulationWorld shared(sharedScene);
   auto entity=sharedScene.CreateObject();sharedScene.Attach<Script>(entity.GetEntity(),sharedLog);
   shared.Update(.01f);
   if(&shared.GetScene()!=&sharedScene || sharedLog!=std::vector<int>{1,2})return 1;
 }
 sharedScene.FixedUpdate(.01f);
 if(sharedLog!=std::vector<int>{1,2,2})return 1;
 std::vector<int> log;SimulationWorld runtime;System system(log);runtime.AddSystem(system);
 auto object=runtime.GetScene().CreateObject();runtime.GetScene().Attach<Script>(object.GetEntity(),log);
 runtime.Update(0);assert(log.empty());runtime.Update(.01f);assert((log==std::vector<int>{1,2,3}));
 log.clear();runtime.Update(.01f);assert((log==std::vector<int>{2,3}));
 log.clear();Physics physics(log);physics.Update(.01f);assert((log==std::vector<int>{4,5}));
 bool invalid=false;try{physics.Update(std::numeric_limits<float>::quiet_NaN());}catch(const std::invalid_argument &){invalid=true;}assert(invalid);
 log.clear();Layer a(log,6),b(log,7),c(log,8);auto rendering=std::make_shared<Rendering>(a,b,c);
 Canvas canvas(nullptr,[](void*,std::span<const Vertex2D>,PrimitiveTopology,const RenderState&){});
 Root root;root.SetRenderingWorld(rendering);rendering.reset();root.GetRenderingWorld()->Render(canvas);assert((log==std::vector<int>{6}));
 b.SetLayerEnabled(false);root.GetRenderingWorld()->Render(canvas,1);assert((log==std::vector<int>{6,8}));
 bool skipped=false;try{root.RunPhase(1,.01f,[]{});}catch(const std::logic_error &){skipped=true;}assert(skipped);
 root.RunPhase(0,.01f,[]{});root.RunPhase(1,.01f,[]{});root.ResetSchedule();root.RunPhase(0,.01f,[]{});
 std::size_t count=0;Canvas capture(&count,[](void *count,std::span<const Vertex2D> vertices,PrimitiveTopology topology,const RenderState&){
   assert(topology==PrimitiveTopology::Triangles);*static_cast<std::size_t*>(count)=vertices.size();
   for(const auto &vertex:vertices){const float radius=std::hypot(vertex.position.x,vertex.position.y);assert(std::abs(radius-2)<.0001f||std::abs(radius-2.2f)<.0001f);}
 });capture.DrawRing({0,0},2,.2f,Color::White);assert(count==384);
 WorldDebugDraw debug;
 const std::array<Vertex2D,3> triangle{{{{1,2},Color::White,{}},{{3,2},Color::White,{}},{{1,4},Color::White,{}}}};
 debug.Mesh(triangle);debug.Circle({10,20},2);
 if(debug.Vertices().size()!=70)return 1;
 bool submitted=false;
 Canvas debugCanvas(&submitted,[](void *value,std::span<const Vertex2D> vertices,PrimitiveTopology topology,const RenderState &state){
   if(topology!=PrimitiveTopology::Lines||vertices.size()!=70||state.texture.resources||state.shader.resources)throw std::runtime_error("Debug geometry must be one untextured line batch");
   *static_cast<bool*>(value)=true;
 });debug.Draw(debugCanvas);if(!submitted)return 1;
 // Visibility must work independently of simulation stepping, including paused frames.
 log.clear(); Presentation always(log,1), physicsView(log,2), meshView(log,3);
 physicsView.SetVisibility(RenderVisibility::Physics);
 meshView.SetVisibility(RenderVisibility::Mesh);
 Presentations views(always,physicsView,meshView);
 views.RenderSystems(canvas,sharedScene);
 if(log!=std::vector<int>{1})return 1;
 log.clear(); views.RenderSystems(canvas,sharedScene,{true,false});
 if(log!=std::vector<int>{1,2})return 1;
 log.clear(); views.RenderSystems(canvas,sharedScene,{false,true});
 if(log!=std::vector<int>{1,3})return 1;
 log.clear(); views.RenderSystems(canvas,sharedScene,{true,true});
 if(log!=std::vector<int>{1,2,3})return 1;
 physicsView.SetEnabled(false);
 log.clear(); views.RenderSystems(canvas,sharedScene,{true,true});
 if(log!=std::vector<int>{1,3})return 1;
 physicsView.SetEnabled(true); physicsView.SetVisibility(RenderVisibility::Always);
 log.clear(); views.RenderSystems(canvas,sharedScene);
 if(log!=std::vector<int>{1,2})return 1;
 std::cout<<"World lifecycle, ordered physics, render ownership/layers, and neutral canvas passed\n";
}
