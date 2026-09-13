#pragma once
#include <PipeFrame/Editor/BrushTool.h>
class TestDensityBrush final:public pipeframe::SchemaBrush<TestDensityBrush>{
public:
    double density{.5};
    static auto Schema(){return pipeframe::ComponentSchema<TestDensityBrush>("test.density","Test Density")
        .Editable({.key="density",.displayName="Density",.kind=pipeframe::PropertyKind::Number,.defaultValue=.5,.minimum=0,.maximum=1},&TestDensityBrush::density);}
    std::string_view DataTarget()const override{return "test.density";}
    double PaintData(pipeframe::GridCoordinate,double)const override{return density;}
};
