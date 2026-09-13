#ifndef PIPEFRAME_PHYSICS_PHYSICS2D_H
#define PIPEFRAME_PHYSICS_PHYSICS2D_H

#include <PipeFrame/Foundation/MathTypes.h>
#include <PipeFrame/Spatial/UniformSpatialIndex.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace pipeframe {
using PhysicsBodyId=std::uint64_t;
inline constexpr PhysicsBodyId InvalidPhysicsBodyId{};
using CollisionLayer=std::uint32_t;
inline constexpr CollisionLayer AllCollisionLayers=std::numeric_limits<CollisionLayer>::max();

struct Circle2D { Vector2f center{}; float radius{0.5f}; };
struct Segment2D { Vector2f start{}; Vector2f end{}; };
struct DebugLine2D { Vector2f start{}; Vector2f end{}; Color color{255,255,255,255}; };
struct DebugCircle2D { Circle2D circle{}; Color color{255,255,255,255}; };
struct Particle2D { Vector2f position{}; Vector2f velocity{}; float remainingLifetime{1.0f}; };

inline bool IntegrateParticle(Particle2D &particle,float deltaTime) {
    if(deltaTime<=0.0f) return particle.remainingLifetime>0.0f;
    particle.position+=particle.velocity*deltaTime;
    particle.remainingLifetime=std::max(0.0f,particle.remainingLifetime-deltaTime);
    return particle.remainingLifetime>0.0f;
}

struct PhysicsBody2D {
    PhysicsBodyId id{InvalidPhysicsBodyId};
    Vector2f position{};
    Vector2f previousPosition{};
    Vector2f velocity{};
    Vector2f lastMove{};
    float radius{0.5f};
    float mass{1.0f};
    float response{1.0f};
    CollisionLayer layer{1};
    CollisionLayer mask{AllCollisionLayers};
    bool moving{true};
};

struct Contact2D {
    PhysicsBodyId first{};
    PhysicsBodyId second{};
    Vector2f normal{};
    float penetration{};
};

[[nodiscard]] inline Vector2f NormalizeOr(Vector2f value, Vector2f fallback={1.0f,0.0f}) {
    const float squared=LengthSquared(value);
    return squared>0.0f ? value/std::sqrt(squared) : fallback;
}

template <typename Body>
void IntegrateBody(Body &body,float deltaTime,float maximumMove=std::numeric_limits<float>::infinity()) {
    if (deltaTime<=0.0f) return;
    if (!body.moving) { body.velocity={}; body.lastMove={}; body.previousPosition=body.position; return; }
    body.position+=body.velocity*deltaTime;
    Vector2f movement=body.position-body.previousPosition;
    const float squared=LengthSquared(movement);
    if (squared>maximumMove*maximumMove) movement=NormalizeOr(movement)*maximumMove;
    body.position=body.previousPosition+movement;
    body.lastMove=movement;
    body.velocity=movement/deltaTime;
    body.previousPosition=body.position;
}

template <typename Body>
bool ConstrainToBounds(Body &body,Rectanglef bounds) {
    const Vector2f before=body.position;
    body.position.x=std::clamp(body.position.x,bounds.position.x+body.radius,
        bounds.position.x+bounds.size.x-body.radius);
    body.position.y=std::clamp(body.position.y,bounds.position.y+body.radius,
        bounds.position.y+bounds.size.y-body.radius);
    if (body.position==before) return false;
    body.previousPosition=body.position; body.lastMove={}; body.velocity={}; return true;
}

