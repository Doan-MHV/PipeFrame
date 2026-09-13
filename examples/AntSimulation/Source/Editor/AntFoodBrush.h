#pragma once
#include <PipeFrame/Editor/BrushTool.h>
namespace ant_simulation {
class AntFoodBrush final : public pipeframe::SchemaBrush<AntFoodBrush> {
  public:
    std::int64_t quantity{10};
    bool erase{};
    static auto Schema() {
        return pipeframe::ComponentSchema<AntFoodBrush>("ant.food-density", "Ant Food Density")
            .Editable({.key = "quantity",
                       .displayName = "Food per cell",
                       .kind = pipeframe::PropertyKind::Integer,
                       .defaultValue = std::int64_t{10},
                       .minimum = 1,
                       .maximum = 10000},
                      &AntFoodBrush::quantity)
            .Editable({.key = "erase",
                       .displayName = "Erase food",
                       .kind = pipeframe::PropertyKind::Boolean,
                       .defaultValue = false},
                      &AntFoodBrush::erase);
    }
    std::string_view DataTarget() const override { return "ant.food-density"; }
    double DataMaximum() const override { return 10000; }
    double PaintData(pipeframe::GridCoordinate, double) const override { return erase ? 0 : double(quantity); }
};
} // namespace ant_simulation
