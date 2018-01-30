
# add_library(serializer_json MODULE sqt_serjson.c)
# set_target_properties(serializer_json PROPERTIES PREFIX "")
# set_target_properties(serializer_json PROPERTIES OUTPUT_NAME "json")
#
# target_link_libraries(serializer_json package_host)
# # target_include_directories(json PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#
# if(NOT DEFINED SQ_DISABLE_INSTALLER)
# install(TARGETS serializer_json LIBRARY DESTINATION ${INSTALL_LIB_DIR}/cpkgs/serializer)
# #     if(DEFINED INSTALL_INC_DIR)
# #       set(PUB_HEADERS sqt_serializer.h)
# #       install(FILES ${PUB_HEADERS} DESTINATION ${INSTALL_INC_DIR}/pkgs)
# #     endif()
# endif()

add_sqmetapkg_lib( serializer)

#define_package_so( json SOURCES sqt_serjson.c)
add_sqpackage_lib( serializer.json SOURCES sqt_serjson.c)

#define_package_so( binary SOURCES sqt_serbin.c)
add_sqpackage_lib( serializer.binary SOURCES sqt_serbin.c)
