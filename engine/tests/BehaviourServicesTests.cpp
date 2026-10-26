#include <PipeFrame/Project/SceneProjectRuntime.h>
#include <iostream>
#include <stdexcept>
using namespace pipeframe;
void Check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct TimedBehaviour:Behaviour {
    explicit TimedBehaviour(int &ticks):ticks(ticks){}
    int &ticks;
    Coroutine Sequence(){++ticks;co_yield WaitForSeconds{.2f};++ticks;co_yield WaitForSeconds{};++ticks;}
    void Start() override {StartCoroutine(Sequence());}
};
struct SelfRemovingTask:Behaviour {
    explicit SelfRemovingTask(int &calls):calls(calls){} int &calls;
    Coroutine Remove(){++calls;GetObject().Destroy();co_return;}
    Coroutine MustNotRun(){calls+=100;co_return;}
    void Start() override{StartCoroutine(Remove());StartCoroutine(MustNotRun());}
};
struct EventComponent {
    int collisionIn{},collisionOut{},triggerIn{},triggerOut{},visible{},hidden{};
    bool destroyOnTrigger{};
    static auto Schema(){return ComponentSchema<EventComponent>("test.events","Events");}
};
struct EventBehaviour:Behaviour {
    void OnCollisionEnter(const CollisionEvent2D &) override{++GetComponent<EventComponent>()->collisionIn;}
    void OnCollisionExit(const CollisionEvent2D &) override{++GetComponent<EventComponent>()->collisionOut;}
    void OnTriggerEnter(const CollisionEvent2D &) override{
        auto &events=*GetComponent<EventComponent>();++events.triggerIn;if(events.destroyOnTrigger)GetObject().Destroy();
    }
    void OnTriggerExit(const CollisionEvent2D &) override{++GetComponent<EventComponent>()->triggerOut;}
    void OnBecameVisible() override{++GetComponent<EventComponent>()->visible;}
    void OnBecameInvisible() override{++GetComponent<EventComponent>()->hidden;}
};
struct ProbeEntity:EntityArchetype {
    void Build(ecs::World &world,ecs::Entity entity) const override {
        world.Add<Transform2DComponent>(entity);world.Add<KinematicBody2DComponent>(entity);
        world.Add<EnvironmentCollider2DComponent>(entity).enabled=false;world.Add<EventComponent>(entity);
    }
    void OnInstantiated(const SceneObject &object) const override {object.Attach<EventBehaviour>();}
};
struct Surface:RenderSurface {
    Canvas GetCanvas() override{return {nullptr,[](void*,std::span<const Vertex2D>,PrimitiveTopology,const RenderState&){}};}
    Vector2u GetSize() const override{return {100,100};} void SetScreenSize(Vector2u)override{}
    void BeginWorld(const Camera2D &)override{} void BeginScreen()override{}
    Rectanglei Viewport(const Camera2D &)const override{return {{},{100,100}};}
    Vector2f PixelToWorld(Vector2i p,const Camera2D &)const override{return {float(p.x),float(p.y)};}
    Vector2i WorldToPixel(Vector2f p,const Camera2D &)const override{return {int(p.x),int(p.y)};}
};
int main(){try{
    int ticks=0;BehaviourScene scene;auto object=scene.CreateObject();auto &timed=object.Attach<TimedBehaviour>(ticks);
    scene.FixedUpdate(.1f);Check(ticks==1,"Coroutine starts on fixed tick");
    scene.SetEnabled(timed,false);scene.FixedUpdate(1);Check(ticks==1,"Disabled task clock pauses");
    scene.SetEnabled(timed,true);scene.FixedUpdate(.1f);Check(ticks==1,"Wait retains remaining time");
    scene.FixedUpdate(.1f);Check(ticks==2,"Elapsed simulation time resumes coroutine");
    timed.StopAllCoroutines();scene.FixedUpdate(1);Check(ticks==2,"Cancellation prevents continuation");
    timed.StartCoroutine(timed.Sequence());scene.FixedUpdate(.1f);object.Destroy();scene.FixedUpdate(1);Check(ticks==3,"Destroy cancels suspended continuation");
    int calls=0;auto doomed=scene.CreateObject();doomed.Attach<SelfRemovingTask>(calls);scene.FixedUpdate(.1f);
    Check(calls==1&&!doomed.IsValid(),"Coroutine destruction is deferred and cancels later tasks safely");
    SceneProjectRuntime runtime("Service callbacks");runtime.RegisterComponent<EventComponent>();
    runtime.RegisterEntity<ProbeEntity>({"test.probe","Probe",{}, {}});
    auto probe=runtime.CreateDefaultObject("test.probe");probe.id=1;
    auto wall=runtime.CreateDefaultObject(EnvironmentObstacleEntityTypeId);wall.id=2;
    runtime.SynchronizeScene(std::vector<SceneObjectData>{probe,wall});
    auto moving=runtime.ResolveSceneObject(1),obstacle=runtime.ResolveSceneObject(2);
    obstacle.GetComponent<Transform2DComponent>()->position={5,0};
    moving.GetComponent<KinematicBody2DComponent>()->velocity={10,0};runtime.Start();runtime.FixedUpdate(1);
    auto *events=moving.GetComponent<EventComponent>();Check(events->collisionIn==1,"Actual kinematic wall contact emits enter");
    runtime.FixedUpdate(.1f);Check(events->collisionIn==1,"Persistent contact does not repeat enter");
    moving.GetComponent<KinematicBody2DComponent>()->velocity={-10,0};runtime.FixedUpdate(.1f);
    Check(events->collisionOut==1,"Leaving contact emits exit");
    obstacle.GetComponent<EnvironmentCollider2DComponent>()->trigger=true;
    moving.GetComponent<KinematicBody2DComponent>()->velocity={10,0};runtime.FixedUpdate(1);
    Check(events->triggerIn==1 && moving.GetComponent<Transform2DComponent>()->position.x>10,"Trigger crossing emits enter without blocking");
    runtime.FixedUpdate(.1f);Check(events->triggerOut==1,"Leaving trigger emits exit");
    RenderContext context(std::make_shared<Surface>());context.GetCamera().SetSize({40,20});context.SetCameraCenter({10,0});
    runtime.Render(context);runtime.Render(context);Check(events->visible==1,"Rendered shape enters view exactly once");
    context.SetCameraCenter({200,0});runtime.Render(context);Check(events->hidden==1,"Camera departure emits invisible");
    events->destroyOnTrigger=true;moving.GetComponent<KinematicBody2DComponent>()->velocity={-10,0};runtime.FixedUpdate(1);
    Check(!moving.IsValid(),"Trigger callback may safely destroy its entity");runtime.Render(context);
    runtime.Reset();Check(runtime.ResolveSceneObject(1).IsValid(),"Reset restores authored entity after callback destruction");
    runtime.Unload();
    SceneProjectRuntime paired("Paired contacts");paired.RegisterComponent<EventComponent>();
    paired.RegisterEntity<ProbeEntity>({"test.probe","Probe",{}, {}});
    auto a=paired.CreateDefaultObject("test.probe");a.id=10;auto b=a;b.id=11;
    paired.SynchronizeScene(std::vector<SceneObjectData>{a,b});
    auto first=paired.ResolveSceneObject(10),second=paired.ResolveSceneObject(11);
    for(auto item:{first,second}){auto *shape=item.GetComponent<EnvironmentCollider2DComponent>();shape->enabled=true;shape->trigger=true;}
    paired.Start();paired.Render(context); // Rendering before Start must not consume visibility transitions.
    context.SetCameraCenter({0,0});paired.FixedUpdate(.1f);paired.Render(context);
    Check(first.GetComponent<EventComponent>()->triggerIn==1 && second.GetComponent<EventComponent>()->triggerIn==1,"Two bodies receive one reciprocal trigger enter each");
    Check(first.GetComponent<EventComponent>()->visible==1,"First render after lifecycle start emits visibility");
    first.Destroy();paired.FixedUpdate(.1f);
    Check(second.GetComponent<EventComponent>()->triggerOut==1,"Surviving second receiver gets exit when first is destroyed");
    paired.Unload();std::cout<<"Coroutine timing/cancellation, real collision/trigger transitions, render visibility and callback destruction passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
