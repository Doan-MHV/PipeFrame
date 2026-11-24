#include "World/Physics/ContactSolver.h"

#include <PipeFrame/Physics/GridCollision2D.h>
#include <PipeFrame/Physics/Physics2D.h>

#include <algorithm>
#include <cmath>

#include "World/Simulation/Environment/AntWorldCell.h"

namespace ant_simulation {

namespace {

// Keep solved positions outside IEEE-754 float rounding at large map coordinates
// so SweepCircleGrid never mistakes a resting contact for initial penetration.
constexpr float WallSeparationEpsilon = 2.0e-4f;

bool IsWallCell(const AntEnvironment &environment, const int column, const int row) {
    const auto *value = environment.TryGetCell(column, row);
    return value && value->wall;
}

bool DepenetrateFromWalls(const AntEnvironment &environment, pipeframe::Vector2f &position, const float radius,
                          pipeframe::Vector2f *velocity) {
    const AntWorldCell *occupied = environment.TryGetCellAtWorldPosition(position);
    if (!occupied || occupied->wall) {
        return false;
    }

    const int width = environment.GetWidth();
    const int height = environment.GetHeight();
    if (width <= 0 || height <= 0 || radius <= 0.0f) {
        return false;
    }

    const float radiusSquared = radius * radius;
    const float safeRadius = radius + WallSeparationEpsilon;
    bool adjusted = false;

    for (int pass = 0; pass < 4; ++pass) {
        bool passAdjusted = false;
        const int minCol = std::clamp(static_cast<int>(std::floor(position.x - radius)) - 1, 0, width - 1);
        const int maxCol = std::clamp(static_cast<int>(std::floor(position.x + radius)) + 1, 0, width - 1);
        const int minRow = std::clamp(static_cast<int>(std::floor(position.y - radius)) - 1, 0, height - 1);
        const int maxRow = std::clamp(static_cast<int>(std::floor(position.y + radius)) + 1, 0, height - 1);

        for (int row = minRow; row <= maxRow; ++row) {
            for (int col = minCol; col <= maxCol; ++col) {
                if (!IsWallCell(environment, col, row)) {
                    continue;
                }

                const float x0 = static_cast<float>(col);
                const float y0 = static_cast<float>(row);
                const pipeframe::Vector2f closest{
                    std::clamp(position.x, x0, x0 + 1.0f),
                    std::clamp(position.y, y0, y0 + 1.0f),
                };
                const pipeframe::Vector2f diff = position - closest;
                const float distSq = pipeframe::LengthSquared(diff);
                if (distSq >= radiusSquared) {
                    continue;
                }

                const pipeframe::Vector2f normal =
                    distSq > 1.0e-12f ? diff / std::sqrt(distSq)
                                      : pipeframe::NormalizeOr(position - pipeframe::Vector2f{x0 + 0.5f, y0 + 0.5f});
                position = closest + normal * safeRadius;
                if (velocity != nullptr) {
                    const float inward = pipeframe::ShapeDot(*velocity, normal);
                    if (inward < 0.0f) {
                        *velocity -= normal * inward;
                    }
                }
                adjusted = true;
                passAdjusted = true;
            }
        }
        if (!passAdjusted) {
            break;
        }
    }

    return adjusted;
}

} // namespace

ContactSolver::ContactSolver(AntQuery &antStore, AntEnvironment &newEnvironment) : environment(newEnvironment) {
    (void)antStore;
}

ContactSolverResult ContactSolver::Solve(AntBodySystem &physicsWorld) {
    ContactSolverResult result;
    // Both solvers consume body storage only. Publish the final solved positions
    // once, after all constraints; intermediate ECS writes have no reader.
    for (AntPhysicsBody &body : physicsWorld.GetBodies())
        if (SolveWallConstraint(body))
            ++result.wallConstraints;

    auto bodies = physicsWorld.GetBodies();
    result.antContacts = pipeframe::SolveCircleContacts<AntPhysicsBody>(
        bodies,
        {{0.0f, 0.0f}, {static_cast<float>(environment.GetWidth()), static_cast<float>(environment.GetHeight())}},
        contacts, [](const pipeframe::Contact2D &) {});

    // Ant-vs-ant contact resolution can push a neighbor slightly into an adjacent
    // wall's radius margin; clamp back out so SweepCircleGrid never locks on the next tick.
    for (AntPhysicsBody &body : bodies) {
        if (DepenetrateFromWalls(environment, body.position, body.radius, &body.velocity)) {
            body.previousPosition = body.position;
        }
    }

    physicsWorld.SynchronizeAntPositions();
    return result;
}

bool ContactSolver::SolveWallConstraint(AntPhysicsBody &body) const {
    const auto originalStart = body.position - body.lastMove;
    auto start = originalStart;
    const bool startPenetrated = DepenetrateFromWalls(environment, start, body.radius, &body.velocity);
    const auto desiredDisplacement = body.position - start;

    const auto motion = pipeframe::MoveCircle(
        {start, body.radius}, desiredDisplacement, body.velocity,
        [&](auto circle, auto delta) -> std::optional<pipeframe::ShapeHit2D> {
            const auto hit = pipeframe::SweepCircleGrid(
                circle, delta, {}, 1, environment.GetWidth(), environment.GetHeight(), [&](auto cell) {
                    return IsWallCell(environment, cell.column, cell.row);
                });
            return hit ? std::optional{hit->contact} : std::nullopt;
        });

    if (!motion.collided && !startPenetrated) {
        return false;
    }

    auto solvedPosition = motion.position;
    auto solvedVelocity = motion.velocity;
    DepenetrateFromWalls(environment, solvedPosition, body.radius, &solvedVelocity);

    body.position = solvedPosition;
    body.previousPosition = solvedPosition;
    body.lastMove = solvedPosition - originalStart;
    body.velocity = solvedVelocity;
    return true;
}

} // namespace ant_simulation
