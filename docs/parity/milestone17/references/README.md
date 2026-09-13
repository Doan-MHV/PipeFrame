# Milestone 17 video reference frames

This index is the permanent source for the four visual references used by the
Milestone 17 parity work. PNG captures should be stored beside this file with
the filenames below. Do not replace a frame without updating its timestamp and
the parity observations it supports.

Current PipeFrame comparison captures are stored as
`pipeframe-ant-1200-default.png` and
`pipeframe-sailboat-1200-training.png`. Their deterministic reproduction commands
are recorded in the [17A baseline](../README.md).

## `ant-simulator-06m20s.png`

- Source: [Ant Simulator 2 — Multiple Colonies at 06:20](https://www.youtube.com/watch?v=TfhrF7VrzWQ&t=380s)
- World remains full-screen beneath translucent UI.
- Compact simulation timer is centered at the top.
- Settings are independently accessible from the left edge.
- Colony and selected-ant panels are independently accessible on the right.
- Transport is centered at the bottom.
- Selected-ant panel includes preview, speed, blocked state, energy, distance,
  food, target visibility, highlighting, and follow controls.

## `sailboat-09m22s.png`

- Source: [SailBoat optimization at 09:22](https://www.youtube.com/watch?v=4ac1Kh30Ppo&t=562s)
- Water and course remain the dominant surface.
- Iteration/time panel remains visible on the left.
- Wind, training result, and selected-boat cards occupy separate compact areas.
- Editor and settings use independent left-edge handles.
- Transport is centered at the bottom.
- Network is independently accessible at the lower right and shows live node
  state, signed weighted connections, labels, and topology.

## `ui-design-09m41s.png`

- Source: [How I create my UIs — an example at 09:41](https://www.youtube.com/watch?v=Tdua8-9CKck&t=581s)
- Consistent outer spacing and parallel nested corner radii.
- Restrained shadows create depth without heavy borders.
- Semantic activity colors are balanced by muted common text.
- Active selection is immediately visible without changing the layout.
- Timeline, aggregate bar, and activity cards have a clear visual hierarchy.

## `animation-10m46s.png`

- Source: [How to animate anything at 10:46](https://www.youtube.com/watch?v=Lw8LPXPyrl0&t=646s)
- Card position, scale, ordering, and flip state transition continuously.
- The application describes target state and interpolation supplies motion.
- Easing and duration change presentation without changing domain behavior.
- Motion communicates spatial relationships rather than acting as decoration.

## Capture requirements

Capture the full YouTube video player after playback has reached the exact
timestamp. Crop away the YouTube page chrome while retaining the complete video
frame. Preserve the native player aspect ratio and save losslessly as PNG. The
captures are reference evidence and must not be used as application assets.
