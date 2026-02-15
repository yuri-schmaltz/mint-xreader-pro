gui_test_runner = find_program(join_paths(meson.current_source_dir(), '..', 'install-scripts', 'gui-test-run.sh'))

test_cases = [
    'testFileMenu.py',
    'testEditMenu.py',
    'testHelpMenu.py',
    'testZoom.py',
    'testGoMenu.py',
    'testBookmarksMenu.py',
    'testTabsShortcuts.py',
    'testEncryptedFile.py',
    'testFileReloading.py',
    'testWrongFileExtension.py'
]

foreach case : test_cases
    test(
        case,
        gui_test_runner,
        args: [case, xreader.full_path()],
        is_parallel: false,
        depends: xreader,
        workdir: join_paths(prefix, bindir),
        timeout: 180,
    )
endforeach
