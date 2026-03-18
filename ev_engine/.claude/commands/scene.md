Deep-dive into a specific scene. Provide the scene name as argument.

Usage: /scene space_suite

1. Find the scene's geometry builder in `src/scene.c` (e.g. `build_space_suite`)
2. Find the scene's load/update logic in `src/scene_*.c` (e.g. `src/scene_suite.c`)
3. Find the scene's lighting preset in `src/lighting.c` (e.g. `LightingPreset_SpaceSuite`)
4. Run QA for just this scene and show the hero + spawn screenshots
5. Report: wall count, material coverage, objects, lighting quality
6. Suggest specific improvements based on the master plan

If no argument given, list all scenes with their wall counts from the last QA run.
