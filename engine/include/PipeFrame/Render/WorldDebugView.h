#pragma once
#include <PipeFrame/Render/Canvas.h>
#include <vector>
namespace pipeframe {
struct WorldDebugOptions { bool physics{}; bool mesh{}; };
// Project worlds supply their real geometry. No backend or collider type is required.
class WorldDebugDraw {
public:
    void Line(Vector2f a, Vector2f b, Color color = {70,230,140,255}) {
        vertices.push_back({a,color,{}}); vertices.push_back({b,color,{}});
    }
    void Circle(Vector2f center, float radius, Color color = {70,230,140,255}) {
        if(!std::isfinite(radius)||radius<=0)return;
        for(int i=0;i<32;++i){const float a=i*6.2831853f/32,b=(i+1)*6.2831853f/32;
            Line(center+Vector2f{std::cos(a),std::sin(a)}*radius,center+Vector2f{std::cos(b),std::sin(b)}*radius,color);}
    }
    void Polygon(std::span<const Vector2f> points, Color color = {70,230,140,255}) {
        if(points.size()<2)return;
        for(std::size_t i=0;i<points.size();++i)Line(points[i],points[(i+1)%points.size()],color);
    }
    void Mesh(std::span<const Vertex2D> triangles, Color color = {255,210,60,255}) {
        for(std::size_t i=0;i+2<triangles.size();i+=3){
            if(!triangles[i].color.a&&!triangles[i+1].color.a&&!triangles[i+2].color.a)continue;
            Line(triangles[i].position,triangles[i+1].position,color);
            Line(triangles[i+1].position,triangles[i+2].position,color);
            Line(triangles[i+2].position,triangles[i].position,color);
        }
    }
    void Draw(Canvas canvas) const {canvas.Draw(vertices.data(),vertices.size(),PrimitiveTopology::Lines);}
    std::span<const Vertex2D> Vertices() const {return vertices;}
private:
    std::vector<Vertex2D> vertices;
};
}
