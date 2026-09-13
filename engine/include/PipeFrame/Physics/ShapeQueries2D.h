#pragma once
#include <PipeFrame/Physics/Physics2D.h>
#include <array>
namespace pipeframe {
// World-space convex box or zero-thickness, two-sided segment. Box vertices follow
// their perimeter; mirrored transforms are supported. No material implies friction.
struct EnvironmentShape2D {
    std::array<Vector2f,4> points{};
    bool segment{};
};
struct ShapeHit2D { float fraction{}; Vector2f point{},normal{}; };
inline float ShapeDot(Vector2f a,Vector2f b){return a.x*b.x+a.y*b.y;}
inline bool Finite(Vector2f p){return std::isfinite(p.x)&&std::isfinite(p.y);}
inline Vector2f ClosestOnSegment(Vector2f p,Vector2f a,Vector2f b){
    const auto d=b-a;const auto n=LengthSquared(d);return a+d*(n>0?std::clamp(ShapeDot(p-a,d)/n,0.f,1.f):0.f);
}
inline bool InsideShape(Vector2f p,const EnvironmentShape2D &shape){
    if(shape.segment)return false;
    bool positive=false,negative=false;
    for(int i=0;i<4;++i){const auto a=shape.points[i],d=shape.points[(i+1)%4]-a,q=p-a;
        const float cross=d.x*q.y-d.y*q.x;if(cross==0)return false;positive|=cross>0;negative|=cross<0;if(positive&&negative)return false;}
    return positive||negative;
}
inline std::optional<ShapeHit2D> OverlapShape(Circle2D circle,const EnvironmentShape2D &shape){
    if(!Finite(circle.center)||!std::isfinite(circle.radius)||circle.radius<0)return {};
    for(const auto p:shape.points)if(!Finite(p))return {};
    if(InsideShape(circle.center,shape))return ShapeHit2D{0,circle.center,{}};
    float best=std::numeric_limits<float>::infinity();Vector2f closest;
    for(int i=0;i<(shape.segment?1:4);++i){auto p=ClosestOnSegment(circle.center,shape.points[i],shape.points[(i+1)%4]);
        float d=LengthSquared(circle.center-p);if(d<best){best=d;closest=p;}}
    if(best>circle.radius*circle.radius)return {};
    return ShapeHit2D{0,closest,best>0?NormalizeOr(circle.center-closest):Vector2f{}};
}
inline std::optional<ShapeHit2D> SweepShape(Circle2D circle,Vector2f delta,const EnvironmentShape2D &shape){
    if(!Finite(delta)||!Finite(circle.center)||!std::isfinite(circle.radius)||circle.radius<0)return {};
    for(const auto p:shape.points)if(!Finite(p))return {};
    const double travel2=double(delta.x)*delta.x+double(delta.y)*delta.y;
    if(InsideShape(circle.center,shape))return ShapeHit2D{0,circle.center,{}};
    if(auto overlap=OverlapShape(circle,shape);overlap&&LengthSquared(circle.center-overlap->point)<circle.radius*circle.radius)
        return ShapeHit2D{0,overlap->point,{}};
    if(travel2==0)return {};
    std::optional<ShapeHit2D> best;
    const auto accept=[&](double t,Vector2f normal){
        if(t<0||t>1||(best&&t>=best->fraction)||ShapeDot(delta,normal)>=0)return;
        best=ShapeHit2D{float(t),circle.center+delta*float(t)-normal*circle.radius,normal};
    };
    for(int i=0;i<(shape.segment?1:4);++i){
        const auto a=shape.points[i],b=shape.points[(i+1)%4],edge=b-a;
        const float length=Length(edge);
        if(length>0){
            const auto axis=edge/length;const Vector2f perpendicular{-axis.y,axis.x};
            for(float sign:{-1.f,1.f}){
                const auto n=perpendicular*sign;
                if(!shape.segment&&ShapeDot(n,(a+b-shape.points[0]-shape.points[2])*.5f)<=0)continue;
                const double velocity=ShapeDot(delta,n);
                if(velocity>=0)continue;
                const double t=(circle.radius-ShapeDot(circle.center-a,n))/velocity;
                const double along=ShapeDot(circle.center+delta*float(t)-a,axis);
                if(along>=0&&along<=length)accept(t,n);
            }
        }
        if(circle.radius==0)continue;
        for(const auto corner:{a,b}){
            const auto q=circle.center-corner;const double bb=double(q.x)*delta.x+double(q.y)*delta.y;
            const double cc=double(q.x)*q.x+double(q.y)*q.y-double(circle.radius)*circle.radius;
            const double discriminant=bb*bb-travel2*cc;
            if(discriminant<=0)continue; // Pure tangency does not block movement.
            const double t=(-bb-std::sqrt(discriminant))/travel2;
            accept(t,NormalizeOr(circle.center+delta*float(t)-corner));
        }
    }
    return best;
}
inline std::optional<ShapeHit2D> RaycastShape(Vector2f origin,Vector2f direction,float distance,const EnvironmentShape2D &shape){
    for(const auto p:shape.points)if(!Finite(p))return {};
    const float length=Length(direction);
    if(!Finite(origin)||!std::isfinite(length)||length<=0||!std::isfinite(distance)||distance<0)return {};
    direction/=length;
    if(InsideShape(origin,shape))return ShapeHit2D{0,origin,{}};
    if(shape.segment){
        const auto a=shape.points[0],b=shape.points[1],edge=b-a;
        const float cross=direction.x*edge.y-direction.y*edge.x;
        if(cross==0){
            const auto q=a-origin;
            if(q.x*direction.y-q.y*direction.x!=0)return {};
            const float t=std::max(0.f,std::min(ShapeDot(a-origin,direction),ShapeDot(b-origin,direction)));
            if(t>distance||std::max(ShapeDot(a-origin,direction),ShapeDot(b-origin,direction))<0)return {};
            return ShapeHit2D{distance>0?t/distance:0,origin+direction*t,{}};
        }
    }
    if(distance==0){if(auto hit=OverlapShape({origin,0},shape))return hit;return {};}
    return SweepShape({origin,0},direction*distance,shape);
}
inline EnvironmentShape2D AxisAlignedBox(Vector2f low,Vector2f high){
    return {{{low,{high.x,low.y},high,{low.x,high.y}}},false};
}
}
