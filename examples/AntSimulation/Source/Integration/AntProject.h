#pragma once
#include <PipeFrame/Project/ProjectDefinition.h>
namespace ant_simulation {
class AntWorld;
class AntProject {
public:
  static const pipeframe::ProjectDefinition<AntWorld>& Definition();
  static const pipeframe::EntityRegistry& EntityTypes();

private:
  static void Configure(pipeframe::ProjectDefinition<AntWorld>& project);
};
}  // namespace ant_simulation
