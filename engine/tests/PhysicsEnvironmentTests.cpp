#include <PipeFrame/Environment/ScalarField2D.h>
#include <PipeFrame/Physics/PhysicsWorld2D.h>
#include <PipeFrame/Spatial/GridRaycast.h>

#include <cassert>
#include <cmath>
#include <stdexcept>

namespace {
pipeframe::PhysicsBody2D Body(pipeframe::Vector2f position,pipeframe::Vector2f velocity={}) {
    pipeframe::PhysicsBody2D body;body.position=position;body.previousPosition=position;body.velocity=velocity;body.radius=0.5f;body.response=1.0f;return body;
}
void TestWorldLifecycleAndContacts() {
    pipeframe::PhysicsWorld2D world({{0,0},{20,20}});
    const auto first=world.Create(Body({5,5},{1,0}));
    const auto second=world.Create(Body({5.6f,5}));
    assert(world.Step(0.1f)>=1);
    assert(world.Find(first)&&world.Find(second));
    assert(pipeframe::Length(world.Find(first)->position-world.Find(second)->position)>=0.999f);
    assert(world.Remove(first)&&!world.Find(first)&&!world.Remove(first));
    assert(world.Bodies().size()==1);
    std::size_t debugBodies{};
    pipeframe::VisitBodyDebugGeometry<pipeframe::PhysicsBody2D>(world.Bodies(),[](const auto&){return pipeframe::Color{1,2,3,4};},[&](const auto &circle){assert(circle.color==pipeframe::Color({1,2,3,4}));++debugBodies;});
    assert(debugBodies==1);
}
void TestDeterminismAndLayers() {
    pipeframe::PhysicsWorld2D a({{0,0},{10,10}}),b({{0,0},{10,10}});
    auto body=Body({2,3},{0.25f,-0.1f});
    const auto aId=a.Create(body),bId=b.Create(body);
    for(int i=0;i<100;++i){a.Step(0.01f);b.Step(0.01f);}
    assert(a.Find(aId)->position==b.Find(bId)->position);
    pipeframe::PhysicsBody2D first=Body({5,5}),second=Body({5.2f,5});
    first.id=1;second.id=2;first.layer=1;first.mask=1;second.layer=2;second.mask=2;
    assert(!pipeframe::SolveCircleContact(first,second));
}
void TestContactWorkspaceReuse() {
    pipeframe::CircleContactWorkspace reused;
    // Change body count/order, cell size and bounds, including an empty tick. A reused
    // solver must produce exactly the same ordered contacts and poses as a fresh one.
    for (int tick=0; tick<80; ++tick) {
        std::vector<pipeframe::PhysicsBody2D> fresh;
        const int count=tick%9==0?0:5+tick%23;
        for(int i=0;i<count;++i) {
            auto body=Body({2.f+float(i%5)*0.4f,2.f+float(i/5)*0.4f});
            body.id=static_cast<pipeframe::PhysicsBodyId>(count-i);
            body.radius=tick%4==0?0.7f:0.5f;
            fresh.push_back(body);
        }
        auto cached=fresh;
        const pipeframe::Rectanglef bounds=tick%3==0?pipeframe::Rectanglef{{1,1},{8,8}}:pipeframe::Rectanglef{{0,0},{20,20}};
        std::vector<pipeframe::Contact2D> a,b;
        pipeframe::SolveCircleContacts<pipeframe::PhysicsBody2D>(fresh,bounds,[&](auto c){a.push_back(c);});
        pipeframe::SolveCircleContacts<pipeframe::PhysicsBody2D>(cached,bounds,reused,[&](auto c){b.push_back(c);});
        if(a.size()!=b.size())throw std::runtime_error("Workspace changed contact count");
        for(std::size_t i=0;i<a.size();++i)
            if(a[i].first!=b[i].first || a[i].second!=b[i].second || a[i].normal!=b[i].normal || a[i].penetration!=b[i].penetration)
                throw std::runtime_error("Workspace changed ordered contacts");
        for(std::size_t i=0;i<fresh.size();++i)
            if(fresh[i].position!=cached[i].position || fresh[i].previousPosition!=cached[i].previousPosition)
                throw std::runtime_error("Workspace changed solved positions");
    }
}
void TestEnvironmentPrimitives() {
    pipeframe::Particle2D particle{{0,0},{2,0},0.25f};assert(pipeframe::IntegrateParticle(particle,0.1f));assert(particle.position.x==0.2f);assert(!pipeframe::IntegrateParticle(particle,0.2f));
    pipeframe::ScalarField2D field;field.Initialize({{0,0},{5,5}},1.0f);
    field.Deposit({2.5f,2.5f},1.0f);assert(field.Sample({2.5f,2.5f})==1.0f);
    field.DecayAndDiffuse(0.5f,0.2f,0.5f);
    assert(field.Sample({2.5f,2.5f})<1.0f&&field.Sample({2.5f,2.5f})>0.0f);
    assert(pipeframe::LengthSquared(field.Gradient({1.5f,2.5f}))>0.0f);
    pipeframe::VectorField2D vectors;vectors.Initialize({{0,0},{2,2}},1.0f);vectors.Deposit({0.5f,0.5f},{0.5f,-0.25f});assert(vectors.Sample({0.5f,0.5f})==pipeframe::Vector2f({0.5f,-0.25f}));
    const auto hit=pipeframe::RaycastGrid({0.5f,0.5f},{1,1},10,{0,0},1,4,4,[](pipeframe::GridCoordinate cell){return cell==pipeframe::GridCoordinate{2,2};});
    assert(hit&&hit->cell==pipeframe::GridCoordinate({2,2}));
    assert(pipeframe::Intersects({{1,1},0.5f},{{0,1},{2,1}}));
}
}
int main(){TestContactWorkspaceReuse();TestWorldLifecycleAndContacts();TestDeterminismAndLayers();TestEnvironmentPrimitives();}
