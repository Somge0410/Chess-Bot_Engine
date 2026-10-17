# Benchmark diagnostics

Build with `ENABLE_QSEARCH_DIAGNOSTICS=ON` to include the existing search/TT
diagnostics and the pawn-table diagnostics. The Visual Studio configuration
`x64-Release-QDiag` already enables this option.

```powershell
cmake -S . -B out/build/Codex-QDiag -DENABLE_QSEARCH_DIAGNOSTICS=ON -DBUILD_TESTING=ON
cmake --build out/build/Codex-QDiag
```

The diagnostic executable supports these command-line modes:

```powershell
.\out\build\Codex-QDiag\Chess-Bot_Engine.exe bench
.\out\build\Codex-QDiag\Chess-Bot_Engine.exe bench_full
.\out\build\Codex-QDiag\Chess-Bot_Engine.exe bench_tt
.\out\build\Codex-QDiag\Chess-Bot_Engine.exe bench_game
```

The equivalent commands inside a UCI session allow shorter runs:

```text
go bench depth 6
go bench_full depth 6
go bench_tt depth 6
go bench_game movetime 50
```

Each position or game-ply row includes `pawnprobes`, `pawnhits`, `pawnhitrate`,
`pawnmisses`, `pawnstores`, `pawnreplacements`, `pawnoccupied`, `pawncapacity`,
and `pawnfill`. Percentages use a 0–100 scale. The final `PAWN TABLE` section
also separates inserts into empty slots from updates to an existing key.

- A hit requires a valid entry and a matching full pawn key.
- A replacement overwrites a valid entry for a different key. It does not increase
  the number of occupied entries.
- Every worker owns its cache. Event counters reset at the start of each search;
  cached entries remain available for later searches.
- `bench` and `bench_full` use fresh engines per position. Their summary occupancy
  and capacity are sums of the final participating-worker snapshots for those positions.
- `bench_tt` reuses one engine. Its summary contains event totals for all positions
  and a final occupancy snapshot.
- `bench_game` retains separate engines for White and Black. Its summary contains
  event totals for all plies and the final occupancy of both sides' participating-worker
  tables, counted once per side.
- Traced evaluation bypasses the cache and does not increment its probe/store counters.
- Counter updates are compiled out when diagnostics are disabled.

Run the focused cache, evaluation, and worker-aggregation tests with:

```powershell
ctest --test-dir out/build/Codex-QDiag -R pawn-table --output-on-failure
```

## Pawn-table sizing / OpenBench SPSA

`PAWN_TABLE_SIZE_LOG2` is a UCI integer tuning option. It controls the number of
entries in each worker's pawn cache: `2^PAWN_TABLE_SIZE_LOG2`. Defaults and bounds
are declared in `src/search/search_parameters.h`; use `uci` to check the values
in the executable you actually built. With 56-byte entries, exponent 2 uses
224 bytes and exponent 20 uses 56 MiB per worker, not per engine.

```text
setoption name PAWN_TABLE_SIZE_LOG2 value 12
```

Existing worker caches resize before their next search; standalone evaluation's
fallback cache resizes on its next use. A changed size discards the old entries
and releases the previous allocation; setting the same size preserves the cache.
The lookup path still uses a bit mask, with no resizing in the search hot path.

Paste the contents of `src/uci/openbench_pawn_table.spsa` into OpenBench:

```text
PAWN_TABLE_SIZE_LOG2, int, 16.0, 10.0, 20.0, 1.0, 0.002
```

The fields are name, type, start, minimum, maximum, final perturbation (`c_end`),
and final learning rate (`r_end`), following
[OpenBench's SPSA input format](https://github.com/AndyGrant/OpenBench/blob/master/OpenBench/spsa_utils.py).
The last two values are starting suggestions, not tuned settings.

Use the normal UCI executable (`ENABLE_SPSA_TUNING=OFF`) for OpenBench. That CMake
option selects the separate local evaluation tuner, not OpenBench UCI tuning.
Keep diagnostics disabled for playing-strength runs, and use clock-based time
controls: fixed-node or fixed-depth games do not measure this cache-size change's
speed benefit. Compare benchmark timings first; this discrete memory/speed tradeoff
can vary with CPU cache size and thread count. OpenBench's `Hash` option budgets
the TT, so allow additional RAM for the pawn caches, especially at the upper bound.

## Repeated pawn-table benchmarks from CMD

From the project folder, run:

```bat
bench_pawn_table.cmd
```

By default this uses `out\build\x64-Release\Chess-Bot_Engine.exe` and performs
`go bench_full depth 8` five times for every exponent from 2 through 20 (95 runs).
The CMD launcher uses the included Windows PowerShell helper for process control,
64-bit counters, averaging, and CSV output. No extra packages are required.

To choose another executable or a narrower range:

```bat
bench_pawn_table.cmd "C:\path\Chess-Bot_Engine.exe" -MinLog2 2 -MaxLog2 18
```

For a quick script check rather than a real performance comparison:

```bat
bench_pawn_table.cmd -MinLog2 2 -MaxLog2 3 -Runs 2 -Depth 2
```

Every timed run launches a fresh engine process, sends the exponent through UCI,
runs the synchronous full benchmark, sends `quit`, and waits for the process to
exit before starting another. It does not kill other engines. Ctrl+C or the
per-process timeout (default one hour, adjustable with `-TimeoutSeconds`) cleans
up the script's own active child. One extra untimed process checks UCI support
before the sweep begins. Unsupported ranges are rejected instead of silently
clamping, and exponents above 20 are deliberately disallowed by the script.

Odd rounds test ascending exponents; even rounds test descending exponents to
reduce order bias. The script requires identical per-position moves, scores, and
node counts throughout. Use a rebuilt, diagnostics-disabled Release executable;
the helper warns if it sees diagnostic output. The benchmark's existing settings
remain unchanged: isolated engines, one search worker, and the benchmark's fixed
TT size (currently 128 MiB). Its reported time includes lazy pawn-table allocation
but excludes engine construction and TT allocation. Process restart/exit time is
recorded separately and is not included in the benchmark NPS average.

Results are printed and saved in a new folder under `out\bench-pawn-table`:

- `summary.csv`: average benchmark time, arithmetic average NPS, pooled NPS
  (`sum(nodes) / sum(time)`), minimum/maximum NPS, and sample standard deviation.
- `runs.csv`: each run's measurements and process ID, updated after every run.
- Per-run raw stdout/stderr logs and `settings.json` with the executable's hash.

CSV numeric formatting follows the Windows locale, recorded as `CsvCulture` in
`settings.json` (for example, a German locale uses decimal commas).

Keep other CPU-heavy work stopped during the sweep. A long full sweep can take
considerable time; the shorter example above only verifies the script.

## OpenBench builds on small hosts

The OpenBench `makefile` defaults to one build job and builds only the
`Chess-Bot_Engine` target. It explicitly disables regression-test targets,
diagnostics, and the separate local SPSA-tuner entry point. Release optimization
and LTO remain enabled; evaluation and search settings are unchanged.

Use the ordinary OpenBench build command on a Raspberry Pi:

```sh
make EXE=MyEngine
```

Larger machines can explicitly request additional build jobs:

```sh
make BUILD_JOBS=2 EXE=MyEngine
```

OpenBench's `CC`, `CXX`, and `EXE` arguments are still supported. An outer
`make -j` does not remove the explicit inner CMake job limit. These settings apply
to future builds after deploying the updated source; they do not change a running
build, cap total RAM use, or control OpenBench's separate Fastchess build process.
