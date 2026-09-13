#ifndef PIPEFRAME_PHYSICS_PHYSICS_WORLD_2D_H
#define PIPEFRAME_PHYSICS_PHYSICS_WORLD_2D_H
#include <PipeFrame/Physics/BodyStorage.h>
#include <algorithm>
#include <span>
#include <unordered_map>
#include <vector>
namespace pipeframe {
class PhysicsWorld2D {
public:
    explicit PhysicsWorld2D(Rectanglef bounds):bounds(bounds){}
    PhysicsBodyId Create(PhysicsBody2D body={}) { return storage.Create(body); }
    bool Remove(PhysicsBodyId id) { return storage.Remove(id); }
    PhysicsBody2D *Find(PhysicsBodyId id) { return storage.Find(id); }
    const PhysicsBody2D *Find(PhysicsBodyId id) const { return storage.Find(id); }
    std::size_t Step(float deltaTime,float maximumMove=std::numeric_limits<float>::infinity()) { for(auto &body:storage.Bodies()){IntegrateBody(body,deltaTime,maximumMove);ConstrainToBounds(body,bounds);} return SolveCircleContacts<PhysicsBody2D>(storage.Bodies(),bounds,contacts,[](const Contact2D&){}); }
    std::span<PhysicsBody2D> Bodies(){return storage.Bodies();} std::span<const PhysicsBody2D> Bodies()const{return storage.Bodies();}
    void Clear(){storage.Clear();}
private: Rectanglef bounds; BodyStorage<> storage; CircleContactWorkspace contacts;
};
} // namespace pipeframe
#endif
