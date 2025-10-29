#!/usr/bin/env python3
import zipfile
import os
import shutil

# Clean and create build directory
if os.path.exists('layer_build'):
    shutil.rmtree('layer_build')
os.makedirs('layer_build/nodejs')

# Copy shared files
shutil.copytree('shared', 'layer_build/nodejs/shared')

# Create zip
with zipfile.ZipFile('shared-layer.zip', 'w', zipfile.ZIP_DEFLATED) as z:
    for root, dirs, files in os.walk('layer_build'):
        for file in files:
            file_path = os.path.join(root, file)
            arc_name = os.path.relpath(file_path, 'layer_build')
            z.write(file_path, arc_name)

print("Layer packaged as shared-layer.zip")