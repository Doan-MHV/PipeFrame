#include "World/Runtime/Environment/AntWorldCell.h"
#include "World/Runtime/Environment/Marker.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void Require(
    const bool condition,
    const char *message
) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

bool NearlyEqual(
    const float first,
    const float second,
    const float tolerance = 0.0001f
) {
    return std::abs(first - second) <= tolerance;
}

} // namespace

int main() {
    using namespace ant_simulation;

    constexpr ColonyId FirstColony{1};
    constexpr ColonyId SecondColony{2};

    Marker marker{
        FirstColony,
        100.0f,
    };

    marker.Decay(
        0.1f,
        2.0f);

    Require(
        NearlyEqual(marker.intensity, 80.0f),
        "A non-persistent marker should decay.");

    marker.persistent = true;

    marker.Decay(
        0.1f,
        2.0f);

    Require(
        NearlyEqual(marker.intensity, 80.0f),
        "A persistent marker should not decay.");

    marker.Clear();

    Require(
        !marker.HasOwner(),
        "Clearing a marker should remove its owner.");

    Require(
        NearlyEqual(marker.intensity, 0.0f),
        "Clearing a marker should remove its intensity.");

    Require(
        !marker.persistent,
        "Clearing a marker should remove persistence.");

    AntWorldCell cell;

    Require(
        cell.IsEmpty(),
        "A new world cell should have no navigation markers.");

    cell.AddMarker(
        MarkerKind::ToFood,
        20.0f,
        FirstColony);

    const Marker &firstFoodMarker =
        cell.GetMarker(MarkerKind::ToFood);

    Require(
        firstFoodMarker.colonyId == FirstColony,
        "The first colony should claim an empty marker.");

    Require(
        NearlyEqual(firstFoodMarker.intensity, 20.0f),
        "The first marker deposit should set its intensity.");

    cell.AddMarker(
        MarkerKind::ToFood,
        5.0f,
        FirstColony);

    Require(
        NearlyEqual(
            cell.GetMarker(MarkerKind::ToFood).intensity,
            25.0f),
        "A matching colony should reinforce its marker.");

    cell.AddMarker(
        MarkerKind::ToFood,
        10.0f,
        SecondColony);

    Require(
        cell.GetMarker(MarkerKind::ToFood).colonyId ==
            FirstColony,
        "A conflicting colony should not immediately take ownership.");

    Require(
        NearlyEqual(
            cell.GetMarker(MarkerKind::ToFood).intensity,
            15.0f),
        "A conflicting marker should degrade the existing marker.");

    cell.AddMarker(
        MarkerKind::ToFood,
        20.0f,
        SecondColony);

    Require(
        cell.GetMarker(MarkerKind::ToFood).colonyId ==
            FirstColony,
        "Ownership should remain until a dissipated marker is deposited again.");

    Require(
        cell.GetMarker(MarkerKind::ToFood).intensity <= 0.0f,
        "A stronger conflicting deposit should dissipate the marker.");

    cell.AddMarker(
        MarkerKind::ToFood,
        12.0f,
        SecondColony);

    Require(
        cell.GetMarker(MarkerKind::ToFood).colonyId ==
            SecondColony,
        "A colony should claim a dissipated marker.");

    Require(
        NearlyEqual(
            cell.GetMarker(MarkerKind::ToFood).intensity,
            12.0f),
        "Claiming a marker should install the new intensity.");

    cell.SetPersistentMarker(
        MarkerKind::ToHome,
        1'000.0f,
        FirstColony);

    cell.DecayMarkers(
        0.5f,
        10.0f);

    Require(
        NearlyEqual(
            cell.GetMarker(MarkerKind::ToHome).intensity,
            1'000.0f),
        "A colony home marker should remain persistent.");

    cell.AddMarker(
        MarkerKind::ToHome,
        500.0f,
        SecondColony);

    Require(
        cell.GetMarker(MarkerKind::ToHome).colonyId ==
            FirstColony,
        "A persistent home marker should reject another colony.");

    Require(
        NearlyEqual(
            cell.GetMarker(MarkerKind::ToHome).intensity,
            1'000.0f),
        "A persistent home marker should not be degraded.");

    cell.AddMarker(
        MarkerKind::ToEnemy,
        30.0f,
        FirstColony);

    Require(
        cell.GetMarker(MarkerKind::ToEnemy).colonyId ==
            FirstColony,
        "Enemy markers should use their own channel.");

    cell.RequestWall();

    Require(
        cell.IsWallRequested(),
        "RequestWall should set the wall edit state.");

    cell.ResetEditState();

    Require(
        !cell.IsWallRequested(),
        "ResetEditState should clear the wall request.");

    cell.RequestErase();

    Require(
        cell.IsEraseRequested(),
        "RequestErase should set the erase edit state.");

    cell.foodQuantity = 42;
    cell.foodEntityId = 10;

    cell.ClearFood();

    Require(
        cell.foodQuantity == 0,
        "ClearFood should remove the cell food.");

    Require(
        cell.foodEntityId == InvalidWorldEntityId,
        "ClearFood should remove the food entity association.");

    cell.wall = true;
    cell.physicsObjectId = 20;
    cell.markerSamplingCoefficient = 0.25f;

    cell.ClearWall();

    Require(
        !cell.wall,
        "ClearWall should remove the wall.");

    Require(
        cell.physicsObjectId == InvalidWorldEntityId,
        "ClearWall should remove its physics association.");

    Require(
        NearlyEqual(
            cell.markerSamplingCoefficient,
            1.0f),
        "ClearWall should restore marker sampling.");

    std::cout << "All marker and world-cell tests passed.\n";

    return 0;
}
