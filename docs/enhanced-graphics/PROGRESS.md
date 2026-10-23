# Enhanced graphics progress

## Iteration 1 — local checks passed, deployment pending (2026-10-23)

The user approved comparing Enhanced speed with the measured Classic baseline
instead of requiring 59–61 retrace calls per second. The user also approved
using the deterministic opening poster for exact Classic identity, with visual
review of the non-deterministic menu. Fullscreen verification is deferred until
the first Pages deployment at the user's request.

### Implemented locally

- Confirmed a clean `enhanced-graphics` branch with the user's fork as `origin`.
- Installed CMake 3.31.10 and Emscripten 6.0.13 outside the repository.
- Added frame dumps, a retrace counter, clean exit after a frame number,
  Classic-path timings, and dev-only scenario controls. The controls include
  skipping the opening poster, starting a demo or game, opening a pause menu,
  and capturing consecutive split-screen frames. All are off by default.
- Added `web/index.html`, `web/style.css`, and a new Pages workflow. GitHub
  Actions is enabled; Pages uses workflow builds; `github-pages` permits
  deployments from `enhanced-graphics`.

### Verification

| Check | Result | Evidence |
| --- | --- | --- |
| Desktop Release build | PASS | `train_fork_artifacts/build-iter1-final.log`; no new project warnings; existing SDL2 duplicate-library warning remains |
| Web build | PASS | `train_fork_artifacts/web-build-iter1-final.log`; JS and WASM produced |
| Debug build and S1 | PASS | `train_fork_artifacts/debug-build-iter1-final.log` and `debug-S1-iter1-final.log`; no assertion |
| S1 poster at frames 60/120 | PASS | Two baseline runs and the final Release run have identical BMP SHA-1 `9427bbc097bc51127bbc724e345e6c8e39b34dcb` for both frames |
| S1 main menu at frames 60/120 | NONDETERMINISTIC | `--skip-intro Player S7` reaches the menu, but generated scenery differs between runs; all four BMP hashes differ |
| S2 reaches frame 2500 | PASS | Two complete runs, both exited after frame 2500 |
| S2 determinism | NONDETERMINISTIC | All five corresponding BMP hashes differ despite the same `Player S7` seed |
| Classic game-speed baseline | RECORDED | S1 static: ~56.7 retraces/s. S2a with dumps, active frames 150–2100: mean 33.3, median 33.5 retraces/s. The latest S2 run without dumps: mean 41.81, median 43.52 retraces/s across 46 one-second samples. Scenario workload varies. |
| S3 dialog over field | PASS | `baseline/S3b/frame-540.bmp` shows the pause dialog over the game field; dialog palette is unchanged |
| S4 transition | PASS | `baseline/S4split-game/frame-406.bmp` through `frame-410.bmp` show successive split-screen steps |
| No enhanced writes to `VGAState` | PASS | No enhanced module exists yet; diagnostics use SDL renderer output and timing only |
| File scope | PASS | Only SDL driver, `src/main.cpp`, one main-menu dev hook, `web/`, Pages workflow, and docs changed |
| Local web page | PARTIAL | Play starts the game in the Codex in-app browser; right click leaves no context menu; canvas accepts keyboard focus. The browser does not expose the Fullscreen API, so the button's effect cannot be verified there. Safari UI access is unavailable in this environment. The user approved checking Fullscreen after deployment. |
| Pages deployment | NOT RUN | The protocol allows commit/push only after the gate passes. |

In the latest S2 run, `updatePicture()` cost across the active demo averaged
0.17 ms for typical updates and 0.28 ms for full updates; the largest observed
full update was 3.23 ms. The earlier dumped run's largest per-update time was
6.45 ms. No new GPU textures exist in iteration 1. The latest measurements
are in `train_fork_artifacts/iter1-S2-metrics.log`.

Artifacts are under `/Users/laverka/workspace/train_fork_artifacts/`, outside
the repository. Visual review: the static poster and menu retain their Classic
colors; S2 frame 600 shows moving trains and semaphores; S3 shows the intact
pause dialog; S4 shows the poster sliding beneath the header.

### Remaining deployment check

After the first push, verify the Pages workflow, public files, and Play. Test
Fullscreen on the deployed page in a browser that supports the Fullscreen API.
Do not start iteration 2 until that check passes.
