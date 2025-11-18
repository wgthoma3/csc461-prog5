# CMAKE Config file for Program 5
#
# Build this project with the following command
#     `cmake -S . -B build`
# 

cmake_minimum_required(VERSION 3.27)

project(prog5 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Get all source files
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    "src/*.cpp"
)

add_executable(prog5 ${SRC})

target_include_directories(prog5 PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# find_package(GLM REQUIRED) # <- This can be a header-only library under include
find_package(SDL3 REQUIRED)
find_package(OpenGL REQUIRED COMPONENTS GLES2) # This method of finding OpenGL is CMake >= 3.27

target_link_libraries(prog5 PUBLIC
    # glm::glm
    SDL3::SDL3
    OpenGL::GLES2
)

# handle windows cross-compile
if(WIN32 AND NOT CMAKE_BUILT_TYPE STREQUAL "Debug")
    set_target_properties(prog5 PROPERTIES WIN32_EXECUTABLE ON)
endif()

# define DEBUG macro on cmake Debug build config
target_compile_definitions(prog5 PRIVATE 
    $<$<CONFIG:Debug>:DEBUG=1>
)
