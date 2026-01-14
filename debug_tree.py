#!/usr/bin/env python3
from dogtail.tree import root
import time

# Wait for app to settle
time.sleep(2)

print("Dumping Accessibility Tree for Xreader:")
try:
    app = root.application('xreader')
    print(f"App computed: {app.name}")
    # Walk the tree manually to be safe
    def walk(node, depth=0):
        print("  " * depth + f"Name: '{node.name}', Role: '{node.roleName}', Desc: '{node.description}'")
        for child in node.children:
            walk(child, depth + 1)
    
    walk(app)
    
except Exception as e:
    print(f"Could not find xreader: {e}")
    print("Dumping all top-level apps:")
    for child in root.children:
        print(f"  App: {child.name}")
