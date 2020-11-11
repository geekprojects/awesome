cmake_minimum_required(VERSION 3.16)
project(awesome)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
#find_package(Vulkan REQUIRED)

#pkg_check_modules(glfw3 REQUIRED glfw3)
include(FindOpenGL)
include(FindGLUT)
pkg_check_modules(sdl2 sdl2)
pkg_check_modules(libgeek REQUIRED libgeek)

add_definitions(${libgeek_CFLAGS} ${sdl2_CFLAGS})
include_directories(include)

add_definitions(-DAWESOME_SRC=${PROJECT_SOURCE_DIR})

add_executable(awesome-display-server
    include/awesome/displaydriver.h
    include/awesome/displayserver.h
    include/awesome/interface.h
    include/awesome/window.h
    include/awesome/compositor.h
    include/awesome/displaydrivers/sdl.h
    include/awesome/interfaces/frontier.h
    src/ads/main.cpp
    src/ads/compositor.cpp
    src/ads/displayserver.cpp
    src/ads/displaydriver.cpp
    src/ads/display.cpp
    src/ads/window.cpp
    src/ads/interface.cpp
    src/ads/drivers/sdl/sdl.cpp
    src/ads/interfaces/frontier/frontier.cpp
)
target_link_libraries(awesome-display-server ${libgeek_LDFLAGS} ${sdl2_LDFLAGS})


#add_executable(awesome-vulkan src/vulkan/main.cpp src/vulkan/vulkanengine.cpp src/vulkan/vulkanengine.h)
#target_link_libraries(awesome-vulkan ${glfw3_LDFLAGS} ${libgeek_LDFLAGS} ${Vulkan_LIBRARY})

