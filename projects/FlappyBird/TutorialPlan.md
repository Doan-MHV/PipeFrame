# PipeFrame Flappy Bird Tutorial Plan

This project should be the small, friendly tutorial that proves PipeFrame can build a normal 2D game, not only simulations. The goal is to teach the engine through one simple game loop:

1. Press a key to make the bird jump.
2. Gravity pulls the bird down.
3. Pipes spawn and move left.
4. Pipes are destroyed after leaving the screen.
5. Passing pipes increases score.
6. Hitting a pipe or ground ends the game.

## Current Project Status

Ready:

- Flappy project exists at `projects/FlappyBird`.
- C++ project module builds.
- Asset manifest contains bird, pipe, sky, ground, cloud, panel, and tile assets.
- Prefabs exist for `bird`, `pipe`, `sky_background`, and `ground_strip`.
- Engine now supports `LayerComponent` for parallax/repeating backgrounds.
- Engine now supports `DestroyWhenOffscreenComponent`.
- Engine now has `SpawnPrefab(...)` for C++ systems.
- Engine now has `CreateSpriteEntity(...)` for quick sprite entity creation.
- Engine now has camera helpers such as `WorldToScreen(...)` and `ScreenToWorld(...)`.

Still starter-only:

- Source files are still generated examples.
- The level currently has no authored Flappy entities.
- No Flappy-specific gameplay systems exist yet.

## Tutorial File Structure

The tutorial should replace generated example files with this shape:

```text
projects/FlappyBird/
  Source/
    Components/
      BirdComponent.h
      PipeComponent.h
      PipeSpawnerComponent.h
      ScoreComponent.h
      GameStateComponent.h

    Entity/
      Bird.h
      PipeSpawner.h
      ScoreHud.h
      Background.h

    Events/
      BirdJumpEvent.h
      BirdPassedPipeEvent.h
      GameOverEvent.h

    Systems/
      BirdInputSystem.h
      BirdMovementSystem.h
      PipeSpawnerSystem.h
      PipeMovementSystem.h
      FlappyCollisionSystem.h
      ScoreSystem.h
      GameStateSystem.h
```

## Components

`BirdComponent`

- `jumpVelocity`
- `gravity`
- `maxFallSpeed`
- `alive`

Purpose: Stores bird tuning values exposed in the inspector.

`PipeComponent`

- `passedBird`

Purpose: Marks pipe entities and prevents scoring twice.

`PipeSpawnerComponent`

- `pipePrefabId`
- `spawnInterval`
- `pipeSpeed`
- `gapSize`
- `minGapY`
- `maxGapY`
- `spawnX`
- `topPipeY`
- `bottomPipeY`
- runtime `timeUntilNextSpawn`

Purpose: Lets the tutorial spawn pipe pairs from one editor-authored spawner.

`ScoreComponent`

- `score`
- `bestScore`

Purpose: Stores game score.

`GameStateComponent`

- `isRunning`
- `isGameOver`

Purpose: Keeps game state in one visible editor component instead of hidden global code.

## Entity Classes

`Bird`

- Adds `TransformComponent`
- Adds `SpriteComponent`
- Adds `RigidBodyComponent`
- Adds `BoxColliderComponent`
- Adds `BirdComponent`
- Uses `flappy-bird-texture`

`PipeSpawner`

- Adds `TransformComponent`
- Adds `PipeSpawnerComponent`

`ScoreHud`

- Adds `TextLabelComponent`
- Adds `ScoreComponent`

`Background`

- Creates sky and ground entities using `LayerComponent`
- Sky uses repeat and parallax.
- Ground uses repeat X and higher layer order.

## Systems

`BirdInputSystem`

- Listens for jump input.
- Emits `BirdJumpEvent`.
- Does not directly move the bird.

`BirdMovementSystem`

- Requires `BirdComponent`, `RigidBodyComponent`, and `TransformComponent`.
- Applies gravity every frame.
- Applies jump velocity when `BirdJumpEvent` happens.
- Clamps fall speed.
- Rotates bird slightly based on vertical velocity.

`PipeSpawnerSystem`

- Requires `PipeSpawnerComponent` and `TransformComponent`.
- Uses `SpawnPrefab(context, pipePrefabId, position)`.
- Spawns top and bottom pipe entities.
- Adds `DestroyWhenOffscreenComponent` if prefab does not already have it.

`PipeMovementSystem`

- Requires `PipeComponent` and `RigidBodyComponent`.
- Sets or maintains pipe velocity.

`FlappyCollisionSystem`

- Listens to `CollisionEvent`.
- If bird hits pipe or ground, emits `GameOverEvent`.

`ScoreSystem`

- Checks when bird passes a pipe pair.
- Emits `BirdPassedPipeEvent`.
- Updates `ScoreComponent`.

`GameStateSystem`

- Handles start, reset, game over, and pause behavior.
- Later can own a restart button or key.

## Tutorial Steps

1. Create the Flappy project.
2. Open the asset manifest and explain texture IDs.
3. Create the bird entity class.
4. Add `BirdComponent`.
5. Add `BirdInputSystem`.
6. Add `BirdMovementSystem`.
7. Create the pipe prefab.
8. Add `PipeSpawnerComponent`.
9. Add `PipeSpawnerSystem`.
10. Add `DestroyWhenOffscreenComponent` to pipe prefabs.
11. Add collision and game over.
12. Add score.
13. Add background layers using `LayerComponent`.
14. Tune values in the editor.
15. Save entities and run the game.

## Engine Features Already Good Enough

- ECS components and systems.
- Project-defined components.
- Project-defined entity classes.
- Project-defined events.
- Generic event bus.
- Prefab registry.
- C++ prefab spawning.
- Sprite rendering.
- RigidBody movement.
- BoxCollider collision.
- Text rendering.
- Camera helpers.
- Layer/parallax rendering.
- Offscreen destroy.

## Engine Features Still Missing Or Weak

These should be fixed before recording the tutorial:

1. Input should expose a simple generic key event API.

   Current engine has input systems, but tutorial code should not feel tied to one old movement use case. Flappy needs a clean `KeyPressedEvent` or `InputState` helper that user systems can read easily.

2. The editor should make prefab spawning clearer.

   Users should be able to see prefab IDs in the content browser and copy/select them for component fields such as `pipePrefabId`.

3. Text/font asset workflow needs a friendly default.

   Score needs text. If the generated project has no default font asset, the tutorial becomes annoying too early.

4. Sprite animation needs a beginner-friendly example.

   Bird wing animation should use existing `AnimationComponent`, but the tutorial needs a clean explanation of sprite sheet frame size and frame speed.

5. Collision debugging should be easy to toggle.

   Flappy is a perfect place to teach box colliders, so the editor should let users toggle collider drawing without digging.

6. Level authoring should support an empty/no-tilemap visual mode.

   Flappy does not really need a tilemap background. It needs sky sprites and ground sprites. The tilemap can be used for collision or removed from the visual layer.

7. Save/load should preserve all new project component values.

   This is critical for tutorial trust. If users tune gravity or pipe gap and reopening loses it, the tutorial feels broken.

## Recommended First Implementation Sprint

Build only the smallest working Flappy loop first:

1. Replace example source files with Bird, PipeSpawner, and Score components.
2. Register Bird, PipeSpawner, Background, and ScoreHud entity classes.
3. Add bird input and movement.
4. Add pipe spawning from prefab.
5. Add offscreen destruction.
6. Add collision game over.

Do not add menus, polish, audio, particles, or advanced animation until the core loop feels good.

