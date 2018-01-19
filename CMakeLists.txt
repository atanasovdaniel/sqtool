if(MSVC)
  cmake_minimum_required(VERSION 3.4)
else()
  cmake_minimum_required(VERSION 2.8)
endif()

include(CMakeParseArguments)

function(define_package_so pkg_name)
    cmake_parse_arguments(
        arg # prefix of output variables
        "" # list of names of the boolean arguments (only defined ones will be true)
        "" # list of names of mono-valued arguments
        "SOURCES" # list of names of multi-valued arguments (output variables are lists)
        ${ARGN} # arguments of the function to parse, here we take the all original ones
    )
#MESSAGE( STATUS "CMAKE_CURRENT_SOURCE_DIR:  " ${CMAKE_CURRENT_SOURCE_DIR} )
    string(LENGTH ${CPKGS_ROOT_DIR}/ cpkg_root_len)
#MESSAGE( STATUS "cpkg_root_len:    " ${cpkg_root_len} )
    string(SUBSTRING ${CMAKE_CURRENT_SOURCE_DIR} ${cpkg_root_len} -1 cpkg_path_root)
#MESSAGE( STATUS "cpkg_path_root:    " ${cpkg_path_root} )
    string(REGEX REPLACE "/" "." cpkg_name_root ${cpkg_path_root})
#MESSAGE( STATUS "cpkg_name_root:    " ${cpkg_name_root} )
    string(REGEX REPLACE "/" "_" cpkg_cname_root ${cpkg_path_root})
#MESSAGE( STATUS "cpkg_cname_root:    " ${cpkg_cname_root} )

    set(cpkg_path ${cpkg_path_root}/${pkg_name})
    set(cpkg_full_name ${cpkg_name_root}.${pkg_name})
    
    set(package_target ${cpkg_cname_root}_${pkg_name})
    set(package_target ${package_target} PARENT_SCOPE)
    
    MESSAGE( STATUS "so package: ${package_target}:  " ${cpkg_full_name})

    add_library(${package_target} MODULE ${arg_SOURCES})
    set_target_properties(${package_target} PROPERTIES PREFIX "")
    set_target_properties(${package_target} PROPERTIES OUTPUT_NAME "${pkg_name}")

    target_link_libraries(${package_target} package_host_so)
    
    target_compile_definitions(${package_target} PRIVATE
        SQPACKAGE_VNAME="${cpkg_full_name}"
        SQPACKAGE_LOADFCT=load_${package_target}
    )
    
    if(NOT DEFINED SQ_DISABLE_INSTALLER)
        install(TARGETS ${package_target} LIBRARY DESTINATION ${INSTALL_LIB_DIR}/cpkgs/${cpkg_path_root})
    endif()
    
endfunction(define_package_so)

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}" CACHE PATH "")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")

project(sqtool C CXX)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(SQ_FLAGS -fno-exceptions -fno-strict-aliasing -Wall -Wextra -pedantic -Wcast-qual)

  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(SQ_FLAGS ${SQ_FLAGS} -O3)
  elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(SQ_FLAGS ${SQ_FLAGS} -O3 -g)
  elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(SQ_FLAGS ${SQ_FLAGS} -Os)
  elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(SQ_FLAGS ${SQ_FLAGS} -pg -pie -gstabs -g3 -Og)
  endif()

  if(CMAKE_VERSION VERSION_GREATER 3)
    add_compile_options(${SQ_FLAGS})
  else()
    add_definitions(${SQ_FLAGS})
  endif()

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -std=c++0x")
elseif(MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  add_definitions(-D_SQ64)
endif()

if(NOT DEFINED SQ_DISABLE_INSTALLER)
  if(NOT DEFINED INSTALL_BIN_DIR)
    set(INSTALL_BIN_DIR bin)
  endif()

  if(NOT DEFINED INSTALL_LIB_DIR)
    set(INSTALL_LIB_DIR lib)
  endif()

  if(NOT DEFINED INSTALL_INC_DIR)
    set(INSTALL_INC_DIR include)
  endif()

  if(NOT DEFINED INSTALL_CMAKE_DIR)
    set(INSTALL_CMAKE_DIR ${INSTALL_LIB_DIR}/cmake)
  endif()
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(squirrel)

add_library(package_host_so INTERFACE)
target_link_libraries(package_host_so INTERFACE squirrel_pkg sqstdlib_pkg)

set(CPKGS_ROOT_DIR ${CMAKE_SOURCE_DIR}/cpkgs)
MESSAGE( STATUS "CPKGS_ROOT_DIR:    " ${CPKGS_ROOT_DIR} )

add_subdirectory(cpkgs/serializer)

add_subdirectory(cpkgs/basictypes)

add_subdirectory(cpkgs/xml)
