#include "RuntimeDebugInspector.h"

#include <cmath>

#include "imgui.h"

#include "Components/RigidBodyComponent.h"
#include "Components/MovementStatusComponent.h"
#include "Components/TransformComponent.h"
#include "ECS/ECS.h"
#include "Map/TileMap.h"

void RuntimeDebugInspector::Draw(
    EngineMode mode,
    Entity selectedEntity,
    const TileMap* tileMap,
    const std::unordered_map<std::string, FieldGrid>& fieldGrids
)
{
    if (mode != EngineMode::Play)
    {
        return;
    }

    ImGui::SeparatorText("Runtime Debug");
    ImGui::BeginDisabled();

    ImGui::Text("Runtime ID: %d", selectedEntity.GetId());

    if (selectedEntity.HasComponent<TransformComponent>())
    {
        const auto& transform = selectedEntity.GetComponent<TransformComponent>();
        ImGui::Text("Position: %.3f, %.3f", transform.position.x, transform.position.y);

        if (tileMap)
        {
            const TerrainType terrain = tileMap->GetTerrainAtWorldPosition(
                transform.position.x,
                transform.position.y
            );
            ImGui::Text("Terrain: %s", TerrainTypeToString(terrain));
        }

        if (!fieldGrids.empty() && ImGui::TreeNode("Field Samples"))
        {
            for (const auto& [fieldName, fieldGrid] : fieldGrids)
            {
                const double value = SampleFieldAt(
                    fieldGrid,
                    transform.position.x,
                    transform.position.y
                );
                ImGui::Text("%s: %.3f", fieldName.c_str(), value);
            }

            ImGui::TreePop();
        }
    }

    if (selectedEntity.HasComponent<RigidBodyComponent>())
    {
        const auto& rigidbody = selectedEntity.GetComponent<RigidBodyComponent>();
        ImGui::Text("Velocity: %.3f, %.3f", rigidbody.velocity.x, rigidbody.velocity.y);
    }

    if (selectedEntity.HasComponent<MovementStatusComponent>())
    {
        const auto& movementStatus = selectedEntity.GetComponent<MovementStatusComponent>();
        ImGui::Text(
            "Movement Blocked: %s",
            movementStatus.wasBlocked ? "true" : "false"
        );
        ImGui::Text(
            "Block Axes: x=%s y=%s collision=%s",
            movementStatus.blockedX ? "true" : "false",
            movementStatus.blockedY ? "true" : "false",
            movementStatus.blockedByCollision ? "true" : "false"
        );
    }

    ImGui::EndDisabled();
}

double RuntimeDebugInspector::SampleFieldAt(
    const FieldGrid& fieldGrid,
    float worldX,
    float worldY
) const
{
    if (fieldGrid.cellWorldSize <= 0.0f || fieldGrid.rows <= 0 || fieldGrid.cols <= 0)
    {
        return 0.0;
    }

    const int col = static_cast<int>(std::floor(worldX / fieldGrid.cellWorldSize));
    const int row = static_cast<int>(std::floor(worldY / fieldGrid.cellWorldSize));

    if (row < 0 || col < 0 || row >= fieldGrid.rows || col >= fieldGrid.cols)
    {
        return 0.0;
    }

    const std::size_t index = static_cast<std::size_t>(row) *
        static_cast<std::size_t>(fieldGrid.cols) +
        static_cast<std::size_t>(col);

    if (index >= fieldGrid.values.size())
    {
        return 0.0;
    }

    return fieldGrid.values[index];
}

const char* RuntimeDebugInspector::TerrainTypeToString(TerrainType terrain) const
{
    switch (terrain)
    {
    case TerrainType::Land:
        return "land";
    case TerrainType::Water:
        return "water";
    case TerrainType::Runway:
        return "runway";
    case TerrainType::Blocked:
        return "blocked";
    }

    return "unknown";
}
