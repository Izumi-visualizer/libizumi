# libizumi

libizumi is a multi-format instruction pipeline dump parser library. If you want to visualize traces, you may be interested in [Izumi-visualizer/izumi_tui](https://github.com/Izumi-visualizer/izumi_tui), the reference frontend.

> [!Note]
> The library is still under development and some format features may not work properly.

## Supported formats

| Format | Versions | Reference | Status |
|--|--|--|--|
| Onikiri2-Kanata | v4 | [kanata-log-format.md](https://github.com/shioyadan/Konata/blob/master/docs/kanata-log-format.md) | Pipeline stages, lanes, instruction labels, cycles, retirement and flushes are supported; threads, tooltip labels and dependencies remain unsupported |
| Gem5-O3PipeView | ? | [o3-pipeview.py](https://github.com/gem5/gem5/blob/stable/util/o3-pipeview.py) | Planned |

The Kanata parser also accepts CVFPU traces. They use the same v4 commands but
differ from the Sargantana dumps in three ways: stage names can be longer than
three characters, the same stage name can appear concurrently on several lanes,
and instruction labels have no program counter. Stage ends are matched to the
most recent open stage on the same lane, so per-lane and iterative stages get
the correct duration; labels without a `0x...:` prefix keep a null PC.

`Stage.lane` preserves each numeric Kanata lane ID independently, and
`Stage.source` identifies which trace the stage came from (0 for the parsed
file; callers merging another file should use 1, 2, ...). A new `S` implicitly
closes the preceding open stage on that lane only. An `E` closes only a
matching open stage on the specified lane; it never closes a same-named stage
on another lane. Retirement and flush close all remaining open lanes,
preserving explicit stage-end cycles and zero-duration stages. The bundled
single-lane Sargantana examples retain identical parsed stages and durations.

`get_instruction()` returns an instruction by ID, creating any missing tables
and initializing the slot when needed. It is intended for tools that merge
several traces by instruction ID.

CVFPU logs indexed by core IDs can contain sparse IDs and explicit empty
instruction rows (`I` followed by `R` without any `S`). The parser preserves
these row indices, safely handles missing table ranges, and does not invent
pipeline stages for empty instructions.

## Library features

- Format and frontend agnostic, aiming to allow parsers and visualizers to be developed independently.
- Pure C implementation and dependency-free.
- Built with the autotools for easy packaging and integration.
- More coming soon:
  - More formats with complete support
  - Out-of-tree format plugins
  - Asyncronous file parsing
  - Transparent compression support
  - Hot reloading of trace files

## Requirements

- `make` (build only)
- `autotools` (build from source only, not needed in distribution packages)
- `doxygen` (optional, for generating documentation)

## Building (and installing)

If you cloned the repository instead of downloading a distribution package,
start by generating the build system:

```sh
autoreconf --install
```

With the `configure` script available, you can follow the traditional mantra:

```sh
mkdir build && cd build       # Any path supported!
../configure
make
(sudo) make install
```

Some intersting `configure` flags:
- `--disable-doxygen-doc` to avoid generating documentation
- `--enable-sanitizers` to enable ASAN (not recommended if you aren't actively developing Izumi!)
- `--help` to see more options!

### Nix derivation

A nix derivation and flake for the library are available. Using the flake system to pin the commit of the library is highly recommended.

## Running tests

From a configured build directory, run:

```sh
make check
```

The dependency-free C tests cover table allocation and reuse, sparse IDs,
exact PC/mnemonic searches in both directions, labels, independent lanes,
stage growth and timing, retirement/flushes, and parser error exits. Failure
details are written to `tests/test-suite.log`. Tests also run in Nix builds.

Every public library function is exercised, together with the internal helpers.
There are 25 test groups across these executables:

| Test executable | Functions covered |
| --- | --- |
| `test_data_structs` | All nine initialization, allocation, lookup, and cleanup functions in `data_structs.c` |
| `test_finder` | `find` and the internal `check_instruction` helper |
| `test_parser` | All eight public parser functions, plus `integer_length` and the private `existing_instruction` helper |

Public functions have direct API tests as well as integration coverage. The
private instruction lookup is exercised through parser commands, including
invalid-ID error paths. Numeric boundary tests include zero, decimal digit
transitions, cycle values beyond 32 bits, and maximum 64-bit values.

For memory and undefined-behavior checks, use a separate build directory:

```sh
mkdir build-sanitized
cd build-sanitized
../configure --disable-doxygen-doc --enable-sanitizers
make check
```

`make distcheck DISTCHECK_CONFIGURE_FLAGS=--disable-doxygen-doc` also checks
that the source distribution builds and runs its tests out of tree.

To measure coverage with GCC and its matching `gcov`, start from the repository
root and use a fresh build directory:

```sh
mkdir build-coverage
cd build-coverage
../configure --disable-doxygen-doc --disable-shared \
  CFLAGS='-O0 -g --coverage -fprofile-abs-path' LDFLAGS=--coverage
make check
gcov -f -b src/libizumi_la-data_structs.gcno \
  src/libizumi_la-finder.gcno src/libizumi_la-parser.gcno
```

Function/line coverage does not mean every branch or possible input has been
tested. Annotated coverage is saved in `*.c.gcov` in that directory.

### Fuzz testing

The optional fuzz runner requires Python 3 and a sanitizer-enabled build. It
uses a fresh parser process per input because the current parser API exits on
invalid input. From the repository root, after configuring `build-sanitized`:

```sh
make -C build-sanitized
make -C build-sanitized/tests fuzz_parser
python3 tests/fuzz_parser.py --driver build-sanitized/tests/fuzz_parser \
  --seconds 600 --seed 20261106 --output build-sanitized/fuzz-results
```

Use the libtool wrapper `tests/fuzz_parser`, not `tests/.libs/fuzz_parser`, to
ensure the just-built library is loaded. The runner mixes generated traces
checked against an independent Python state model with byte-level mutations
and fixed invalid-input seeds. It checks labels, PCs, stages, lanes, durations,
and retirement state, and stops on the first mismatch, sanitizer finding,
unexpected exit or per-input timeout (2 seconds by default).

Failures save the input, diagnostics and model output (when available), plus
`summary.json`. Replay a saved input with the driver; use the same seed to
reproduce a campaign's input sequence. Memory/undefined-behavior checks run on
all cases, while leak checking runs on valid model cases: rejected inputs exit
before caller-owned allocations can be freed. The runner caps individual ASAN
allocations at 64 MiB and treats allocation failure diagnostics as rejection.

The parser rejects signed/overflowing numeric fields, trailing junk in numeric
commands, cycle overflow, decreasing file cycles, and embedded NUL bytes.
Allocation failures report an error rather than dereferencing a null pointer.
Stage starts/ends and sidebar labels after retirement or flush are rejected;
duplicate retirement remains idempotent.

Local verification with GCC ASAN/LSAN/UBSAN, seed `20261106`: a 600-second
campaign completed 32,350 cases (20,434 accepted, 11,916 rejected) with no
remaining findings. An earlier campaign found a post-retirement stage reopening
bug; the reduced sequence is now included in the C regression tests.

Sidebar labels preserve leading whitespace and append repeated type-0 `L`
text. A leading `0x...:` on the first label is extracted as a padded PC;
other labels have no PC. Both LF and CRLF line endings are accepted.

## Documentation

After running `make`, you will find the HTML docs and man pages in the `docs/out` subdirectory of the build tree. `make install` will also install the man pages to your system. If no documentation is generated, ensure that you have `doxygen` installed and rerun the `configure` script before rebuilding.

---

![Izumi Konata](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fstatic.zerochan.net%2FIzumi.Konata.full.955876.jpg&f=1&nofb=1&ipt=11ebd39eb2229bec63db528410089c03d01dd9a541df01063ee5c809b6c69f58&ipo=images)
