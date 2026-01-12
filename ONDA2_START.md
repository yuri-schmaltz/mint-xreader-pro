# NEXT STEPS — ONDA 2 (Quick Reference)

## Status Atual
✅ ONDA 1 Completa (baseline, instrumentação, hotspots identificados)
⏳ ONDA 2 Pronta para começar (3-5 dias)

## Quick Start

### Build & Test Current
```bash
cd /home/yurix/Documentos/mint-xreader
ninja -C builddir                    # Rebuild (rápido)
builddir/shell/xreader --version     # Verify binary
```

### Verify Instrumentation
```bash
G_MESSAGES_DEBUG=all builddir/shell/xreader --help 2>&1 | grep BASELINE
# Expected: BASELINE: i18n_init took 0.01 ms
```

### Test with Document
```bash
builddir/shell/xreader /usr/share/doc/shared-mime-info/shared-mime-info-spec.pdf
```

## ONDA 2 Patches (In Order)

### PATCH 2: Lazy-load Backends
**Effort**: 3-5 days | **Expected**: -30-50ms startup

**Location**: `libdocument/ev-backends-manager.c`

**Strategy**:
1. Move `ev_backends_manager_scan_dir()` from init to first use
2. Defer loading backends until document opened
3. Cache result (only scan once)

**Files to Modify**:
- [ ] libdocument/ev-backends-manager.c
- [ ] libdocument/ev-backends-manager.h (if needed)
- [ ] shell/main.c (remove early scan if present)

**Test**:
```bash
# Before patch
time builddir/shell/xreader --help  # Should be ~50ms

# After patch
time builddir/shell/xreader --help  # Should still be ~50ms (no change in --help)
time builddir/shell/xreader document.pdf  # Should be noticeably faster
```

---

### PATCH 3: Reduce Memory Allocations
**Effort**: 2-3 days | **Expected**: -10-20% memory, +5-10% speed

**Location**: `libdocument/ev-document.c` (line 804-813)

**Strategy**:
1. Replace `g_strdup()` with `g_intern_string()` in `ev_document_info_copy()`
2. Reduce copies of immutable strings
3. Consider ref-counting for large structures

**Files to Modify**:
- [ ] libdocument/ev-document.c (main change)
- [ ] Other files with repeated allocations (search for g_strdup patterns)

**Test**:
```bash
# Measure with valgrind
timeout 10 valgrind --tool=massif builddir/shell/xreader document.pdf &
sleep 12
killall -9 xreader 2>/dev/null
ms_print massif.out.* > /tmp/massif_before.txt
# Compare peak memory
```

---

### PATCH 4: Thumbnail Cache LRU
**Effort**: 1 day | **Expected**: -30-50% RAM (large documents)

**Location**: `shell/ev-sidebar-thumbnails.c`

**Strategy**:
1. Add `#define MAX_THUMBNAIL_CACHE_SIZE 100`
2. Implement LRU eviction when cache exceeds limit
3. Add cache_size counter

**Files to Modify**:
- [ ] shell/ev-sidebar-thumbnails.c (main change)
- [ ] Maybe: shell/ev-sidebar-thumbnails.h (if new struct)

**Test**:
```bash
# Open large PDF (500+ pages)
builddir/shell/xreader large_document.pdf

# Scroll through thumbnails, observe memory
ps aux | grep xreader | grep -v grep

# Memory should stabilize around 150-200MB (instead of unlimited growth)
```

---

### PATCH 5: Fix Deprecated Poppler APIs
**Effort**: 1 day | **Expected**: Future-proofing (no perf gain)

**Location**: `backend/pdf/ev-poppler.cc` (4 occurrences)

**Strategy**:
1. Replace `poppler_page_get_selection_region()` with `poppler_page_get_selected_region()`
2. Verify no ABI changes needed

**Files to Modify**:
- [ ] backend/pdf/ev-poppler.cc (line 1985, 2022, 2998)

