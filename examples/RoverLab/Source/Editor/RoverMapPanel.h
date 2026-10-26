#pragma once

#include <PipeFrame/UI/ProjectUIHost.h>
#include <PipeFrame/UI/ViewPanel.h>

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Editor/RoverMapCamera.h"
#include "World/ProjectWorld.h"

namespace RoverLab {

class RoverMapPanel final : public pipeframe::ui::ViewPanel {
public:
  RoverMapPanel(ProjectWorld& world, pipeframe::ui::ProjectUIHost&) : world(world) {}

  void OnUpdate(float delta) override {
    elapsed += delta;
    if (elapsed >= 0.1f) {
      elapsed = 0;
      RefreshView();
    }
  }

protected:
  pipeframe::ui::View BuildView() override {
    using namespace pipeframe::ui;
    std::vector<View> children;
    children.push_back(views::Text("legend",
      "Grey: free | Amber: uncertain | Red: occupied\n"
      "Blue: rover | Dark: unknown\nCyan: route | Yellow: destination\n"
      "Drag to pan; wheel to zoom. Select destinations in the scene using Rover Mission. +Y down.")
        .FitHeight());

    auto& mapping = world.simulation.mapping;
    const auto entities = mapping.Entities();
    std::erase_if(cameras, [&](const auto& entry) { return !mapping.Find(entry.first); });
    if (entities.empty()) {
      children.push_back(
        views::Text("empty", "Add Rover Mapping to the rover, then press Play.").FitHeight());
    }

    for (const auto entity : entities) {
      const auto* map = mapping.Find(entity);
      if (!map) continue;
      auto& camera = cameras[entity];
      camera.Track(*map);
      const auto key = std::to_string(entity);
      std::ostringstream text;
      text << std::fixed << std::setprecision(2) << "Estimated: (" << map->pose.x << ", "
           << map->pose.y << ") m | View: " << camera.view.span << " m\nMap: " << map->Width()
           << " x " << map->Height() << " m | "
           << (camera.following ? "Following rover" : "Manual view");
      children.push_back(views::Text("info-" + key, text.str()).FitHeight());
      children.push_back(views::Row("controls-" + key,
        {
          views::Button("follow",
            camera.following ? "FOLLOW ON" : "FOLLOW ROVER",
            [this, entity] {
              if (const auto* current = world.simulation.mapping.Find(entity)) {
                auto& value = cameras[entity];
                value.following = true;
                value.Center(*current);
                RefreshView();
              }
            })
            .Selected(camera.following),
          views::Button("fit",
            "FIT EXPLORED",
            [this, entity] {
              if (const auto* current = world.simulation.mapping.Find(entity)) {
                cameras[entity].Fit(*current);
                RefreshView();
              }
            }),
          views::Button("out", "-", [this, entity] { Zoom(entity, -2); }).Width(32),
          views::Button("in", "+", [this, entity] { Zoom(entity, 2); }).Width(32),
        }));
      auto viewport = views::Viewport(
        "map-" + key,
        {BuildMesh(*map, camera), BuildRoute(entity, camera)},
        camera.view,
        [this, entity](const Viewport2D& value, bool panned) {
          cameras[entity].Change(value, panned);
          RefreshView();
        },
        !camera.following)
                        .Height(300);
      viewport.onViewportResized = [this, entity](pipeframe::Vector2f size) {
        cameras[entity].viewportSize = size;
        RefreshView();
      };
      children.push_back(std::move(viewport));
      std::ostringstream status;
      if (const auto* mission = world.simulation.solver.Mission(entity);
        mission && mission->timed) {
        status << MissionPhaseText(mission->phase) << '\n' << mission->selectionMessage << '\n';
      }
      status << "Accepted: " << map->accepted << " | Ignored: " << map->ignored << '\n'
             << map->status;
      if (map->storageLimited)
        status << "\nStorage limit reached (4M cells); existing map retained.";
      children.push_back(views::Text("status-" + key, status.str()).FitHeight());
    }
    return views::Column("map-panel",
      {
        views::Row("map-header",
          {
            views::Text("map-title", "DISCOVERED MINIMAP"),
            views::Button("map-close", "CLOSE", [this] { Hide(); }).Width(80),
          })
          .Padding(12)
          .Spacing(8)
          .FitHeight(),
        views::Scroll("map-scroll",
          views::Column("map-content", std::move(children)).Padding(12).Spacing(8).FitHeight())
          .FillHeight(),
      })
      .FillHeight();
  }

private:
  using Mesh = pipeframe::ui::View::MeshLayer;

