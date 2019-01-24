cmake_minimum_required(VERSION 3.4)
project(modelr)

include(FindPkgConfig)

pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(PNG REQUIRED libpng)

####

set(CMAKE_CXX_STANDARD 17)

set(INCLUDE_DIRS ${GLFW_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
set(LIBRARIES ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${PNG_LIBRARIES})
set(SOURCES src/main.cpp
            src/render/meshbuilder.cpp
            src/render/wavefront.cpp
            src/render/shader.cpp
            src/res/lodepng.cpp
            src/gameobject.cpp
            src/scene.cpp
            src/renderer.cpp
            src/input.cpp
            src/modelobject.cpp
            src/resources.cpp
            src/utils.cpp
            src/res/assets.cpp)
configure_file(src/config.h.in ${CMAKE_BINARY_DIR}/config.h)

include_directories(${INCLUDE_DIRS} ${CMAKE_BINARY_DIR})

####

add_executable(modelr ${SOURCES})
target_link_libraries(modelr ${LIBRARIES})
