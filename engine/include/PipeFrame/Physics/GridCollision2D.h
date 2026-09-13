#pragma once
#include <PipeFrame/Physics/ShapeQueries2D.h>
#include <PipeFrame/Data/Grid2D.h>
namespace pipeframe {
struct GridShapeHit2D { GridCoordinate cell{}; ShapeHit2D contact; };
// Borrows the existing simulation grid. No mirrored wall array or per-agent allocation.
template<class Blocked> std::optional<GridShapeHit2D> SweepCircleGrid(Circle2D circle,Vector2f delta,
    Vector2f origin,float size,int columns,int rows,Blocked blocked){
    if(!Finite(circle.center)||!Finite(delta)||!std::isfinite(circle.radius)||circle.radius<0||
        !Finite(origin)||!std::isfinite(size)||size<=0||columns<=0||rows<=0)return {};
    const auto to=circle.center+delta;if(!Finite(to))return {};
    const auto index=[&](double value,double base,int count){return int(std::clamp(std::floor((value-base)/size),0.0,double(count-1)));};
    const int left=index(std::min(circle.center.x,to.x)-circle.radius,origin.x,columns);
    const int right=index(std::max(circle.center.x,to.x)+circle.radius,origin.x,columns);
    const int top=index(std::min(circle.center.y,to.y)-circle.radius,origin.y,rows);
    const int bottom=index(std::max(circle.center.y,to.y)+circle.radius,origin.y,rows);
    std::optional<GridShapeHit2D> result;
    for(int y=top;y<=bottom;++y)for(int x=left;x<=right;++x){
        if(!blocked(GridCoordinate{x,y}))continue;
        const auto p=origin+Vector2f{float(x),float(y)}*size;
        auto hit=SweepShape(circle,delta,AxisAlignedBox(p,p+Vector2f{size,size}));
        if(hit&&(!result||hit->fraction<result->contact.fraction))result=GridShapeHit2D{{x,y},*hit};
    }
    return result;
}
struct CircleMotion2D {Vector2f position{},velocity{};bool collided{};};
// Query returns a normalized time of impact;
// zero-normal hits mean initial penetration and deliberately stop rather than teleport.
template<class Query> CircleMotion2D MoveCircle(Circle2D circle,Vector2f displacement,Vector2f velocity,Query query){
    CircleMotion2D result{circle.center,velocity,false};
    for(int i=0;i<4&&LengthSquared(displacement)>1e-12f;++i){
        const auto hit=query(Circle2D{result.position,circle.radius},displacement);
        if(!hit){result.position+=displacement;break;}
        result.collided=true;const float fraction=std::clamp(hit->fraction,0.f,1.f);
        result.position+=displacement*fraction;displacement*=1-fraction;
        if(LengthSquared(hit->normal)==0){result.velocity={};break;}
        const float into=ShapeDot(displacement,hit->normal);if(into<0)displacement-=hit->normal*into;
        const float speed=ShapeDot(result.velocity,hit->normal);if(speed<0)result.velocity-=hit->normal*speed;
    }
    return result;
}
}
