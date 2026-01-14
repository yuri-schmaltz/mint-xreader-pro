import pyatspi
import time
import subprocess
import os

# Start xreader in background
print("Starting xreader...")
process = subprocess.Popen(['./build/shell/xreader'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(5)  # Give it time to register

try:
    print("Checking AT-SPI registry...")
    desktop = pyatspi.Registry.getDesktop(0)
    found = False
    for app in desktop:
        print(f"Found app: {app.name}")
        if 'xreader' in app.name.lower() or 'document viewer' in app.name.lower():
            found = True
            break
    
    if found:
        print("SUCCESS: xreader found in AT-SPI registry.")
    else:
        print("FAILURE: xreader NOT found in AT-SPI registry.")

finally:
    print("Killing xreader...")
    process.terminate()
