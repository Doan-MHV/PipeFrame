#include "World/Runtime/Environment/AntEnvironment.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>

#include "World/Runtime/Environment/WallBuilder.h"

namespace ant_simulation {

namespace {

template<typename Callback>
std::size_t ForEachBrushCell(
    AntEnvironment &environment,
    const pipeframe::Vector2f center,
    const float radius,
    Callback &&callback
) {
    if (!std::isfinite(center.x) ||
        !std::isfinite(center.y) ||
        !std::isfinite(radius) ||
        radius <= 0.0f) {
        return 0;
    }

    const pipeframe::Vector2i centerCell =
        AntEnvironment::WorldToCell(center);

    const int integerRadius =
        static_cast<int>(radius);

    const float radiusSquared =
        radius * radius;

    std::size_t affectedCount{0};

    for (int y = centerCell.y - integerRadius;
         y <= centerCell.y + integerRadius;
         ++y) {
        for (int x = centerCell.x - integerRadius;
             x <= centerCell.x + integerRadius;
             ++x) {
            const pipeframe::Vector2f cellCenter =
                AntEnvironment::GetCellCenter(
                    {x, y});

            const float deltaX =
                cellCenter.x - center.x;

            const float deltaY =
                cellCenter.y - center.y;

            const float distanceSquared =
                deltaX * deltaX +
                deltaY * deltaY;

            if (distanceSquared >= radiusSquared) {
                continue;
            }

            if (!environment
                     .IsSimulationPositionValid(
                         cellCenter)) {
                continue;
            }

            if (callback(cellCenter)) {
                ++affectedCount;
            }
        }
    }

    return affectedCount;
}

} // namespace

AntEnvironment::AntEnvironment(
    const AntConfiguration &initialConfiguration
) {
    std::string ignoredError;

    Initialize(
        initialConfiguration,
        ignoredError);
}

bool AntEnvironment::Initialize(
    const AntConfiguration &newConfiguration,
    std::string &errorMessage
) {
    errorMessage.clear();

    if (!newConfiguration.Validate(errorMessage)) {
        return false;
    }

    if (newConfiguration.worldSize.x <=
            BorderMargin * 2 ||
        newConfiguration.worldSize.y <=
            BorderMargin * 2) {
        errorMessage =
            "Ant world dimensions must leave space inside the "
            "two-cell physics border.";

        return false;
    }

    configuration = newConfiguration;

    width = configuration.worldSize.x;
    height = configuration.worldSize.y;

    const std::size_t cellCount =
        static_cast<std::size_t>(width) *
        static_cast<std::size_t>(height);

    cells.assign(
        cellCount,
        AntWorldCell{});

    foodEntities.clear();
    foodEntityIndices.clear();
    nextFoodEntityId = 1;

    CreateBorderWalls();

    return true;
}

void AntEnvironment::Clear() {
    cells.clear();
    foodEntities.clear();
    foodEntityIndices.clear();

    nextFoodEntityId = 1;
    width = 0;
    height = 0;
}

void AntEnvironment::Update(
    const float deltaTime
) {
    if (deltaTime <= 0.0f) {
        return;
    }

    for (AntWorldCell &cell : cells) {
        cell.DecayMarkers(
            configuration.markerDecayRate,
            deltaTime);
    }
}

const AntConfiguration &
AntEnvironment::GetConfiguration() const {
    return configuration;
}

int AntEnvironment::GetWidth() const {
    return width;
}

int AntEnvironment::GetHeight() const {
    return height;
}

std::size_t AntEnvironment::GetCellCount() const {
    return cells.size();
}

bool AntEnvironment::IsInitialized() const {
    return width > 0 &&
           height > 0 &&
           !cells.empty();
}

bool AntEnvironment::ContainsCell(
    const int x,
    const int y
) const {
    return x >= 0 &&
           y >= 0 &&
           x < width &&
           y < height;
}

bool AntEnvironment::IsSimulationPositionValid(
    const pipeframe::Vector2f worldPosition
) const {
    if (!std::isfinite(worldPosition.x) ||
        !std::isfinite(worldPosition.y)) {
        return false;
    }

    const float minimum =
        static_cast<float>(BorderMargin);

    const float maximumX =
        static_cast<float>(
            width - BorderMargin);

    const float maximumY =
        static_cast<float>(
            height - BorderMargin);

    return worldPosition.x >= minimum &&
           worldPosition.y >= minimum &&
           worldPosition.x < maximumX &&
           worldPosition.y < maximumY;
}

pipeframe::Vector2i AntEnvironment::WorldToCell(
    const pipeframe::Vector2f worldPosition
) {
    return {
        static_cast<int>(
            std::floor(worldPosition.x)),
        static_cast<int>(
            std::floor(worldPosition.y)),
    };
}

pipeframe::Vector2f AntEnvironment::GetCellCenter(
    const pipeframe::Vector2i cellPosition
) {
    return {
        static_cast<float>(cellPosition.x) + 0.5f,
        static_cast<float>(cellPosition.y) + 0.5f,
    };
}

AntWorldCell *AntEnvironment::TryGetCell(
    const int x,
    const int y
) {
    if (!ContainsCell(x, y)) {
        return nullptr;
    }

    return &cells[GetCellIndex(x, y)];
}

const AntWorldCell *AntEnvironment::TryGetCell(
    const int x,
    const int y
) const {
    if (!ContainsCell(x, y)) {
        return nullptr;
    }

    return &cells[GetCellIndex(x, y)];
}

AntWorldCell *AntEnvironment::TryGetCell(
    const pipeframe::Vector2i cellPosition
) {
    return TryGetCell(
        cellPosition.x,
        cellPosition.y);
}

const AntWorldCell *AntEnvironment::TryGetCell(
    const pipeframe::Vector2i cellPosition
) const {
    return TryGetCell(
        cellPosition.x,
        cellPosition.y);
}

AntWorldCell *
AntEnvironment::TryGetCellAtWorldPosition(
    const pipeframe::Vector2f worldPosition
) {
    if (!IsSimulationPositionValid(worldPosition)) {
        return nullptr;
    }

    return TryGetCell(
        WorldToCell(worldPosition));
}

const AntWorldCell *
AntEnvironment::TryGetCellAtWorldPosition(
    const pipeframe::Vector2f worldPosition
) const {
    if (!IsSimulationPositionValid(worldPosition)) {
        return nullptr;
    }

    return TryGetCell(
        WorldToCell(worldPosition));
}

std::span<AntWorldCell>
AntEnvironment::GetCells() {
    return cells;
}

std::span<const AntWorldCell>
AntEnvironment::GetCells() const {
    return cells;
}

std::span<const Food>
AntEnvironment::GetFoodEntities() const {
    return foodEntities;
}

const Food *AntEnvironment::FindFoodEntity(
    const WorldEntityId id
) const {
    const auto iterator =
        foodEntityIndices.find(id);

    if (iterator == foodEntityIndices.end()) {
        return nullptr;
    }

    return &foodEntities[iterator->second];
}

std::size_t
AntEnvironment::GetTotalFoodQuantity() const {
    std::size_t totalFood{0};

    for (const AntWorldCell &cell : cells) {
        totalFood += cell.foodQuantity;
    }

    return totalFood;
}

WorldEntityId AntEnvironment::AddFood(
    const pipeframe::Vector2f worldPosition,
    const std::size_t quantity
) {
    if (quantity == 0) {
        return InvalidWorldEntityId;
    }

    AntWorldCell *cell =
        TryGetCellAtWorldPosition(worldPosition);

    if (cell == nullptr) {
        return InvalidWorldEntityId;
    }

    if (cell->foodQuantity == 0 ||
        cell->foodEntityId ==
            InvalidWorldEntityId) {
        const pipeframe::Vector2i cellPosition =
            WorldToCell(worldPosition);

        cell->foodEntityId =
            CreateFoodEntity(
                GetCellCenter(cellPosition));
    }

    const std::size_t maximumQuantity =
        std::numeric_limits<std::size_t>::max();

    if (quantity >
        maximumQuantity - cell->foodQuantity) {
        cell->foodQuantity = maximumQuantity;
    } else {
        cell->foodQuantity += quantity;
    }

    return cell->foodEntityId;
}

std::size_t AntEnvironment::AddFoodPatch(
    const pipeframe::Vector2f center,
    const float radius,
    const std::size_t quantityPerCell
) {
    if (!std::isfinite(center.x) ||
        !std::isfinite(center.y) ||
        !std::isfinite(radius) ||
        radius <= 0.0f ||
        quantityPerCell == 0) {
        return 0;
    }

    const float radiusSquared =
        radius * radius;

    std::size_t modifiedCellCount{0};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const pipeframe::Vector2f samplePosition{
                static_cast<float>(x),
                static_cast<float>(y),
            };

            const float deltaX =
                center.x - samplePosition.x;

            const float deltaY =
                center.y - samplePosition.y;

            const float distanceSquared =
                deltaX * deltaX +
                deltaY * deltaY;

            if (distanceSquared >= radiusSquared) {
                continue;
            }

            if (AddFood(
                    samplePosition,
                    quantityPerCell) !=
                InvalidWorldEntityId) {
                ++modifiedCellCount;
            }
        }
    }

    return modifiedCellCount;
}