**Test**:
```bash
ninja -C builddir  # Should compile with no new warnings
# Test PDF selection still works
```

---

## Git Workflow

### Before Starting
```bash
cd /home/yurix/Documentos/mint-xreader
git status  # Review current changes
git diff shell/main.c  # View instrumentation changes
```

### For Each Patch
```bash
# Create feature branch (optional)
git checkout -b feature/patch-2-lazy-backends

# Make changes
# ...

# Test
ninja -C builddir

# Verify baseline metrics
G_MESSAGES_DEBUG=all builddir/shell/xreader document.pdf

# Compare before/after
# time xreader (before)
# time xreader (after)

# Commit
git add -A
git commit -m "PATCH 2: Lazy-load backends (est. -30-50ms startup)"
```

---

## Validation Checklist (Before Merge)

For each patch:
- [ ] Builds without errors
- [ ] No new warnings (unless expected)
- [ ] Smoke test: open PDF, EPUB, PS, TIFF
- [ ] Startup measurement before/after
- [ ] Memory baseline before/after
- [ ] No functional regressions
- [ ] Valgrind: no new leaks (if applicable)

---

## Documentation

### Update During Onda 2
- [ ] PERFORMANCE.md - Record actual improvements
- [ ] README.md - Update baseline metrics
- [ ] ONDA1_SUMMARY.md - Add notes for each patch

Example:
```markdown
#### PATCH 2: Lazy-load Backends ✅ COMPLETE
- Status: Merged
- Startup improvement: Measured -35ms (est. -30-50ms)
- Files modified: ev-backends-manager.c
- Regression: None found
```

---

## Commands Reference

```bash
# Navigate
cd /home/yurix/Documentos/mint-xreader

# Build
meson setup builddir --buildtype=debugoptimized  # One-time setup
ninja -C builddir                                 # Incremental rebuild

# Test
builddir/shell/xreader --help                   # Quick startup test
builddir/shell/xreader document.pdf             # Full app test

# Measure
time builddir/shell/xreader --help              # Startup time
G_MESSAGES_DEBUG=all builddir/shell/xreader --help 2>&1 | grep BASELINE
ps aux | grep xreader                           # RAM usage

# Profile (if available)
valgrind --tool=callgrind builddir/shell/xreader document.pdf
valgrind --tool=massif builddir/shell/xreader document.pdf

# Clean
rm -rf builddir                                 # Full rebuild
ninja -C builddir clean                         # Partial clean
```

---

## Useful Files for Reference

- **PERFORMANCE.md** - Full roadmap and analysis
- **ISSUES_FOUND.md** - Catalog of 30 issues
- **ONDA1_SUMMARY.md** - Onda 1 completion report
- **README.md** - Updated with Performance Notes
- **shell/main.c** - See BASELINE macros (lines ~29-43)

---

## Known Issues

### perf Tool
- Current system: `perf_event_paranoid=4` (restrictive)
- Solution: Use `valgrind --tool=callgrind` or manual timing instead

### Profile Output
- Valgrind slows down execution ~10-20x
- Use for memory/hotspot analysis, not absolute timing
- Timing metrics use `time` command instead

---

## Expected Outcomes (Onda 2)

After all 4 patches:
- **Startup**: -50-100ms (current ~120ms → ~20-70ms)
- **Memory**: -20-30% on typical usage
- **Maintenance**: No regressions
- **Code quality**: Better (fewer deprecations, more optimal)

---

## Next Wave (Onda 3)

After Onda 2 completes, planned:
- **PATCH 6**: Refactor ev-window.c (1-2 weeks, structural)
- **PATCH 7**: Add test suite (unit tests)
- **PATCH 8**: Async metadata loading (UI responsiveness)

---

**Last Updated**: Jan 12, 2026  
**Status**: Ready for implementation  
**Contact**: See CONTRIBUTING.md

---
