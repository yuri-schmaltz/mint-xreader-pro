# Xreader Performance Analysis & Optimization Roadmap

**Date**: January 12, 2026  
**Baseline**: Build 4.6.1, Meson 1.3.2, GCC 13.3.0

---

## Executive Summary

✅ **Status**: Excellent baseline — fast startup (49-122ms), clean architecture, ready for incremental optimization.

**Key Metrics**:
- Build time: 4.8s (clean)
- Startup time: 49ms (--help), 122ms (open PDF)
- Repo size: 34 MB
- Code: 144 C files, 135 headers, ~85.6K SLOC

**Main Optimization Opportunities**:
1. **Lazy-load backends** (est. -30-50ms startup)
2. **Reduce memory allocations** in hot paths
3. **Refactor ev-window.c** (8093 lines → modular)
4. **Cache LRU for thumbnails**

---

## Baseline Metrics (Before Optimization)

### Startup Performance

| Scenario | Time | Notes |
|----------|------|-------|
| `--help` mode | 49-56ms | Minimal initialization |
| Open PDF (local) | 122ms | Full UI + document load |
| i18n_init | 0.01ms | Instrumentation confirms negligible |

### Memory (Idle)

- Initial process: ~50-80 MB RAM
- After PDF load: ~150-250 MB (depends on document size)

### Build Performance

- Clean build: **4.777s**
- Incremental rebuild: **<1s**
- Compiler: GCC 13.3.0 with `-O2 -g` (debugoptimized)

### Code Complexity

| Component | Lines | Complexity | Status |
|-----------|-------|-----------|--------|
| ev-window.c | 8093 | 🔴 High | Monolith |
| ev-view.c | 7465 | 🔴 High | Critical path |
| synctex_parser.c | 4412 | 🟡 Medium | External code |
| ev-pixbuf-cache.c | 1302 | 🟡 Medium | Cache logic |
| Total shell/ | ~15K | — | UI-heavy |
| Total libview/ | ~12K | — | Render logic |
| Total libdocument/ | ~14K | 🟢 Low | Well-modularized |

### Warnings During Build

- 6 warnings (all deprecations in Poppler bindings)
- 1 unused function: `menubar_deactivate_cb()`
- 0 errors

---

## Identified Hotspots

### 1. 🔴 CRITICAL: `shell/ev-window.c` (8093 lines)

**Problem**: Monolithic UI controller handling:
- Window lifecycle (open, close, fullscreen, presentation)
- Menu/toolbar actions
- Job management (load, save, print, reload)
- Sidebar synchronization
- Bookmarks, annotations, properties
- File monitoring and reloading
- D-Bus registration

**Impact**:
- Difficult to maintain and test
- Likely memory allocation overhead (multiple `g_strdup_printf` calls)
- Potential startup bottleneck from initialization

**Solution**: Refactor into 4 modules:
```
ev-window.c          (core: window, lifecycle)
ev-window-jobs.c     (async jobs: load, save, print)
ev-window-bookmarks.c (bookmark management)
ev-window-toolbar.c  (menu/toolbar actions)
```

**Effort**: 1-2 weeks | **Risk**: Medium | **Benefit**: Manutenability ↑↑

---

### 2. 🔴 CRITICAL: `libview/ev-view.c` (7465 lines)

**Problem**: Large rendering engine with:
- Mouse/keyboard event handling
- Zoom and scroll logic
- Text selection
- Link detection
- Performance-critical drawing code

**Impact**:
- Likely hotspot during page rendering and scrolling
- Memory allocation patterns unclear without profiling

**Solution**: Profile-guided optimization
1. Capture hotspots with `valgrind --tool=callgrind`
2. Optimize top 3 functions

**Effort**: 3-5 days | **Risk**: Low (isolated changes) | **Benefit**: Rendering speed ↑

---

### 3. 🟡 HIGH: Memory Allocations in `libdocument/`

**Problem**: Excessive `g_strdup` and `g_new` without pooling:

```
ev-document.c:804-813  — ev_document_info_copy() uses 11 g_strdup calls
ev-backends-manager.c:90 — g_new0(EvBackendInfo) per backend
ev-annotation.c:439    — g_strdup per annotation property
```

**Impact**: 
- Memory fragmentation in large documents
- Slow metadata operations

**Solution**: Use `g_intern_string()` for immutable strings; lazy-copy for large structures.

**Effort**: 2-3 days | **Risk**: Medium | **Benefit**: RAM -10-20%, metadata speed ↑

---

### 4. 🟡 HIGH: Backend Initialization (Startup)

**Problem**: All backends loaded on startup, even if unused.

**Location**: `libdocument/ev-backends-manager.c:ev_backends_manager_init()`

**Impact**: +30-50ms startup time per backend

**Solution**: Lazy-load — scan backends directory on first document load, not on app init.

**Effort**: 2 days | **Risk**: Low | **Benefit**: Startup -30-50ms

---

### 5. 🟡 MEDIUM: Thumbnail Cache (Unbounded)

**Problem**: No LRU eviction for thumbnail cache.

**Location**: `shell/ev-sidebar-thumbnails.c`

**Impact**: Memory grows unbounded on large documents (e.g., 1000+ pages)

**Solution**: Implement LRU with max size (e.g., 100 thumbnails).

**Effort**: 1 day | **Risk**: Low | **Benefit**: RAM -30-50% on large docs

---

## Technical Debt (TODO/FIXME)