std::size_t AntEnvironment::ConsumeFood(
    const pipeframe::Vector2f worldPosition,
    const std::size_t requestedQuantity
) {
    if (requestedQuantity == 0) {
        return 0;
    }

    AntWorldCell *cell =
        TryGetCellAtWorldPosition(worldPosition);

    if (cell == nullptr ||
        cell->foodQuantity == 0) {
        return 0;
    }

    const std::size_t consumedQuantity =
        std::min(
            requestedQuantity,
            cell->foodQuantity);

    cell->foodQuantity -= consumedQuantity;

    if (cell->foodQuantity == 0) {
        RemoveFoodEntity(
            cell->foodEntityId);

        cell->ClearFood();
    }

    return consumedQuantity;
}

bool AntEnvironment::RemoveFood(
    const pipeframe::Vector2f worldPosition
) {
    AntWorldCell *cell =
        TryGetCellAtWorldPosition(worldPosition);

    if (cell == nullptr ||
        cell->foodQuantity == 0) {
        return false;
    }

    RemoveFoodEntity(
        cell->foodEntityId);

    cell->ClearFood();

    return true;
}

void AntEnvironment::ClearAllFood() {
    for (AntWorldCell &cell : cells) {
        cell.ClearFood();
    }

    foodEntities.clear();
    foodEntityIndices.clear();
}

