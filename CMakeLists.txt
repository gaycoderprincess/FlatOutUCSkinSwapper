cmake_minimum_required(VERSION 3.27)
project(FlatOutUCSkinSwapper)

SET(CMAKE_SYSTEM_NAME Windows)

SET(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
SET(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
SET(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
SET(CMAKE_RANLIB i686-w64-mingw32-ranlib)

SET(CMAKE_CXX_STANDARD 20)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -masm=intel -s -static")

add_compile_definitions(NYA_COMMON_NO_D3D)
add_library(FlatOutUCSkinSwapper SHARED main.cpp)
target_include_directories(FlatOutUCSkinSwapper PUBLIC ../nya-common)
target_include_directories(FlatOutUCSkinSwapper PUBLIC ../nya-common/3rdparty)
target_include_directories(FlatOutUCSkinSwapper PUBLIC ../nya-common-fouc)
target_link_options(FlatOutUCSkinSwapper PRIVATE "-Wl,--exclude-all-symbols")

set_target_properties(FlatOutUCSkinSwapper PROPERTIES PREFIX "")
set_target_properties(FlatOutUCSkinSwapper PROPERTIES SUFFIX "_gcp.asi")

target_link_libraries(FlatOutUCSkinSwapper d3dx9)