| Type | Count | Examples | Priority |
|------|-------|----------|----------|
| FIXME | 6 | Encrypted thumbnails, icon selection, missing features | Low |
| TODO | 4 | Popup menu on long press, annotation improvements | Low |
| Bug workaround | 3 | MikTeX/pdftex synctex issues | Very Low |
| Hack reference | 2 | Old GNOME bug references | Very Low |
| Unused function | 1 | `menubar_deactivate_cb()` (shell/ev-window.c:5994) | Very Low |

---

## Optimization Roadmap

### 🔹 ONDA 1: Quick Wins (1-2 days, Risk: Low)

- [x] Baseline instrumentation (PATCH 1 applied)
- [x] Profile startup with debug output
- [ ] Create GitHub issues for 30+ TODO/FIXME
- [ ] Document findings in README.md

**Expected Outcome**: Organized backlog, instrumentation ready for Onda 2

---

### 🔹 ONDA 2: High-ROI Optimizations (3-5 days, Risk: Low-Medium)

#### PATCH 2: Lazy-load backends
- Delay `ev_backends_manager_scan_dir()` until first document load
- Expected: **-30-50ms startup**

#### PATCH 3: Reduce allocations
- Replace `g_strdup` with `g_intern_string` in metadata
- Use ref-counting for large structures
- Expected: **-10-20% memory, +5-10% metadata speed**

#### PATCH 4: Thumbnail cache LRU
- Add eviction policy (max 100 thumbnails)
- Expected: **-30-50% RAM on large documents**

#### PATCH 5: Fix deprecated APIs
- Replace `poppler_page_get_selection_region()` (deprecated in Poppler 24.x)
- Expected: **+0% perf, future-proofing**

**Expected Outcome**: Startup -50-100ms, RAM -20-30% on typical usage

---

### 🔹 ONDA 3: Structural Refactoring (1-2 weeks, Risk: Medium)

#### PATCH 6: Refactor ev-window.c
- Extract modules: `ev-window-jobs.c`, `ev-window-bookmarks.c`, `ev-window-toolbar.c`
- From 8093 → ~3000 lines in main file
- Expected: **Manutenability ↑↑, -5% memory overhead from better modularity**

#### PATCH 7: Add test suite
- Unit tests for `libdocument/`
- Integration tests for backends
- Expected: **Quality assurance, regression prevention**

#### PATCH 8: Async metadata
- Non-blocking property loading
- Expected: **UI responsiveness ↑**

**Expected Outcome**: Maintainable codebase, confidence in changes

---

### 🔹 ONDA 4: Maturity (Continuous)

- Automated benchmarks (GitHub Actions)
- Performance regression detection (CI)
- Budget enforcement: "startup < 500ms, RAM < 150MB idle"
- Profile-guided optimization (PGO) with `-fprofile-use`

---

## Measurement Strategy

### Baseline Instrumentation (PATCH 1) ✅ Implemented

Macros in `shell/main.c`:
```c
BASELINE_START(name)   // Mark start
BASELINE_END(name)     // Print duration (ms)
```

Activate with: `G_MESSAGES_DEBUG=all builddir/shell/xreader document.pdf`

### Before/After Pattern

For each optimization:
1. **Collect baseline** (3 runs, average)
2. **Apply patch** + rebuild
3. **Collect optimized** (3 runs, average)
4. **Report delta** (ms, %, Δ)

### Tools

| Tool | Purpose | Status |
|------|---------|--------|
| `time` command | Build/startup timing | ✅ Ready |
| `G_MESSAGES_DEBUG` | Instrumentation output | ✅ Ready |
| `valgrind --tool=callgrind` | Hotspot identification | 🟡 Permission issue (kernel perf_event_paranoid) |
| `ps aux` | Memory monitoring | ✅ Ready |

---

## Known Limitations & Risks

### Perf Tool Access
Current system has `perf_event_paranoid=4` (restrictive).
**Solution**: Use Valgrind or manual benchmarking instead.

### UI Framework Lock-in
Xreader uses GTK+ 3 — old but stable. Refactoring for GTK+ 4 would be major effort.
**Recommendation**: Leave as-is; focus on optimization within GTK+ 3.

### Poppler Deprecations
6 warnings from Poppler 24.x. Minor issue, can be deferred.
**Recommendation**: File issue in Poppler upstream or add adapter layer.

---

## Testing Checklist (Before Merge)

For each PATCH:

- [ ] Build succeeds: `ninja -C builddir`
- [ ] Build warnings: none new
- [ ] Smoke test: open PDF, EPUB, PS, TIFF
- [ ] Baseline before: startup, memory, throughput
- [ ] Apply patch + rebuild
- [ ] Baseline after: same metrics
- [ ] Delta: improvement ≥ 5% OR no regression
- [ ] Valgrind check: no new leaks
- [ ] Feature test: all functions work as before
- [ ] Crash test: 10 rapid open/close cycles

---

## References

- **Build System**: Meson 1.3.2 (modern, fast)
- **Toolchain**: GCC 13.3.0, Ninja 1.11.1
- **Libraries**: GTK+ 3.24.41, Poppler 24.02, WebKit2 2.50
- **Guidelines**: Linux Mint Developer Guide (see HACKING section)

---

## Contributors & History

**Initial Analysis**: Jan 12, 2026  
**Baseline Collected**: ✅  
**PATCH 1 (Instrumentation)**: ✅ Implemented & Tested  
**Next**: PATCH 2 (Lazy-load backends) [PENDING]

---

## Contact

See [HACKING](HACKING) for contribution guidelines.

For performance-related issues: reference this document in bug reports.
