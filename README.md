# Developing smart\_ptr

You will need to install a compiler that supports C++20.

Download and run CMake:
```
git clone https://github.com/Izaron/static_ptr.git
cd static_ptr && mkdir build && cd build
cmake ..
```

You can specify the concrete compiler with `CMAKE_CXX_COMPILER` setting:
```
cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/g++-11
```
To build in Release mode specify `CMAKE_BUILD_TYPE` setting:
```
cmake -DCMAKE_BUILD_TYPE=Release ..
```

After you ran `cmake`, you can launch tests:
```
make check -j6
```

After you ran `cmake`, you can launch benchmark:
```
make bm -j6
./benchmark/bm
```
