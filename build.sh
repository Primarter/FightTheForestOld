mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
mv ./Debug/bin/MyApplication ../
