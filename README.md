# Xreader 
![build](https://github.com/linuxmint/xreader/actions/workflows/build.yml/badge.svg)

**Xreader** is a document viewer capable of displaying multiple and single page
document formats like PDF and Postscript. For more general information about
Xreader please visit our website at https://github.com/linuxmint/xreader.

This software is licensed under GNU GENERAL PUBLIC LICENSE Version 2 from June
1991 (see COPYING).

## Installation

- This package may be available through your system's package manager. This is the recommended method of installation.
- The latest unstable version is available for Linux Mint and LMDE in the [releases](https://github.com/linuxmint/xreader/releases) section.
- See [INSTALL](INSTALL.md) for instructions for building from source.

## Hacking

Our Coding Guidelines can be found under
https://linuxmint-developer-guide.readthedocs.io/en/latest/guidelines.html

## Performance Notes (Jan 2026)

**Baseline Metrics**:
- **Build time**: 4.8s (clean, Ninja)
- **Startup time**: 49-56ms (--help mode), 122ms (PDF open)
- **RAM idle**: ~50-80 MB (initial)
- **Largest module**: `shell/ev-window.c` (8093 lines, 52% of shell code)

**Known Hotspots**:
1. `shell/ev-window.c` - Monolithic UI controller, candidate for refactoring
2. `libview/ev-view.c` (7465 lines) - Critical rendering path
3. Memory allocations: 40+ `g_strdup` calls in `libdocument/`

**Optimization Opportunities** (see PERFORMANCE.md):
- Lazy-load backends (~30-50ms potential savings)
- Reduce allocations in hot paths (use `g_intern_string` for UI labels)
- Cache LRU for thumbnails (limit memory growth on large documents)
- Refactor `ev-window.c` into smaller modules

**Build Instructions**:
```bash
meson setup builddir --buildtype=debugoptimized
ninja -C builddir
```

**Measure Performance**:
```bash
G_MESSAGES_DEBUG=all builddir/shell/xreader document.pdf
```

## Testing & Accessibility

**Automated UI Tests**:
Xreader supports automated UI testing via AT-SPI. To run the test suite:
1. Ensure `python3-dogtail` is installed.
2. Build the project.
3. Run: `python3 test/testFileMenu.py ./build/shell/xreader`

**Accessibility (a11y)**:
Xreader integrates with the AT-SPI registry (via `atk-bridge`), allowing screen readers (like Orca) and automation tools to interact with the UI. If you encounter issues, ensure `at-spi2-core` is running and `GTK_MODULES` includes `atk-bridge` (though xreader initializes it explicitly).

- Poppler for PDF Backend [ http://poppler.freedesktop.org/ ]
- DjVuLibre for DjVu viewing [ http://djvulibre.djvuzone.org/ ]
- Rar for viewing CBR comics [ http://www.rarsoft.com/ ]
- libgxps for XPS documents [ https://wiki.gnome.org/libgxps ]
- GhostScript for Postscript Backend [ http://www.cs.wisc.edu/~ghost/ ]
- What about libspectre [ http://libspectre.freedesktop.org/wiki/ ]
- TIFF Backend
- DVI Backend
- Pixbuf Backend
- Comics Backend
- Impress Backend

-----
### Optional Programs
- Freeware unrar for viewing CBR comics [ http://www.rarsoft.com/ ]
- Gna! unrar (unrar-free) can be used too, though it can't decompress rar v3.0
  files [ http://gna.org/projects/unrar/ ]
- unzip for viewing CBZ comics [ http://www.info-zip.org ]
- p7zip for viewing CB7 comics [ http://p7zip.sourceforge.net/ ]
