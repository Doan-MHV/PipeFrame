#include "AntPopulation.h"

#include <random>

void AntPopulation::Initialize(std::size_t antCount, std::uint32_t seed) {
    ants.clear();
    ants.reserve(antCount);

    std::mt19937 randomGenerator(seed);

    std::uniform_real_distribution positionX(-WorldHalfWidth, WorldHalfWidth);

    std::uniform_real_distribution positionY(-WorldHalfHeight, WorldHalfHeight);

    std::uniform_real_distribution velocity(-120.0f, 120.0f);

    for (std::size_t index = 0; index < antCount; ++index) {
        Ant ant;

        ant.position = {positionX(randomGenerator), positionY(randomGenerator)};

        ant.velocity = {velocity(randomGenerator), velocity(randomGenerator)};

        ants.push_back(ant);
    }
}

void AntPopulation::Update(float fixedDeltaTime) {
    for (Ant &ant : ants) {
        ant.position += ant.velocity * fixedDeltaTime;

        if (ant.position.x < -WorldHalfWidth) {
            ant.position.x = -WorldHalfWidth;
            ant.velocity.x = -ant.velocity.x;
        } else if (ant.position.x > WorldHalfWidth) {
            ant.position.x = WorldHalfWidth;
            ant.velocity.x = -ant.velocity.x;
        }

        if (ant.position.y < -WorldHalfHeight) {
            ant.position.y = -WorldHalfHeight;
            ant.velocity.y = -ant.velocity.y;
        } else if (ant.position.y > WorldHalfHeight) {
            ant.position.y = WorldHalfHeight;
            ant.velocity.y = -ant.velocity.y;
        }
    }
}

const std::vector<Ant> &AntPopulation::GetAnts() const { return ants; }

std::size_t AntPopulation::GetCount() const { return ants.size(); }