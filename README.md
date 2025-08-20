# libizumi

libizumi is a multi-format instruction pipeline dump parser library. If you want to visualize traces, you may be interested in [Izumi-visualizer/izumi_tui](https://github.com/Izumi-visualizer/izumi_tui), the reference frontend.

> [!Note]
> The library is still under development and some format features may not work properly.

## Supported formats

| Format | Versions | Reference | Status |
|--|--|--|--|
| Onikiri2-Kanata | v4 | [kanata-log-format.md](https://github.com/shioyadan/Konata/blob/master/docs/kanata-log-format.md) | Everything but threads, lanes, tooltip labels, and dependencies works, full support is planned |
| Gem5-O3PipeView | ? | [o3-pipeview.py](https://github.com/gem5/gem5/blob/stable/util/o3-pipeview.py) | Planned |

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

## Documentation

After running `make`, you will find the HTML docs and man pages in the `docs/out` subdirectory of the build tree. `make install` will also install the man pages to your system. If no documentation is generated, ensure that you have `doxygen` installed and rerun the `configure` script before rebuilding.

---

![Izumi Konata](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fstatic.zerochan.net%2FIzumi.Konata.full.955876.jpg&f=1&nofb=1&ipt=11ebd39eb2229bec63db528410089c03d01dd9a541df01063ee5c809b6c69f58&ipo=images)