bool AntEnvironment::AddWall(
    const pipeframe::Vector2f worldPosition
) {
    AntWorldCell *cell =
        TryGetCellAtWorldPosition(worldPosition);

    if (cell == nullptr ||
        cell->wall) {
        return false;
    }

    cell->wall = true;

    WallBuilder::RebuildSamplingCoefficients(
        *this);

    return true;
}

bool AntEnvironment::RemoveWall(
    const pipeframe::Vector2f worldPosition
) {
    AntWorldCell *cell =
        TryGetCellAtWorldPosition(worldPosition);

    if (cell == nullptr ||
        !cell->wall) {
        return false;
    }

    cell->ClearWall();

    WallBuilder::RebuildSamplingCoefficients(
        *this);

    return true;
}

std::size_t AntEnvironment::GetWallCount() const {
    std::size_t count{0};

    for (const AntWorldCell &cell : cells) {
        if (cell.wall) {
            ++count;
        }
    }

    return count;
}

bool AntEnvironment::MarkCellForWall(
    const pipeframe::Vector2f worldPosition
) {
    AntWorldCell *cell =
        TryGetCellAtWorldPosition(worldPosition);

    if (cell == nullptr ||
        cell->editState !=
            WorldCellEditState::None) {
        return false;
    }

    cell->RequestWall();

    return true;
}

