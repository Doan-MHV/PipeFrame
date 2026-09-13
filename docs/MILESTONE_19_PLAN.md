# Milestone 19 — Modular robotics simulation

Status: planned; begins only after Milestone 18 satisfies its robot-readiness
contract. This milestone delivers PipeFrame's third reference project: a
brand-independent system for assembling and programming mobile robots from
mechanical, electrical, sensing, and control parts. An ELEGOO UNO R3 Smart Robot
Car style configuration is an initial validation target, not the project model.

## Required environment prerequisite

Before this milestone starts, the [R6-E environment authoring gate](rework/R6_ENVIRONMENT_AUTHORING.md)
must pass: a blank project can author textured or colored ground, tile layers, walls,
obstacles, spawn and goal regions; save/reopen them; and use shared collision/query data.
The remaining R6 native UI/header isolation and measured performance gates also remain
required. No imported PNG, Ant runtime or hardcoded arena is a prerequisite.
The foundation is 2D; elevation, ramps and slope physics listed below require an explicit
implemented fidelity contract and must not be claimed from flat tilemap support.

## Scope boundary

PipeFrame owns reusable physics, ray and shape queries, timing, rendering,
resources, input, telemetry, serialization, and editor extension mechanisms.
The robotics project owns robot-part definitions, simplified electrical
behavior, sensor models, drivetrain policy, controller APIs, autonomy examples,
and its editor presentation. No robot project code may expose SFML or require
brand-specific changes in PipeFrame or SimulationWorkbench.

Milestone 19 continuously dogfoods Milestone 18. Each feature starts with the
public editor and library workflow. If a button fails, a required feature is
missing, an API forces boilerplate, or the workflow/design is confusing, the
work pauses at that boundary and fixes the shared engine or editor. The fix adds
regression coverage and is checked against the active engine/Ant and independent
reuse fixtures before the
robotics feature continues. Internal PipeFrame and Workbench changes are
expected; robot-specific workarounds and duplicated framework code are not an
acceptable substitute.

## 19A — Robotics contracts and reference baselines

- Define supported units, axes, update rates, tolerances, and determinism rules.
- Define part, attachment, mechanical connection, power connection, signal
  connection, controller, and telemetry schemas.
- Document fidelity levels for physics, power, ultrasonic, LiDAR, infrared,
  servo, LED, and camera behavior so simplified models are explicit.
- Record validation scenarios and expected results for a differential-drive
  reference robot and at least one materially different custom assembly.

Acceptance: schemas round-trip through the Milestone 18 editor, reference
scenarios have measurable expected outcomes, and no contract assumes one kit.

## 19B — Part catalog and assembly workflow

- Add chassis, wheel, motor, motor driver, caster, battery, controller, servo,
  ultrasonic sensor, LiDAR, infrared line sensor, LED, and generic payload parts.
- Provide attachment geometry, compatibility constraints, mass, collision,
  configuration limits, icons, previews, and safe defaults.
- Support add, remove, position, rotate, attach, connect, duplicate, group,
  validate, save, and reuse through the standard editor workflows.
- Supply an ELEGOO-style reference assembly and a custom robot assembled from
  different compatible parts.

Acceptance: both robots can be assembled without source changes, invalid
connections explain how to fix them, and save/load and undo/redo preserve all
part and endpoint identities.

## 19C — Drivetrain and vehicle dynamics

- Model DC motor command, torque and speed response, gearing, wheel radius,
  traction, slip, rolling resistance, braking, body mass, and collision response
  at the documented fidelity level.
- Implement reusable differential-drive kinematics and odometry adapters over
  PipeFrame physics while retaining physical collision behavior.
- Support dynamic, kinematic, and idealized drive modes for learning, debugging,
  and comparison.

Acceptance: straight, turn-in-place, arc, stop, slope, collision, and odometry
fixtures remain within documented error bounds at supported time scales.

## 19D — Sensor simulation

- Implement ultrasonic emission, field-of-view sampling, obstruction queries,
  round-trip travel time, range limits, update rate, noise, dropout, and invalid
  return behavior.