  Mesh BuildRoute(pipeframe::ecs::Entity entity, const RoverMapCamera& camera) const {
    Mesh mesh;
    const float width = camera.view.span * .006f;
    const auto route = world.simulation.solver.PlannedPath(entity);
    const pipeframe::Color cyan{40, 240, 225}, yellow{255, 215, 45};
    for (std::size_t i = 1; i < route.size(); ++i) {
      const auto a = route[i - 1], b = route[i];
      const double distance = std::hypot(b.x - a.x, b.y - a.y);
      if (distance < 1e-8) continue;
      const float dx = float(-(b.y - a.y) / distance) * width / 2;
      const float dy = float((b.x - a.x) / distance) * width / 2;
      const pipeframe::Vector2f p{float(a.x) + dx, float(a.y) + dy};
      const pipeframe::Vector2f q{float(b.x) + dx, float(b.y) + dy};
      const pipeframe::Vector2f r{float(b.x) - dx, float(b.y) - dy};
      const pipeframe::Vector2f t{float(a.x) - dx, float(a.y) - dy};
      mesh.vertices.insert(
        mesh.vertices.end(), {{p, cyan}, {q, cyan}, {r, cyan}, {p, cyan}, {r, cyan}, {t, cyan}});
    }
    if (const auto* mission = world.simulation.solver.Mission(entity);
      mission && mission->destination) {
      const auto point = *mission->destination;
      const float radius = width * 2;
      Quad(mesh, float(point.x) - radius, float(point.y) - width / 2, radius * 2, width, yellow);
      Quad(mesh, float(point.x) - width / 2, float(point.y) - radius, width, radius * 2, yellow);
    }
    return mesh;
  }

  void Zoom(pipeframe::ecs::Entity entity, float steps) {
    cameras[entity].view.Zoom(steps);
    RefreshView();
  }

  static void Quad(
    Mesh& mesh, float x, float y, float width, float height, pipeframe::Color color) {
    const pipeframe::Vector2f a{x, y}, b{x + width, y}, c{x + width, y + height}, d{x, y + height};
    mesh.vertices.insert(mesh.vertices.end(),
      {
        {a, color},
        {b, color},
        {c, color},
        {a, color},
        {c, color},
        {d, color},
      });
  }

  static Mesh BuildMesh(const RoverMappingSystem::Map& map, const RoverMapCamera& state) {
    const auto& camera = state.view;
    const auto half = state.HalfView();
    Mesh mesh{};
    const double cell = map.settings.cellSize;
    // Only emit visible cells; storage growth does not increase a close-up's draw cost.
    const auto bound = [&](double coordinate, double origin, int count) {
      return int(std::clamp(std::floor((coordinate - origin) / cell), 0.0, double(count)));
    };
    const int left = bound(camera.center.x - half.x, map.originX, map.width);
    const int right = bound(camera.center.x + half.x + cell, map.originX, map.width);
    const int top = bound(camera.center.y - half.y, map.originY, map.height);
    const int bottom = bound(camera.center.y + half.y + cell, map.originY, map.height);
    // At wide zoom levels, summarize roughly two screen pixels per quad.
    // Underlying evidence remains full resolution; this bounds mesh allocation.
    const double pixels = std::max(1.0f, std::min(state.viewportSize.x, state.viewportSize.y));
    const int stride = std::max(1, int(std::ceil(camera.span / (pixels * cell) * 2)));
    for (int row = top; row < bottom; row += stride) {
      for (int col = left; col < right; col += stride) {
        int positive = 0, negative = 0;
        const int endRow = std::min(row + stride, bottom);
        const int endCol = std::min(col + stride, right);
        for (int y = row; y < endRow; ++y) {
          for (int x = col; x < endCol; ++x) {
            const int value = map.cells[std::size_t(y * map.width + x)];
            positive = std::max(positive, value);
            negative = std::min(negative, value);
          }
        }
        const int score = positive ? positive : negative;
        if (!score) continue;
        const pipeframe::Color color = score >= 3    ? pipeframe::Color{220, 85, 75}
                                       : score > 0   ? pipeframe::Color{200, 155, 65}
                                       : score <= -3 ? pipeframe::Color{145, 165, 175}
                                                     : pipeframe::Color{65, 80, 90};
        Quad(mesh,
          float(map.originX + col * cell),
          float(map.originY + row * cell),
          float((endCol - col) * cell),
          float((endRow - row) * cell),
          color);
      }
    }
    // Keep the estimated rover marker legible at every zoom, even at a storage limit.
    const float c = float(std::cos(map.pose.heading)), s = float(std::sin(map.pose.heading));
    const auto point = [&](float forward, float sideward) {
      return pipeframe::Vector2f{float(map.pose.x) + camera.span * (forward * c - sideward * s),
        float(map.pose.y) + camera.span * (forward * s + sideward * c)};
    };
    const pipeframe::Color blue{40, 180, 255};
    mesh.vertices.insert(mesh.vertices.end(),
      {
        {point(0.024f, 0), blue},
        {point(-0.016f, -0.014f), blue},
        {point(-0.016f, 0.014f), blue},
      });
    return mesh;
  }

  ProjectWorld& world;
  std::unordered_map<pipeframe::ecs::Entity, RoverMapCamera> cameras;
  float elapsed{};
};
}  // namespace RoverLab