bool AntEnvironment::MarkCellForErase(
    const pipeframe::Vector2f worldPosition
) {
    AntWorldCell *cell =
        TryGetCellAtWorldPosition(worldPosition);

    if (cell == nullptr ||
        cell->editState !=
            WorldCellEditState::None ||
        (!cell->wall &&
         cell->foodQuantity == 0)) {
        return false;
    }

    cell->RequestErase();

    return true;
}

std::size_t AntEnvironment::MarkWallBrush(
    const pipeframe::Vector2f center,
    const float radius
) {
    return ForEachBrushCell(
        *this,
        center,
        radius,
        [this](const pipeframe::Vector2f position) {
            return MarkCellForWall(position);
        });
}

std::size_t AntEnvironment::MarkEraseBrush(
    const pipeframe::Vector2f center,
    const float radius
) {
    return ForEachBrushCell(
        *this,
        center,
        radius,
        [this](const pipeframe::Vector2f position) {
            return MarkCellForErase(position);
        });
}

std::size_t AntEnvironment::ApplyWallRequests() {
    std::size_t changedCellCount{0};

    for (AntWorldCell &cell : cells) {
        if (!cell.IsWallRequested()) {
            continue;
        }

        cell.ResetEditState();

        if (!cell.wall) {
            cell.wall = true;
            ++changedCellCount;
        }
    }

    WallBuilder::RebuildSamplingCoefficients(
        *this);

    return changedCellCount;
}

std::size_t AntEnvironment::ApplyEraseRequests() {
    std::size_t changedCellCount{0};

    for (AntWorldCell &cell : cells) {
        if (!cell.IsEraseRequested()) {
            continue;
        }

        cell.ResetEditState();

        const bool hadContent =
            cell.wall ||
            cell.foodQuantity > 0;

        if (cell.foodEntityId !=
            InvalidWorldEntityId) {
            RemoveFoodEntity(
                cell.foodEntityId);
        }

        cell.ClearFood();
        cell.ClearWall();

        if (hadContent) {
            ++changedCellCount;
        }
    }

    WallBuilder::RebuildSamplingCoefficients(
        *this);

    return changedCellCount;
}

void AntEnvironment::CreateBorderWalls() {
    if (!IsInitialized()) {
        return;
    }

    WallBuilder::CreateBorderWalls(
        *this);

    WallBuilder::RebuildSamplingCoefficients(
        *this);
}

std::size_t AntEnvironment::GetCellIndex(
    const int x,
    const int y
) const {
    return
        static_cast<std::size_t>(y) *
            static_cast<std::size_t>(width) +
        static_cast<std::size_t>(x);
}

WorldEntityId AntEnvironment::CreateFoodEntity(
    const pipeframe::Vector2f position
) {
    const WorldEntityId id =
        nextFoodEntityId++;

    const std::size_t index =
        foodEntities.size();

    foodEntities.push_back(
        Food{
            id,
            position,
        });

    foodEntityIndices.emplace(
        id,
        index);

    return id;
}

void AntEnvironment::RemoveFoodEntity(
    const WorldEntityId id
) {
    const auto iterator =
        foodEntityIndices.find(id);

    if (iterator == foodEntityIndices.end()) {
        return;
    }

    const std::size_t removedIndex =
        iterator->second;

    const std::size_t lastIndex =
        foodEntities.size() - 1;

    if (removedIndex != lastIndex) {
        foodEntities[removedIndex] =
            foodEntities[lastIndex];

        foodEntityIndices[
            foodEntities[removedIndex].id
        ] = removedIndex;
    }

    foodEntities.pop_back();
    foodEntityIndices.erase(iterator);
}

} // namespace ant_simulation