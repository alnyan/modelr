cmake_minimum_required(VERSION 3.4)
project(modelr)

include(FindPkgConfig)

pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(PNG REQUIRED libpng)

####

set(INCLUDE_DIRS ${GLFW_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${PNG_INCLUDE_DIRS})
set(LIBRARIES ${GLFW_LIBRARIES} ${GLEW_LIBRARIES} ${PNG_LIBRARIES})
set(SOURCES src/main.cpp
            src/render/model.cpp
            src/render/shader.cpp
            src/render/texture.cpp
            src/res/lodepng.cpp
            src/render/material.cpp
            src/gameobject.cpp
            src/scene.cpp)

include_directories(${INCLUDE_DIRS})

####

add_executable(modelr ${SOURCES})
target_link_libraries(modelr ${LIBRARIES})
