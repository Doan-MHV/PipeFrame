# Sprite and kinematic defaults — 2026-09-13

Debug: SpritePhysicsAcceptance and SceneProjectRuntimeAcceptance passed (2/2).

Release: all 80 enabled tests passed, including the new SpritePhysicsAcceptance
and extended BlankProjectAcceptance. The latter generated/built a fresh project,
created the built-in Sprite through editor session actions, assigned its texture,
attached body and box collider, and verified movement/collision across save/reopen
and build/reload. Tests use programmatically generated image/map fixtures.

The existing Ant-only suite stayed enabled; SailBoat was not included. No graphical
multi-colony performance benchmark or manual interactive screenshot session was run.

Read ../../SPRITE_AND_KINEMATIC_AUTHORING.md for setup and explicit physics limits.