- Implement configurable 2D LiDAR scanning with angle, resolution, range,
  rotation or update rate, noise, occlusion, point output, and viewport overlay.
- Implement infrared line and reflectance sensors and interfaces for servo pose,
  camera observations, bump sensors, and encoders.
- Use PipeFrame raycasts, spatial queries, deterministic random streams, clocks,
  and debug drawing instead of project-local replacements.

Acceptance: canonical wall, corner, narrow obstacle, moving object, reflective
line, occlusion, and seeded-noise fixtures match their expected readings.

## 19E — Power, signals, and actuators

- Model digital, PWM, analog, and typed data connections plus simplified battery
  voltage, capacity, current demand, brownout, and power switching.
- Route controller outputs to motors, servos, and LEDs and route sensor readings
  back through stable ports with timestamps and units.
- Detect incompatible pins, missing grounds or power, duplicate drivers,
  disconnected parts, invalid ranges, and update-order problems.

Acceptance: signal and power traces explain each device state; faults are
deterministic and visible; LEDs, servos, and motors respond to controller output.

## 19F — User control logic

- Provide a backend-neutral robot controller API for setup, fixed update, sensor
  reads, actuator writes, time, logging, and deterministic input.
- Support built-in C++ project controllers for obstacle avoidance, line
  following, manual drive, and waypoint navigation. Any later scripting option
  must use a bounded API and report compile or runtime errors in Workbench.
- Add play, pause, step, reset, speed, replay, breakpoint or watch, and controller
  state inspection through standard simulation controls.

Acceptance: users can replace control logic without changing the robot runtime,
and seeded runs replay with identical commands, readings, and poses.

## 19G — Environment and autonomy workflows

Consume the shared R6-E environment editor, assets, collision and query services.
This stage adds robotics scenarios and domain features; it does not defer the first
working generic tilemap/material editor until the robotics project.

- Add reusable rooms, walls, obstacles, ramps, floors, tracks, line materials,
  lights, spawn points, waypoints, goals, and moving hazards.
- Provide environment templates for obstacle avoidance, line following, mapping,
  parking, and waypoint navigation.
- Add optional occupancy-map and path visualization as project tools built on
  PipeFrame spatial and rendering services.

Acceptance: each workflow can be created, configured, run, reset, saved, and
replayed in the editor without hardcoded scene coordinates.

## 19H — Robotics workbench experience

- Add a searchable parts palette, assembly validation, connections view, device
  inspector sections, sensor previews, controller console, and telemetry charts
  through Milestone 18 extension points.
- Add overlays for attachment points, collisions, ultrasonic cones and returns,
  LiDAR beams and point clouds, wheel forces, odometry, paths, power flow, and
  signal values.
- Ensure panels remain reachable and non-overlapping across supported window
  sizes and DPI settings, with keyboard access and saved layouts.

Acceptance: a user can build and diagnose a robot from the editor while keeping
the simulation viewport readable; automated interaction and visual checks cover
all primary workflows.

## 19I — Validation and completion

- Validate the ELEGOO-style reference assembly against documented dimensions and
  observable behaviors where source material permits, while labeling deliberate
  simplifications.
- Validate a second custom robot containing a different sensor or actuator set
  to prove that the architecture is modular.
- Run deterministic, migration, save/load, undo/redo, plugin reload, performance,
  memory, resize and DPI, Debug, and Release suites.
- Publish tutorials for assembling a robot, adding a custom part, writing a
  controller, creating an environment, and debugging sensor data.

Acceptance: both reference robots satisfy their scenario gates; users can add a
new part and controller through documented extension APIs; no project-facing
contract exposes SFML; and the complete test and review matrix passes.

## Completion rule

Milestone 19 is complete only when modular assembly, physics, sensing, signals,
control, environments, editor workflows, persistence, deterministic replay, and
validation work together in running applications. A catalog item or isolated
sensor class does not complete a phase without its authored, simulated,
observable, saved, and tested workflow.
