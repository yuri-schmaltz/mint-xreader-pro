#!/usr/bin/python3
from dogtail.tree import root
from dogtail.utils import run
import time
import os

# Ensure accessibility is enabled
os.environ['GTK_MODULES'] = 'gail:atk-bridge'

print("Starting Xreader...")
pid = run('./build/shell/xreader')
time.sleep(2)

try:
    print("Finding application...")
    app = root.application('xreader')
    
    print("Clicking Help...", flush=True)
    app.menu('Help').click()
    time.sleep(1)
    
    print("Clicking About...")
    app.menu('Help').menuItem('About').click()
    time.sleep(2)
    
    print("\n--- SEARCHING ENTIRE DESKTOP FOR DIALOG ---")
    found = False
    for child in root.children:
        if child.roleName == 'dialog' or 'About' in child.name:
            print(f"FOUND ROOT CHILD: Name: '{child.name}', Role: {child.roleName}, App: {child.application.name if child.application else 'None'}")
            found = True
        # Also check children of applications
        if child.roleName == 'application':
            for app_child in child.children:
                if app_child.roleName == 'dialog' or 'About' in app_child.name:
                    print(f"FOUND APP CHILD: Name: '{app_child.name}', Role: {app_child.roleName}, App: {child.name}")
                    found = True
    
    if not found:
        print("Dialog NOT found anywhere on desktop.")
    print("--- END SEARCH ---\n")

except Exception as e:
    print(f"Error: {e}")

finally:
    # Kill the app
    os.system(f"kill {pid}")
