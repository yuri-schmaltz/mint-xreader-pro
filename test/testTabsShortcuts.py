#!/usr/bin/python3

# Test tab lifecycle using default tab shortcuts.

import os
import shutil
import subprocess
import sys
import tempfile
import time

from testCommon import run_app, bail

from dogtail.procedural import *


def open_in_running_instance(path):
    subprocess.run([sys.argv[1], path], check=False, timeout=8)
    time.sleep(1)


def tab_exists(name):
    try:
        focus.widget(name, roleName='page tab')
        return True
    except Exception:
        return False


def assert_tab_exists(name):
    if not tab_exists(name):
        raise RuntimeError(f"Expected tab '{name}' to exist")


def assert_tab_missing(name):
    if tab_exists(name):
        raise RuntimeError(f"Expected tab '{name}' to be closed")


tmp_dir = None

try:
    fixture_dir = os.path.dirname(__file__)
    tmp_dir = tempfile.mkdtemp(prefix='xreader-tabs-')

    tab_b_path = os.path.join(tmp_dir, 'tab-b.pdf')
    tab_c_path = os.path.join(tmp_dir, 'tab-c.pdf')
    tab_d_path = os.path.join(tmp_dir, 'tab-d.pdf')

    shutil.copyfile(os.path.join(fixture_dir, 'test-page-labels.pdf'), tab_b_path)
    shutil.copyfile(os.path.join(fixture_dir, 'test-links.pdf'), tab_c_path)
    shutil.copyfile(os.path.join(fixture_dir, 'test-page-labels.pdf'), tab_d_path)

    run_app(file='test-links.pdf')

    # Open two extra tabs via second instance dispatch.
    open_in_running_instance(tab_b_path)
    open_in_running_instance(tab_c_path)

    assert_tab_exists('tab-b.pdf')
    assert_tab_exists('tab-c.pdf')

    # Close tabs to the left from current tab (tab-c).
    click('tab-c.pdf', roleName='page tab')
    keyCombo('<Control><Alt>Left')
    time.sleep(0.5)
    assert_tab_exists('tab-c.pdf')
    assert_tab_missing('tab-b.pdf')
    assert_tab_missing('test-links.pdf')

    # Re-open tabs and close to the right from tab-b.
    open_in_running_instance(tab_b_path)
    open_in_running_instance(tab_d_path)
    click('tab-b.pdf', roleName='page tab')
    keyCombo('<Control><Alt>Right')
    time.sleep(0.5)
    assert_tab_exists('tab-b.pdf')
    assert_tab_missing('tab-d.pdf')

    # Create left and right neighbors and close others from middle tab (tab-b).
    open_in_running_instance(tab_d_path)
    click('tab-b.pdf', roleName='page tab')
    keyCombo('<Control><Shift>w')
    time.sleep(0.5)
    assert_tab_exists('tab-b.pdf')
    assert_tab_missing('tab-c.pdf')
    assert_tab_missing('tab-d.pdf')

    # Open one more tab and close current via Ctrl+F4.
    open_in_running_instance(tab_d_path)
    click('tab-d.pdf', roleName='page tab')
    keyCombo('<Control>F4')
    time.sleep(0.5)
    assert_tab_exists('tab-b.pdf')
    assert_tab_missing('tab-d.pdf')

    click('File', roleName='menu')
    click('Close All Windows', roleName='menu item')

except Exception:
    bail()
finally:
    try:
        shutil.rmtree(tmp_dir)
    except Exception:
        pass
