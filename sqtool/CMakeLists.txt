
add_executable(sqtool sqtool.c)
set_target_properties(sqtool PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(sqtool squirrel_static sqstdlib_static)
if(NOT SQ_DISABLE_INSTALLER)
install(TARGETS sqtool RUNTIME DESTINATION ${INSTALL_BIN_DIR})
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(sqtool PROPERTIES COMPILE_FLAGS "-static -Wl,-static")
endif()
