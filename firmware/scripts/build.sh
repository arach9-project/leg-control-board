# from the hexapod/ root
# mkdir -p build
# cmake -S . -B build -DPython_EXECUTABLE=$(mjpython -c "import sys; print(sys.executable)")
# cmake --build build -j$(nproc)

#!/bin/bash
set -e
ROOT=$(pwd)
rm -rf build
mkdir -p build
cmake -S . -B build -G Ninja \
  -DPython_EXECUTABLE=$(mjpython -c "import sys; print(sys.executable)")
cmake --build build -j$(sysctl -n hw.logicalcpu)
