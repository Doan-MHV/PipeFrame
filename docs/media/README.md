# Portfolio media

These are real frames from PipeFrame and Ant, captured on macOS from a Release build.
The editor GIF shows Ant running and the shared physics overlay toggling on/off.
The Ant GIF shows the world render without editor panels. Both cover six seconds
of simulation at 15 captured frames per second; the GIFs repeat at the end.

To recapture from the repository root (requires the test targets):

```sh
cmake --build --preset release --target WorkbenchUITests
./cmake-build-release/apps/SimulationWorkbench/WorkbenchUITests \
  --portfolio-capture /tmp/pipeframe-portfolio
```

The command runs native UI acceptance using temporary test projects. It writes
90 frames each to `editor-frames/1000.png` through `1089.png` and `ant-frames/`.
It does not drive or save the user's open editor session.

Encode with FFmpeg:

```sh
ffmpeg -y -framerate 15 -start_number 1000 \
  -i /tmp/pipeframe-portfolio/editor-frames/%04d.png \
  -filter_complex '[0:v]scale=960:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse=dither=bayer:bayer_scale=3' \
  -loop 0 docs/media/pipeframe-editor.gif

ffmpeg -y -framerate 15 -start_number 1000 \
  -i /tmp/pipeframe-portfolio/ant-frames/%04d.png \
  -filter_complex '[0:v]scale=800:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse=dither=bayer:bayer_scale=3' \
  -loop 0 docs/media/ant-simulation.gif
```

The PNG screenshots use frame 1025 from the corresponding sequence.
Ant's reference-source attribution is in its project README.

Additional gallery images come from the same native acceptance capture:

| Portfolio file | Capture file |
| --- | --- |
| pipeframe-inspector.png | signal-beacon-inspector.png |
| pipeframe-source-generation.png | source-generation.png |
| pipeframe-assets.png | editor-1440-assets.png |
| ant-playground.png | ant-authored-playground.png |
| ant-mesh.png | ant-debug-mesh.png |

These are unedited screenshots. The asset browser image uses an editor test scene;
the other gallery images show the Ant project.
