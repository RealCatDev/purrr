add_executable(example main.c)
target_link_libraries(example purrr)

# https://stackoverflow.com/a/71317698
add_custom_target(shaders)

set(HLSL_SHADER_FILES vertex.hlsl fragment.hlsl)

set_source_files_properties(vertex.hlsl PROPERTIES ShaderType "vs")
set_source_files_properties(fragment.hlsl PROPERTIES ShaderType "ps")
set_source_files_properties(${HLSL_SHADER_FILES} PROPERTIES ShaderModel "6_0")

foreach(FILE ${HLSL_SHADER_FILES})
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  get_source_file_property(shadertype ${FILE} ShaderType)
  get_source_file_property(shadermodel ${FILE} ShaderModel)
  add_custom_command(TARGET shaders
                     COMMAND dxc.exe -spirv -E main -T ${shadertype}_${shadermodel} $<IF:$<CONFIG:DEBUG>,-Od,-O1> -Fo ${CMAKE_CURRENT_BINARY_DIR}/${FILE_WE}.spv ${FILE}
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)
endforeach(FILE)

add_custom_target(copy)

file(GLOB_RECURSE COPY_FILES ${CMAKE_CURRENT_SOURCE_DIR}/**.png)

foreach(file ${COPY_FILES})
  add_custom_command(TARGET copy PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                         copy ${file} ${CMAKE_CURRENT_BINARY_DIR})
endforeach()

add_dependencies(example shaders)
add_dependencies(example copy)