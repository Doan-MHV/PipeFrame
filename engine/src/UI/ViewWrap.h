#pragma once
#include <PipeFrame/Backend/SFML/UI/Panel.h>
#include <cmath>
#include <limits>

namespace pipeframe::ui {
// Equal-width responsive action rows. Width constraints determine wrapping;
// captions never force overlapping neighboring hit regions.
class ViewWrap final : public Panel {
public:
    float minimumWidth{100}, gap{4};
    void Refresh() {
        if (arranging) return;
        arranging=true;
        const float width=std::max(0.0f,GetSize().x);
        const auto columns=Columns(width);
        const float cell=std::max(0.0f,(width-gap*(columns-1))/columns);
        float y=0;
        for(std::size_t begin=0;begin<GetChildCount();begin+=columns) {
            float rowHeight=0;
            for(std::size_t i=begin;i<std::min(begin+columns,GetChildCount());++i)
                rowHeight=std::max(rowHeight,GetChild(i)->Measure({{0,0},{cell,std::numeric_limits<float>::infinity()}}).y);
            for(std::size_t i=begin;i<std::min(begin+columns,GetChildCount());++i)
                GetChild(i)->Arrange({{float(i-begin)*(cell+gap),y},{cell,rowHeight}});
            y+=rowHeight+gap;
        }
        if(GetHeightPolicy()==SizePolicy::FitContent) SetSize({width,std::max(0.0f,y-(GetChildCount()?gap:0))});
        arranging=false;
    }
protected:
    sf::Vector2f OnMeasure(const BoxConstraints &constraints) override {
        const float width=std::isfinite(constraints.maximum.x)?constraints.maximum.x:minimumWidth*GetChildCount();
        const auto columns=Columns(width);
        const float cell=std::max(0.0f,(width-gap*(columns-1))/columns);
        float height=0;
        for(std::size_t begin=0;begin<GetChildCount();begin+=columns) {
            float rowHeight=0;
            for(std::size_t i=begin;i<std::min(begin+columns,GetChildCount());++i)
                rowHeight=std::max(rowHeight,GetChild(i)->Measure({{0,0},{cell,std::numeric_limits<float>::infinity()}}).y);
            height+=rowHeight+(begin?gap:0);
        }
        return {width,height};
    }
    void OnGeometryChanged() override { Panel::OnGeometryChanged(); Refresh(); }
    void OnChildGeometryChanged(Widget &) override { Refresh(); }
private:
    std::size_t Columns(float width) const {
        return std::max(std::size_t{1},std::min(GetChildCount(),static_cast<std::size_t>(std::max(1.0f,std::floor((width+gap)/(minimumWidth+gap))))));
    }
    bool arranging{};
};
}