template <typename First,typename Second>
std::optional<Contact2D> SolveCircleContact(First &first,Second &second) {
    if ((first.mask&second.layer)==0 || (second.mask&first.layer)==0) return std::nullopt;
    const Vector2f difference=first.position-second.position;
    const float distanceSquared=LengthSquared(difference);
    const float contactDistance=first.radius+second.radius;
    if (distanceSquared>=contactDistance*contactDistance) return std::nullopt;
    const float distance=distanceSquared>0.0001f?std::sqrt(distanceSquared):0.0f;
    const Vector2f normal=distance>0.0f?difference/distance:
        (first.id<second.id?Vector2f{-1.0f,0.0f}:Vector2f{1.0f,0.0f});
    const float penetration=contactDistance-distance;
    const float correctionLength=std::max(first.response,second.response)*penetration;
    const float firstMass=std::max(0.0001f,first.mass),secondMass=std::max(0.0001f,second.mass);
    const Vector2f correction=normal*correctionLength;
    first.position+=correction*(secondMass/(firstMass+secondMass));
    second.position-=correction*(firstMass/(firstMass+secondMass));
    first.previousPosition=first.position; second.previousPosition=second.position;
    return Contact2D{first.id,second.id,normal,penetration};
}

// One workspace per independently executing solver. Retains allocation capacity only;
// positions and cell membership are rebuilt on every solve. No body pointers survive.
class CircleContactWorkspace {
public:
    void Prepare(Rectanglef bounds, float cellSize) {
        if (!initialized || previousBounds.position != bounds.position ||
            previousBounds.size != bounds.size || previousCellSize != cellSize) {
            broadphase.Initialize(bounds, cellSize);
            previousBounds = bounds;
            previousCellSize = cellSize;
            initialized = true;
        }
        entries.clear();
    }
    UniformSpatialIndex<std::size_t> broadphase;
    std::vector<UniformSpatialIndex<std::size_t>::Entry> entries;
private:
    Rectanglef previousBounds{};
    float previousCellSize{};
    bool initialized{};
};

template <typename Body,typename OnContact>
std::size_t SolveCircleContacts(std::span<Body> bodies,Rectanglef bounds,CircleContactWorkspace &workspace,OnContact onContact) {
    if (bodies.empty()) return 0;
    float maximumRadius=0.001f;
    for(const auto &body:bodies) maximumRadius=std::max(maximumRadius,body.radius);
    workspace.Prepare(bounds,maximumRadius*2.0f);
    auto &broadphase=workspace.broadphase;
    auto &entries=workspace.entries;
    entries.reserve(bodies.size());
    for(std::size_t index=0;index<bodies.size();++index) entries.push_back({index,bodies[index].position});
    broadphase.Rebuild(entries);
    std::size_t count{};
    for(std::size_t first=0;first<bodies.size();++first) {
        const auto range=broadphase.CellsOverlapping(bodies[first].position,bodies[first].radius+maximumRadius);
        for(int row=range.minimum.row;row<=range.maximum.row;++row) {
            for(int column=range.minimum.column;column<=range.maximum.column;++column) {
                for(const std::size_t second:broadphase.GetIds({column,row})) {
                    if(second<=first) continue;
                    if(auto contact=SolveCircleContact(bodies[first],bodies[second])) { onContact(*contact); ++count; }
                }
            }
        }
    }
    return count;
}

// Compatibility convenience overload for one-off queries.
template <typename Body,typename OnContact>
std::size_t SolveCircleContacts(std::span<Body> bodies,Rectanglef bounds,OnContact onContact) {
    CircleContactWorkspace workspace;
    return SolveCircleContacts(bodies,bounds,workspace,onContact);
}

[[nodiscard]] inline Vector2f ClosestPoint(Segment2D segment,Vector2f point) {
    const Vector2f edge=segment.end-segment.start;
    const float lengthSquared=LengthSquared(edge);
    if(lengthSquared<=0.0f) return segment.start;
    const float projection=std::clamp(((point.x-segment.start.x)*edge.x+(point.y-segment.start.y)*edge.y)/lengthSquared,0.0f,1.0f);
    return segment.start+edge*projection;
}
[[nodiscard]] inline bool Intersects(Circle2D circle,Segment2D segment) {
    return LengthSquared(circle.center-ClosestPoint(segment,circle.center))<=circle.radius*circle.radius;
}

template <typename Body,typename ColorFor,typename Visitor>
void VisitBodyDebugGeometry(std::span<const Body> bodies,ColorFor colorFor,Visitor visitor) {
    for(const auto &body:bodies) visitor(DebugCircle2D{{body.position,body.radius},colorFor(body)});
}
} // namespace pipeframe
#endif
