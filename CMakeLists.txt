cmake_minimum_required(VERSION 3.31)
project(Zenith)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ZENITH_EXT_WINDOWING "Enable integrated windowing support" ON)
option(ZENITH_GLFW "Enable GLFW windowing support" ON)
option(ZENITH_VULKAN "Enable Vulkan support" ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

add_library(Zenith SHARED
        extensions/windowing/glfw.cpp
        vulkan/utils/extensions.cpp
        vulkan/utils/errors.cpp
        vulkan/device.cpp)

target_include_directories(Zenith PUBLIC include)
target_compile_options(Zenith PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
)

target_link_libraries(Zenith PUBLIC Vulkan::Vulkan)

if (APPLE)
    message(STATUS "macOS detected - configuring MoltenVK")

    if (DEFINED ENV{VULKAN_SDK})
        set(PREFERRED_MOLTENVK_PATH "$ENV{VULKAN_SDK}/lib")
        message(STATUS "Using VULKAN_SDK MoltenVK: ${PREFERRED_MOLTENVK_PATH}")
    else ()
        set(PREFERRED_MOLTENVK_PATH "/opt/homebrew/lib")
        message(STATUS "VULKAN_SDK not set, using system MoltenVK: ${PREFERRED_MOLTENVK_PATH}")
    endif ()

    if (EXISTS "${PREFERRED_MOLTENVK_PATH}/libMoltenVK.dylib")
        message(STATUS "Found MoltenVK at: ${PREFERRED_MOLTENVK_PATH}/libMoltenVK.dylib")

        set_target_properties(Zenith PROPERTIES
                INSTALL_RPATH "${PREFERRED_MOLTENVK_PATH}"
                BUILD_RPATH "${PREFERRED_MOLTENVK_PATH}"
                INSTALL_RPATH_USE_LINK_PATH FALSE
        )

        if (DEFINED ENV{VULKAN_SDK} AND EXISTS "$ENV{VULKAN_SDK}/include")
            target_include_directories(Zenith PUBLIC "$ENV{VULKAN_SDK}/include")
        endif ()

    else ()
        message(WARNING "MoltenVK not found at preferred path: ${PREFERRED_MOLTENVK_PATH}")
        message(STATUS "Please install the Vulkan SDK from: https://vulkan.lunarg.com/sdk/home#mac")
    endif ()

    target_link_libraries(Zenith PUBLIC
            "-framework Metal"
            "-framework Foundation"
            "-framework QuartzCore"
            "-framework IOSurface"
    )

    target_compile_definitions(Zenith PRIVATE
            VK_USE_PLATFORM_METAL_EXT
            VK_ENABLE_BETA_EXTENSIONS
    )
endif ()

if (ZENITH_EXT_WINDOWING)
    target_compile_definitions(Zenith PUBLIC ZENITH_EXT_WINDOWING)
endif ()

if (ZENITH_GLFW)
    target_compile_definitions(Zenith PUBLIC ZENITH_GLFW)
    target_link_libraries(Zenith PUBLIC glfw)
endif ()

if (ZENITH_VULKAN)
    target_compile_definitions(Zenith PUBLIC ZENITH_VULKAN)
else ()
    message(WARNING "Vulkan support is disabled. Some features may not work.")
endif ()

add_executable(TriangleTest examples/triangle/main.cpp)
target_link_libraries(TriangleTest PRIVATE Zenith glm::glm glfw)
target_include_directories(TriangleTest PUBLIC include)

if (ZENITH_EXT_WINDOWING)
    target_compile_definitions(TriangleTest PRIVATE ZENITH_EXT_WINDOWING)
endif ()

if (ZENITH_GLFW)
    target_compile_definitions(TriangleTest PRIVATE ZENITH_GLFW)
endif ()

if (ZENITH_VULKAN)
    target_compile_definitions(TriangleTest PRIVATE ZENITH_VULKAN)
endif ()

if (APPLE AND DEFINED PREFERRED_MOLTENVK_PATH)
    set_target_properties(TriangleTest PROPERTIES
            INSTALL_RPATH "${PREFERRED_MOLTENVK_PATH}"
            BUILD_RPATH "${PREFERRED_MOLTENVK_PATH}"
            INSTALL_RPATH_USE_LINK_PATH FALSE
    )
endif ()

message(STATUS "=== Zenith Configuration ===")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Windowing: ${ZENITH_EXT_WINDOWING}")
message(STATUS "GLFW: ${ZENITH_GLFW}")
message(STATUS "Vulkan: ${ZENITH_VULKAN}")
if (APPLE AND DEFINED PREFERRED_MOLTENVK_PATH)
    message(STATUS "MoltenVK Path: ${PREFERRED_MOLTENVK_PATH}")
endif ()
message(STATUS "==============================")
