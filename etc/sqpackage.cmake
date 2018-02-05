include(CMakeParseArguments)

#######################################
# Init

function(add_sqmetaroot tgt_name)
    set(sqpackage_list_file ${CMAKE_BINARY_DIR}/sq_packages_list.h)
    
    add_library(${tgt_name} STATIC etc/empty_c_file.c)
    target_link_libraries(${tgt_name} PUBLIC package_host_lib)
#    target_include_directories(${tgt_name} INTERFACE ${CMAKE_BINARY_DIR})
    target_include_directories(${tgt_name} PRIVATE
                    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
                    $<INSTALL_INTERFACE:${INSTALL_INC_DIR}>)
    
    target_compile_definitions(${tgt_name}
        INTERFACE SQ_HAVE_BUILTIN_PACKAGES
        PRIVATE SQ_BUILTIN_PACKAGE_LIST=\"sq_packages_list.h\")

    if(NOT DEFINED SQ_DISABLE_INSTALLER)
        #install(TARGETS ${tgt_name} EXPORT squirrel ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
        install(FILES ${sqpackage_list_file} DESTINATION ${INSTALL_INC_DIR})
    endif()

    file(WRITE ${sqpackage_list_file} "// packages for ${tgt_name}\n")
    
    set(sqmetapkg_target ${tgt_name} PARENT_SCOPE)
    set(sqmetapkg_target_type LIB PARENT_SCOPE)
    set(sqpackage_list_file ${sqpackage_list_file} PARENT_SCOPE)
endfunction(add_sqmetaroot)

#######################################
# vname parse

macro(sqpackage_parse_pkg_vname tgt_pfx)
    string(REGEX REPLACE "\\." "/" cpkg_path ${pkg_vname})
    string(REGEX REPLACE "\\." "_" cpkg_cname ${pkg_vname})
    
    if( ${cpkg_path} MATCHES "^(.+)/([^/]+)$")
        set(cpkg_proot ${CMAKE_MATCH_1})
        set(cpkg_pname ${CMAKE_MATCH_2})
    else()
        set(cpkg_proot "")
        set(cpkg_pname ${cpkg_path})
    endif()
    
    set(cpkg_target ${tgt_pfx}_${cpkg_cname})
endmacro(sqpackage_parse_pkg_vname)

#######################################
# Meta packages

function(add_sqmetapkg_lib pkg_vname)
    MESSAGE( STATUS "sqmetapkg lib: (${pkg_vname})")
endfunction(add_sqmetapkg_lib)

function(add_sqmetapkg_so pkg_vname)
    sqpackage_parse_pkg_vname(sqmetapkgso)
    MESSAGE( STATUS "sqmetapkg so: ${pkg_vname}, target:${cpkg_target}, inst:/${cpkg_proot}")
    
    add_library(${cpkg_target} MODULE qaz.c)
    set_target_properties(${cpkg_target} PROPERTIES PREFIX "")
    set_target_properties(${cpkg_target} PROPERTIES OUTPUT_NAME "${cpkg_pname}")

    target_link_libraries(${cpkg_target} package_host_so)

    if(NOT DEFINED SQ_DISABLE_INSTALLER)
        install(TARGETS ${cpkg_target} LIBRARY DESTINATION ${INSTALL_LIB_DIR}/cpkgs/${cpkg_proot})
    endif()

    set(sqmetapkg_target ${cpkg_target} PARENT_SCOPE)
    set(sqmetapkg_target_type SO PARENT_SCOPE)
endfunction(add_sqmetapkg_so)

#######################################
# Packages

function(add_sqpackage_so pkg_vname)
    cmake_parse_arguments(
        arg # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "" # list of names of mono-valued arguments
        "SOURCES" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    sqpackage_parse_pkg_vname(sqpackageso)
    MESSAGE( STATUS "sqpackage so: ${pkg_vname} , target:${cpkg_target}, inst:/${cpkg_proot}")

    add_library(${cpkg_target} MODULE ${arg_SOURCES})
    set_target_properties(${cpkg_target} PROPERTIES PREFIX "")
    set_target_properties(${cpkg_target} PROPERTIES OUTPUT_NAME "${cpkg_pname}")

    target_link_libraries(${cpkg_target} package_host_so)
    
    target_compile_definitions(${cpkg_target} PRIVATE
        SQPACKAGE_VNAME="${pkg_vname}"
        SQPACKAGE_LOADFCT=sqload_${cpkg_cname}
    )

    if(NOT DEFINED SQ_DISABLE_INSTALLER)
        install(TARGETS ${cpkg_target} LIBRARY DESTINATION ${INSTALL_LIB_DIR}/cpkgs/${cpkg_proot})
    endif()

    set(sqpackage_target ${cpkg_target} PARENT_SCOPE)
endfunction(add_sqpackage_so)

function(add_sqpackage_lib pkg_vname)
    cmake_parse_arguments(
        arg # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "" # list of names of mono-valued arguments
        "SOURCES" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )
    sqpackage_parse_pkg_vname(sqpackagelib)
    MESSAGE( STATUS "sqpackage lib: ${pkg_vname} @ ${sqmetapkg_target}, target:${cpkg_target}")

    add_library(${cpkg_target} STATIC ${arg_SOURCES})

    ##target_include_directories(${cpkg_target} PRIVATE $<TARGET_PROPERTY:package_host_lib,INTERFACE_INCLUDE_DIRECTORIES>)
    target_link_libraries(${cpkg_target} PUBLIC package_host_lib)
    
    target_compile_definitions(${cpkg_target} PRIVATE
        SQPACKAGE_VNAME="${pkg_vname}"
        SQPACKAGE_LOADFCT=sqload_${cpkg_cname}
    )

    #target_sources(${sqmetapkg_target} PRIVATE $<TARGET_OBJECTS:${cpkg_target}>)
    #target_link_libraries(${sqmetapkg_target} PRIVATE ${cpkg_target})
    #get_property(prev_props TARGET sq_builtin_packages PROPERTY QAZ)
    set_property( TARGET ${sqmetapkg_target} APPEND PROPERTY QAZ ${cpkg_target})

    if( ${sqmetapkg_target_type} STREQUAL LIB)
        file(APPEND ${sqpackage_list_file}  "SQ_PACKAGE_DEF(${pkg_vname},sqload_${cpkg_cname})\n")
    endif()

    set(sqpackage_target ${cpkg_target} PARENT_SCOPE)
endfunction(add_sqpackage_lib)

