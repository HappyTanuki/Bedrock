include_guard(GLOBAL)
function(_BedrockBuildConfig_copy_compile_commands)
    cmake_parse_arguments(ARG "" "NAME;SOURCE;DESTINATION" "" ${ARGN})
    if(NOT ARG_NAME OR NOT ARG_SOURCE OR NOT ARG_DESTINATION)
        message(FATAL_ERROR "_BedrockBuildConfig_copy_compile_commands requires NAME, SOURCE, DESTINATION")
    endif()
    # Visual Studio generators do not emit compile_commands.json. Ninja and
    # Makefile generators do; the file is created during the build, so do not
    # test for its existence at configure time.
    if(CMAKE_EXPORT_COMPILE_COMMANDS AND
       CMAKE_GENERATOR MATCHES "^(Ninja|Unix Makefiles|MinGW Makefiles|MSYS Makefiles|NMake Makefiles)$")
        add_custom_target(${ARG_NAME} ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ARG_SOURCE}" "${ARG_DESTINATION}"
            COMMENT "Copy compile_commands.json" VERBATIM)
    endif()
endfunction()
