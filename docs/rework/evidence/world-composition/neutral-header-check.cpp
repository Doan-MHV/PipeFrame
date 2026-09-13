#include "World/AntWorld.h"
#include "World/Runtime/AntRuntimeWorld.h"
#include "World/Physics/AntPhysicsWorld.h"
#include "World/Rendering/AntRenderingWorld.h"
static_assert(std::is_base_of_v<pipeframe::World,ant_simulation::AntWorld>);
static_assert(std::is_base_of_v<pipeframe::RuntimeWorld,ant_simulation::AntRuntimeWorld>);
static_assert(std::is_base_of_v<pipeframe::PhysicsWorld,ant_simulation::AntPhysicsWorld>);
static_assert(std::is_base_of_v<pipeframe::RenderingWorld,ant_simulation::AntRenderingWorld>);
