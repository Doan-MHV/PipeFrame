#include "Runtime/AntSimulationRuntime.h"
#include "Components/AntSimulationTypes.h"
#include "Runtime/SailBoatSimulationRuntime.h"
#include "Components/SailBoatSimulationTypes.h"
#include <PipeFrame/Render/RenderContext.h>
#include <PipeFrame/Backend/SFML/InputEventAdapter.h>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#if defined(__APPLE__)
#include <malloc/malloc.h>
#endif

using Clock = std::chrono::steady_clock;
struct Heap {
    std::size_t blocks = 0, bytes = 0, highWater = 0;
};
Heap ReadHeap() {
#if defined(__APPLE__)
    malloc_statistics_t stats{};
    malloc_zone_statistics(nullptr, &stats);
    return {stats.blocks_in_use, stats.size_in_use, stats.max_size_in_use};
#else
    return {};
#endif
}
std::size_t Nodes(const Widget &root) {
    std::size_t count = 1;
    for (std::size_t i = 0; i < root.GetChildCount(); ++i)
        count += Nodes(*root.GetChild(i));
    return count;
}
template <class Fn> double Time(unsigned repetitions, Fn fn) {
    const auto start = Clock::now();
    for (unsigned i = 0; i < repetitions; ++i)
        fn(i);
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count() / repetitions;
}
void Profile(pipeframe::ProjectRuntime &runtime, SimulationDashboard &dashboard, RenderContext &context,
             const char *name, std::size_t population, std::ostream &out) {
    runtime.Start();
    runtime.FixedUpdate(0.001f);
    context.BeginWorld();
    runtime.Render(context);
    context.BeginScreen();
    runtime.RenderScreen(context);
    sf::RenderTexture target({1000, 800});
    const auto count = Nodes(dashboard.Root());
    const std::size_t surfaceCount = std::max<std::size_t>(1, dashboard.GetIndependentDrawerCount());
    for (std::size_t page = 0; page < surfaceCount; ++page) {
        if (page < dashboard.GetIndependentDrawerCount())
            dashboard.GetIndependentDrawer(page).Open(false);
        dashboard.Update(0.3f);
        for (unsigned i = 0; i < 8; ++i) {
            runtime.RenderScreen(context);
            dashboard.Render(target);
            target.display();
        }
        const auto before = ReadHeap();
        const double layout = Time(24, [&](unsigned i) { dashboard.Layout({{}, {1000.f + float(i % 2), 800}}); });
        dashboard.Layout({{}, {1000, 800}});
        const double dispatch = Time(
            500, [&](unsigned i) {
                const auto event = pipeframe::backend::sfml::FromBackend(
                    sf::Event{sf::Event::MouseMoved{{50 + int(i % 2), 150}}});
                runtime.HandleUIEvent(*event, context);
            });
        const double render = Time(24, [&](unsigned) {
            target.clear();
            dashboard.Render(target);
            target.display();
        });
        const double refresh = Time(24, [&](unsigned) { runtime.RenderScreen(context); });
        const auto after = ReadHeap();
        out << name << ',' << population << ',' << page << ',' << layout << ',' << dispatch << ',' << render << ','
            << refresh << ',' << count << ',' << before.blocks << ',' << after.blocks << ',' << before.bytes << ','
            << after.bytes << ',' << after.highWater << '\n';
        if (Nodes(dashboard.Root()) != count)
            throw std::runtime_error("Steady-state profiling grew the retained UI tree");
        if (after.bytes > before.bytes + 32 * 1024 * 1024)
            throw std::runtime_error("Dashboard retained more than 32 MiB after warmup");
        if (layout > 100 || dispatch > 20 || render > 100 || refresh > 100)
            throw std::runtime_error("Dashboard exceeded broad UI latency budget");
    }
    runtime.Stop();
}
int main(int argc, char **argv) {
    try {
        sf::RenderWindow window(sf::VideoMode({1000, 800}), "UI stress profile");
        window.setVisible(false);
        RenderContext context(window);
        std::ofstream file;
        if (argc > 1)
            file.open(argv[1]);
        std::ostream &out = file.is_open() ? file : std::cout;
        out << "project,population,page,layout_ms,event_ms,render_submit_ms,refresh_render_ms,widgets,heap_blocks_"
               "before,heap_blocks_after,heap_bytes_before,heap_bytes_after,heap_high_water\n";
        std::string error;
        for (const auto count : {10000u, 100000u}) {
            ant_simulation::AntSimulationRuntime ant;
            if (!ant.Load({std::filesystem::path(PIPEFRAME_PROFILE_EXAMPLES) / "AntSimulation"}, error))
                throw std::runtime_error(error);
            auto colony = ant.CreateDefaultObject(ant_simulation::ColonyTypeId);
            colony.id = 1;
            colony.properties[ant_simulation::InitialPopulationKey] = std::int64_t{1};
            ant.SynchronizeScene(std::array{colony});
            auto &world = *ant.GetSimulationWorld();
            auto &store = world.GetAntStore();
            store.Clear();
            const auto &configuration = world.GetConfiguration();
            const auto size = configuration.GetWorldSizeFloat();
            for (unsigned i = 0; i < count; ++i)
                store.Create(1, ant_simulation::AntRole::Follower,
                             {size.x * (0.05f + 0.9f * float(i % 400) / 400),
                              size.y * (0.05f + 0.9f * float(i / 400) / std::max(1u, count / 400))},
                             0, 0, configuration);
            world.GetPhysicsWorld().Synchronize();
            Profile(ant, *ant.GetDashboard(), context, "Ant", count, out);
            ant.Unload();
        }
        sailboat_simulation::SailBoatSimulationRuntime boat;
        if (!boat.Load({std::filesystem::path(PIPEFRAME_PROFILE_EXAMPLES) / "SailBoatSimulation"}, error))
            throw std::runtime_error(error);
        auto start = boat.CreateDefaultObject(sailboat_simulation::RaceStartTypeId);
        start.id = 1;
        auto finish = boat.CreateDefaultObject(sailboat_simulation::FinishLineTypeId);
        finish.id = 2;
        finish.transform.position = {1400, 1400};
        auto settings = boat.CreateDefaultObject(sailboat_simulation::TrainingSettingsTypeId);
        settings.id = 3;
        settings.properties[sailboat_simulation::PopulationSizeKey] = std::int64_t{10000};
        settings.properties[sailboat_simulation::MaximumIterationTimeKey] = 3600.0;
        boat.SynchronizeScene(std::array{start, finish, settings});
        if (!boat.GetPopulationTrainer().IsInitialized())
            throw std::runtime_error("SailBoat profile population did not initialize");
        Profile(boat, *boat.GetDashboard(), context, "SailBoat", 10000, out);
        boat.Unload();
        std::cout << "Dashboard stress profiles passed.\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
