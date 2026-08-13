#ifndef PIPEFRAME_ANT_POPULATION_H
#define PIPEFRAME_ANT_POPULATION_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include <SFML/System/Vector2.hpp>

struct Ant {
    sf::Vector2f position;
    sf::Vector2f velocity;
};

class AntPopulation {
  public:
    void Initialize(std::size_t antCount, std::uint32_t seed);

    void Update(float fixedDeltaTime);

    const std::vector<Ant> &GetAnts() const;
    std::size_t GetCount() const;

    static constexpr float WorldHalfWidth = 900.0f;
    static constexpr float WorldHalfHeight = 500.0f;

  private:
    std::vector<Ant> ants;
};

#endif