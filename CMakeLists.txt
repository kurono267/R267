cmake_minimum_required (VERSION 3.7.2)
# r267 render library and examples
project (R267)

add_definitions(-std=c++17)

find_package(PkgConfig REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
include_directories(${GLFW_INCLUDE_DIRS} ${GLM_INCLUDE_DIRS} ${VULKAN_INCLUDE_DIRS})
link_directories(${PROJECT_SOURCE_DIR}/bin)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/external/include)

add_definitions(-fPIC -O3 -DEBUG)

# Custom command for compile shaders
set(GLSL_COMPILER /usr/bin/glslangValidator)

function (get_shortest_file_extension _sourceFile _extVar)
    # get_filename_component returns extension from first occurrence of . in file name
    # this function computes the extension from last occurrence of . in file name
    string (FIND "${_sourceFile}" "." _index REVERSE)
    if (_index GREATER -1)
        math (EXPR _index "${_index} + 1")
        string (SUBSTRING "${_sourceFile}" ${_index} -1 _sourceExt)
    else()
        set (_sourceExt "")
    endif()
    set (${_extVar} "${_sourceExt}" PARENT_SCOPE)
endfunction()

function(add_shader file target)
	get_filename_component(file_name ${file} NAME_WE)
	get_filename_component(file_path ${file} DIRECTORY)
	get_shortest_file_extension(${file} type)
	message(filename:${file_name})
	message(file_path:${file_path})
	set(SPIRV ${CMAKE_SOURCE_DIR}/shaders/${file_path}/${file_name}_${type}.spv)
	set(GLSL ${CMAKE_SOURCE_DIR}/shaders/${file_path}/${file_name}.${type})
	#execute_process(COMMAND ${GLSL_COMPILER} -V ${GLSL} -o ${SPIRV})
    set(custom shader_${target}_${file_name}_${type})
    add_custom_target(shader_${target}_${file_name}_${type}
        command ${GLSL_COMPILER} -V ${GLSL} -o ${SPIRV})
    add_dependencies(${target} ${custom})
endfunction(add_shader)

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(viewer)