# Source2Mod samples

Sample plugins for [Source2Mod](https://source2mod.net), one per language,
section for section the same:

- [`cs2/cpp_sample`](cs2/cpp_sample) -- a C++ plugin: lifecycle, ConVars,
  commands, game events, net messages, sounds, game hooks, entities, timers,
  transmit, natives for scripts, paths and permissions, traces, game systems,
  JSON/HTTP, SQL, menus and the custom HUD, cs_script.
- [`cs2/sp_sample`](cs2/sp_sample) -- the same plugin in SourcePawn, through
  the script API.

Copy one as the start of your own plugin; each section is a `SetupXxx()` and
its handlers, so it lifts out as it stands.

## Build

Both build against the [Source2Mod SDK](https://github.com/Source2Mod/source2mod-sdk)
-- the one variable they need is `SOURCE2MOD_SDK`:

```bash
export SOURCE2MOD_SDK=/path/to/source2mod-sdk

# C++ (CMake; or configure.py + ambuild)
cd cs2/cpp_sample
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build build

# SourcePawn: spcomp on the PATH, or SPCOMP=/path/to/spcomp (a Source2Mod
# build or release has it in addons/source2mod/scripting)
cd cs2/sp_sample
cmake -S . -B build -G Ninja && cmake --build build
```

The plugin lands in `build/addons/source2mod/plugins/<name>.smx`.

The samples are public domain.
