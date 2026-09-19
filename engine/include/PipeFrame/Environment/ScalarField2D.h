#ifndef PIPEFRAME_ENVIRONMENT_SCALAR_FIELD_2D_H
#define PIPEFRAME_ENVIRONMENT_SCALAR_FIELD_2D_H
#include <PipeFrame/Data/Grid2D.h>
#include <PipeFrame/Foundation/MathTypes.h>

#include <algorithm>
#include <cmath>
namespace pipeframe {
class ScalarField2D {
public:
    void Initialize(Rectanglef newBounds, float requestedCellSize, float initialValue = 0.0f) {
        bounds = newBounds;
        cellSize = std::max(0.001f, requestedCellSize);
        inverseCellSize = 1.0f / cellSize;
        values.Resize(std::max(1, static_cast<int>(std::ceil(bounds.size.x / cellSize))),
                      std::max(1, static_cast<int>(std::ceil(bounds.size.y / cellSize))), initialValue);
        scratch.Resize(values.Columns(), values.Rows(), initialValue);
    }
    void Clear(float value = 0.0f) { values.Fill(value); }
    void Deposit(Vector2f position, float amount, float minimum = 0.0f, float maximum = 1.0f) {
        auto& value = values.At(Cell(position));
        value = std::clamp(value + amount, minimum, maximum);
    }
    [[nodiscard]] float Sample(Vector2f position) const { return values.At(Cell(position)); }
    [[nodiscard]] Vector2f Gradient(Vector2f position) const {
        const auto cell = Cell(position);
        return {Value(cell.column + 1, cell.row) - Value(cell.column - 1, cell.row),
                Value(cell.column, cell.row + 1) - Value(cell.column, cell.row - 1)};
    }
    void DecayAndDiffuse(float deltaTime, float decayRate, float diffusionRate, float minimum = 0.0f,
                         float maximum = 1.0f) {
        if (deltaTime <= 0.0f) return;
        const float decay = std::clamp(1.0f - decayRate * deltaTime, 0.0f, 1.0f),
                    diffusion = std::clamp(diffusionRate * deltaTime, 0.0f, 1.0f);
        for (int row = 0; row < values.Rows(); ++row)
            for (int column = 0; column < values.Columns(); ++column) {
                const float center = Value(column, row);
                const float average = (Value(column - 1, row) + Value(column + 1, row) + Value(column, row - 1) +
                                       Value(column, row + 1)) *
                                      0.25f;
                scratch.At({column, row}) =
                    std::clamp((center + (average - center) * diffusion) * decay, minimum, maximum);
            }
        std::swap(values, scratch);
    }
    [[nodiscard]] const Grid2D<float>& Values() const { return values; }
    [[nodiscard]] GridCoordinate Cell(Vector2f position) const {
        return values.Clamp({static_cast<int>(std::floor((position.x - bounds.position.x) * inverseCellSize)),
                             static_cast<int>(std::floor((position.y - bounds.position.y) * inverseCellSize))});
    }

private:
    [[nodiscard]] float Value(int column, int row) const { return values.At(values.Clamp({column, row})); }
    Rectanglef bounds{};
    float cellSize{1.0f}, inverseCellSize{1.0f};
    Grid2D<float> values, scratch;
};

class VectorField2D {
public:
    void Initialize(Rectanglef bounds, float cellSize, Vector2f initial = {}) {
        x.Initialize(bounds, cellSize, initial.x);
        y.Initialize(bounds, cellSize, initial.y);
    }
    void Deposit(Vector2f position, Vector2f amount, float minimum = -1.0f, float maximum = 1.0f) {
        x.Deposit(position, amount.x, minimum, maximum);
        y.Deposit(position, amount.y, minimum, maximum);
    }
    [[nodiscard]] Vector2f Sample(Vector2f position) const { return {x.Sample(position), y.Sample(position)}; }
    void DecayAndDiffuse(float deltaTime, float decay, float diffusion, float minimum = -1.0f, float maximum = 1.0f) {
        x.DecayAndDiffuse(deltaTime, decay, diffusion, minimum, maximum);
        y.DecayAndDiffuse(deltaTime, decay, diffusion, minimum, maximum);
    }

private:
    ScalarField2D x, y;
};
}  // namespace pipeframe
#endif
