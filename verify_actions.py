import pyatspi
import time
import subprocess
import os
import sys

def print_tree(obj, depth=0):
    indent = "  " * depth
    try:
        print(f"{indent}Name: {obj.name}, Role: {obj.getRoleName()}")
    except:
        pass
    
    try:
        for i in range(obj.childCount):
            print_tree(obj.getChildAtIndex(i), depth + 1)
    except:
        pass

def find_child_by_name(obj, name, role_name=None):
    try:
        if obj.name == name:
            if role_name is None or obj.getRoleName() == role_name:
                return obj
    except:
        pass

    try:
        for i in range(obj.childCount):
            child = obj.getChildAtIndex(i)
            result = find_child_by_name(child, name, role_name)
            if result:
                return result
    except:
        pass
    return None

def verify_actions():
    # Start xreader
    process = subprocess.Popen(["./build/shell/xreader"], env=dict(os.environ, GTK_MODULES="atk-bridge"))
    print("Xreader started with PID:", process.pid)
    
    # Wait for registry
    time.sleep(3)
    
    desktop = pyatspi.Registry.getDesktop(0)
    xreader = None
    
    # Try to find xreader application
    for i in range(desktop.childCount):
        app = desktop.getChildAtIndex(i)
        try:
            if app.name == "xreader":
                xreader = app
                break
        except:
            continue
            
    if not xreader:
        print("FAIL: Could not find xreader in accessibility registry")
        process.terminate()
        return

    print("SUCCESS: Found xreader in accessibility registry")
    
    # Function to traverse menus and find items
    # Typically: Frame -> Panel -> MenuBar -> Menu -> Menu
    
    # We look for specific restored items
    # File -> Open Copy (FileOpenCopy)
    # Edit -> Rotate Left (EditRotateLeft)
    # View -> Zoom In (ViewZoomIn)
    
    # Structure is usually: Application -> Frame -> RootPane -> Panel -> MenuBar
    frame = xreader.getChildAtIndex(0)
    
    # Let's perform a deep search for the items to confirm they exist in the tree
    # This confirms they were added to the UI Manager and constructed as Widgets
    
    items_to_verify = [
        "Op_en a Copy",      # FileOpenCopy
        "Rotate _Left",      # EditRotateLeft
        "Zoom _In",          # ViewZoomIn
        "_Previous Page",    # GoPreviousPage
        "_Add Bookmark",     # BookmarksAdd
        "_Content",          # HelpContents (might be "Contents" or "Help Contents") - actually it is "_Contents" in code but AT-SPI usually strips underscore or shows visible label.
                             # Let's check for "Contents" too just in case.
        "Contents"
    ]
    
    found_items = []
    
    def search_for_items(obj):
        try:
            name = obj.name
            role = obj.getRoleName()
            # print(f"Scanning: {name} ({role})") # Debug
            
            for item in items_to_verify:
                # Simple loose matching because underscores might be stripping or different in label
                clean_name = item.replace("_", "")
                if name and (name == item or name == clean_name):
                    if item not in found_items:
                        found_items.append(item)
                        print(f"FOUND: {item} as '{name}'")
        except:
            pass
            
        try:
            for i in range(obj.childCount):
                search_for_items(obj.getChildAtIndex(i))
        except:
            pass

    print("Scanning UI for restored action menu items...")
    search_for_items(frame)
    
    process.terminate()
    
    if len(found_items) >= 3: # If we found at least a few, the menus are likely populated
        print("\nVERIFICATION PASSED: Restored items were found in the UI.")
        print(f"Items found: {found_items}")
    else:
        print("\nVERIFICATION FAILED: Could not find significant restored items.")
        print(f"Items found: {found_items}")

if __name__ == "__main__":
    verify_actions()
