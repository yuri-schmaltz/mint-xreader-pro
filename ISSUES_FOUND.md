# Xreader Issues from Code Review (Jan 2026)

## Issue Template (Use for GitHub)

```markdown
Title: [CATEGORY] Description from TODO/FIXME

Labels: technical-debt, enhancement, low-priority
Assignee: (open)
Priority: Low/Medium/High

**File**: path/to/file.c:line
**Type**: TODO / FIXME / HACK

**Description**:
[Extracted from code comment]

**Suggested Fix**:
[Brief recommendation]
```

---

## Issues by Priority

### HIGH PRIORITY (Functional Impact)

#### Issue #1: Missing get_selected_items() in Poppler
- **File**: libview/ev-view.c:2455
- **Type**: FIXME
- **Current Code**: `/* FIXME: we need a get_selected_items function in poppler */`
- **Problem**: Cannot properly handle selected form fields across pages
- **Impact**: Form field selection limited
- **Status**: Blocked on Poppler upstream

---

#### Issue #2: Pixbuf Cache Not Extended
- **File**: libview/ev-view.c:6263
- **Type**: FIXME
- **Current Code**: `/* FIXME: if we're keeping the pixbuf cache around, we should extend the...`
- **Problem**: Pixbuf cache lifecycle management incomplete
- **Impact**: Possible memory leak or cache misses on large documents
- **Recommendation**: Review `ev-pixbuf-cache.c` for LRU eviction policy

---

#### Issue #3: Form Field Navigation Incomplete
- **File**: libview/ev-view.c:5507
- **Type**: FIXME
- **Current Code**: `/* FIXME: this doesn't work if the next/previous page doesn't have form fields */`
- **Problem**: Next/previous page navigation fails when target page lacks form fields
- **Impact**: Tab key navigation broken on some documents
- **Recommendation**: Implement skip-to-next-field-with-content logic

---

### MEDIUM PRIORITY (Code Quality)

#### Issue #4: Annotation Icon Selection
- **File**: libview/ev-annotation-window.c:275
- **Type**: FIXME
- **Current Code**: `icon = gtk_image_new (); /* FIXME: use the annot icon */`
- **Problem**: Annotation window always shows default icon, not annotation-specific icon
- **Impact**: UI feedback unclear for users
- **Recommendation**: Map annotation types to GTK+ icons or custom assets

---

#### Issue #5: Form Field Button Update Logic
- **File**: libview/ev-view.c:2162
- **Type**: FIXME
- **Current Code**: `/* FIXME: only buttons in the same group should be updated */`
- **Problem**: All buttons updated instead of just those in same group
- **Impact**: Unexpected radio button behavior
- **Recommendation**: Implement button group tracking

---

#### Issue #6: Form Field Toggle Behavior
- **File**: libview/ev-view.c:2143
- **Type**: FIXME
- **Current Code**: `/* FIXME: it actually depends on NoToggleToOff flags */`
- **Problem**: Toggle off behavior not respecting PDF form flags
- **Impact**: Some PDFs with specific flags behave incorrectly
- **Recommendation**: Parse NoToggleToOff flag and respect it

---

#### Issue #7: Annotation Rendering Optimization
- **File**: libview/ev-view.c:3220
- **Type**: FIXME
- **Current Code**: `/* FIXME: only redraw the annot area */`
- **Problem**: Full page redrawn for annotation changes, not just annotation region
- **Impact**: Slower annotation editing on large pages
- **Recommendation**: Implement region-based invalidation for annotations

---

#### Issue #8: Annotation Update Region
- **File**: libview/ev-view.c:4404
- **Type**: FIXME
- **Current Code**: `/* FIXME: reload only annotation area */`
- **Problem**: Full page reload on annotation updates
- **Impact**: Visible flicker when editing annotations
- **Recommendation**: Partial reload or region-based rendering

---

### LOW PRIORITY (Code Cleanliness)

#### Issue #9: File Helpers Error Codes
- **File**: libdocument/ev-file-helpers.c:550
- **Type**: FIXME
- **Current Code**: `/* FIXME: better error codes! */`
- **Problem**: Generic error codes don't distinguish failure causes
- **Impact**: Difficult to diagnose file operation failures
- **Recommendation**: Define specific error enums for mkstemp, open, etc.

---

#### Issue #10: Localization of Error Messages
- **File**: libdocument/ev-file-helpers.c:551
- **Type**: FIXME
- **Current Code**: `/* FIXME: i18n later */`
- **Problem**: Some error messages not translated
- **Impact**: Non-English users see English errors
- **Recommendation**: Wrap error strings with `_()` macro and add to .pot

---

#### Issue #11: Document Info Labels
- **File**: libdocument/ev-document.c:282
- **Type**: FIXME
- **Current Code**: `/* FIXME: Labels, or bookmarks though, can be done. */`
- **Problem**: Page labels not fully supported in all backends
- **Impact**: Some documents display page numbers instead of custom labels
- **Recommendation**: Implement label support in PDF/EPUB/DjVu backends

---

#### Issue #12: Backend Initialization Error Handling
- **File**: libdocument/ev-document-factory.c:282
- **Type**: FIXME
- **Current Code**: `/* FIXME: this really should not happen; the backend should... */`
- **Problem**: Missing graceful fallback when backend fails to initialize
- **Impact**: Cryptic errors when backend not available
- **Recommendation**: Add backend validation on startup

---

#### Issue #13: DjVu URI Support
- **File**: backend/djvu/djvu-document.c:159
- **Type**: FIXME
- **Current Code**: `/* FIXME: We could actually load uris */`
- **Problem**: DjVu backend only supports local files
- **Impact**: Cannot open remote DjVu documents
- **Recommendation**: Implement async download + local cache (like PDF backend)

