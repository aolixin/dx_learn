# cmake_minimum_required(VERSION 3.10.0)
project(LandingMoon VERSION 0.1.0 LANGUAGES C CXX)

add_compile_definitions(UNICODE _UNICODE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE HLSL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Shaders/*.hlsl" "${CMAKE_CURRENT_SOURCE_DIR}/Resource/Shaders/*.hlsli")
message(STATUS "Found HLSL files: ${HLSL_FILES}")

foreach(HLSL_FILE ${HLSL_FILES})

	get_filename_component(HLSL_FDIR ${HLSL_FILE} DIRECTORY)
	get_filename_component(HLSL_FBASENAME_WE ${HLSL_FILE} NAME_WE)
	string(CONCAT HLSL_FNAME_WE ${HLSL_FDIR} / ${HLSL_FBASENAME_WE})

	string(LENGTH ${HLSL_FBASENAME_WE} LEN_FNAME_WE)
	math(EXPR LEN_FNAME_WE "${LEN_FNAME_WE}-2")
	string(SUBSTRING ${HLSL_FBASENAME_WE} ${LEN_FNAME_WE} 2 ENTRY_POINT)
	string(TOLOWER ${ENTRY_POINT} SHADER_TYPE)

	if ("${SHADER_TYPE}" STREQUAL "vs") 
		set(SHADER_TYPE "Vertex")
	elseif("${SHADER_TYPE}" STREQUAL "hs") 
		set(SHADER_TYPE "Hull")
	elseif("${SHADER_TYPE}" STREQUAL "ds") 
		set(SHADER_TYPE "Domain")
	elseif("${SHADER_TYPE}" STREQUAL "gs") 
		set(SHADER_TYPE "Geometry")
	elseif("${SHADER_TYPE}" STREQUAL "ps")
		set(SHADER_TYPE "Pixel")
	elseif("${SHADER_TYPE}" STREQUAL "cs")
		set(SHADER_TYPE "Compute")
	endif()
    # compile hlsl
	set_source_files_properties(${HLSL_FILE} PROPERTIES
								VS_SHADER_OBJECT_FILE_NAME ShaderBin/${HLSL_FBASENAME_WE}.cso
								VS_SHADER_TYPE ${SHADER_TYPE}
								VS_SHADER_MODEL 5.0
								VS_SHADER_ENTRYPOINT ${ENTRY_POINT}
								VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:true>
								VS_SHADER_ENABLE_DEBUG $<$<CONFIG:Debug>:true>)
endforeach()
source_group("Shaders" FILES ${HLSL_FILES})

add_executable(LandingMoon WIN32 ${SOURCE_FILES} ${HEADER_FILES} ${HLSL_FILES})


target_include_directories(LandingMoon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(LandingMoon d3d11.lib dxgi.lib dxguid.lib D3DCompiler.lib d2d1.lib dwrite.lib winmm.lib)

# Assimp
set_target_properties(LandingMoon PROPERTIES
    VS_DEBUGGER_ENVIRONMENT 
    "PATH=${ASSIMP_LIBRARY_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>;%PATH%"
)



message(STATUS "Assimp DLL path: ${ASSIMP_LIBRARY_OUTPUT_DIRECTORY}")


target_link_libraries(LandingMoon assimp)

set_target_properties(LandingMoon PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/release"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/relwithdebinfo"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin/minsizerel"
)


add_custom_command(TARGET LandingMoon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ASSIMP_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/assimp-vc143-mt.dll"
        "$<TARGET_FILE_DIR:LandingMoon>"
)



set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resource")
set(DEST_RESOURCE_DIR "$<TARGET_FILE_DIR:LandingMoon>/Resource")

add_custom_command(TARGET LandingMoon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEST_RESOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${RESOURCE_DIR}"
        "${DEST_RESOURCE_DIR}"
    COMMENT "Copying entire Resource folder to runtime output directory"
)


add_custom_command(TARGET LandingMoon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${RESOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/Resource"
)



add_custom_command(TARGET LandingMoon POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:LandingMoon>/ShaderBin"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_BINARY_DIR}/ShaderBin"
            "$<TARGET_FILE_DIR:LandingMoon>/ShaderBin"
    COMMENT "Copying compiled shaders to runtime directory"
)