---

#### Issue #14-16: DjVu Component File IDs
- **Files**: 
  - backend/djvu/djvu-links.c:84
  - backend/djvu/djvu-links.c:111
  - backend/djvu/djvu-links.c:196
- **Type**: FIXME (x3)
- **Current Code**: `/* FIXME: component file identifiers */`
- **Problem**: DjVu multi-file documents not fully supported
- **Impact**: Links in multi-page DjVu don't work correctly
- **Recommendation**: Track component file IDs and resolve links correctly

---

#### Issue #17-18: DjVu Shaped Links
- **Files**: 
  - backend/djvu/djvu-links.c:244
  - backend/djvu/djvu-links.c:263
- **Type**: FIXME (x2)
- **Current Code**: `/* FIXME: get bounding box since Xreader doesn't support shaped links */`
- **Problem**: Non-rectangular links not supported
- **Impact**: Some DjVu links not clickable
- **Recommendation**: Either implement shaped link support or improve fallback approximation

---

#### Issue #19: Unused Function in ev-window.c
- **File**: shell/ev-window.c:5994
- **Type**: FIXME (compiler warning)
- **Current Code**: `menubar_deactivate_cb()` defined but never called
- **Problem**: Dead code increases maintenance burden
- **Impact**: Code clutter, compiler warnings
- **Recommendation**: Remove function or add to TODO if future feature

---

#### Issue #20: Annotation Properties Dialog TODO
- **File**: shell/ev-annotation-properties-dialog.c
- **Type**: TODO
- **Problem**: Properties dialog incomplete for some annotation types
- **Impact**: User cannot customize all annotation properties
- **Recommendation**: Complete implementation for all annotation types

---

#### Issue #21: Long Press Popup Menu
- **File**: shell/ev-history-action-widget.c:176
- **Type**: TODO
- **Current Code**: `/* TODO: Show the popup menu after a long press too */`
- **Problem**: Long press gesture not implemented
- **Impact**: Touchscreen users cannot access menu via long press
- **Recommendation**: Implement GTK+ long-press gesture handler

---

#### Issue #22: Sidebar Annotations Icon
- **File**: shell/ev-sidebar-annotations.c:358
- **Type**: FIXME
- **Current Code**: `/* FIXME: use a better icon than EDIT */`
- **Problem**: Generic edit icon used for all annotations
- **Impact**: Users don't see annotation type visually
- **Recommendation**: Add annotation-type-specific icons

---

#### Issue #23: Sidebar Annotations Select All
- **File**: shell/ev-sidebar-annotations.c:379
- **Type**: FIXME
- **Current Code**: `/* FIXME: use better icon than select all */`
- **Problem**: Icon semantics unclear
- **Impact**: UI clarity reduced
- **Recommendation**: Use more intuitive icon or add tooltip

---

#### Issue #24: Daemon URI Validation
- **File**: shell/ev-daemon.c:118
- **Type**: TODO
- **Current Code**: `/* TODO Check that the uri exists */`
- **Problem**: D-Bus daemon doesn't validate URIs
- **Impact**: Can pass invalid paths to main app
- **Recommendation**: Add URI validation before passing to main window

---

#### Issue #25: Daemon Error Return
- **File**: shell/ev-daemon.c:347
- **Type**: FIXME
- **Current Code**: `// FIXME: shouldn't this return an error then?`
- **Problem**: Error condition not signaled back to caller
- **Impact**: Clients don't know operation failed
- **Recommendation**: Return error code instead of silently failing

---

### VERY LOW PRIORITY (External Code / Documentation)

#### Issue #26-28: DjVu Hyperlink Limitations
- **File**: backend/djvu/djvu-links.c
- **Type**: FIXME comments
- **Problem**: DjVu hyperlink attributes ignored
- **Impact**: Some metadata lost
- **Recommendation**: Low priority; DjVu declining in popularity

---

#### Issue #29: Encrypted Document Thumbnails
- **File**: thumbnailer/xreader-thumbnailer.c:146
- **Type**: FIXME
- **Current Code**: `/* FIXME: Create a thumb for cryp docs */`
- **Problem**: Password-protected documents cannot be thumbnailed
- **Impact**: No preview for encrypted PDFs
- **Status**: Complex; may require UI for password entry

---

#### Issue #30: Password Deletion from Keyring
- **File**: shell/ev-window.c
- **Type**: FIXME
- **Problem**: No UI to delete stored passwords from keyring
- **Impact**: Users cannot revoke saved passwords
- **Recommendation**: Add "Forget Password" option in document properties

---

---

## Summary Table

| Category | Count | Priority | Effort |
|----------|-------|----------|--------|
| Form Fields | 5 | High/Medium | 1-2 weeks |
| Annotations | 5 | Medium | 1 week |
| File Operations | 2 | Low | 2-3 days |
| Backends (DjVu) | 8 | Low | 2 weeks |
| UI/UX | 3 | Low | 3-5 days |
| Code Cleanup | 2 | Very Low | <1 day |
| **TOTAL** | **30** | **Mixed** | **~4-5 weeks** |

---

## Next Steps

1. **Triage**: Decide which issues to tackle first
2. **Backlog**: Add to GitHub Issues with labels
3. **Sprint**: Assign to milestones based on priority
4. **Review**: Each patch reviewed against this list

---

## Contributing

If you fix one of these issues, reference this document in your PR:
```
Closes: [Line X] from PERFORMANCE.md Issues
```

Or create a PR with title:
```
Fix: [Short description] (#XX)

Related to issue from code review (date).
```
