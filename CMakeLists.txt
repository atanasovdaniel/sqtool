if(MSVC)
  cmake_minimum_required(VERSION 3.4)
else()
  cmake_minimum_required(VERSION 2.8)
endif()

set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "")

include(etc/sqpackage.cmake)

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

add_library(package_host_so INTERFACE)
target_link_libraries(package_host_so INTERFACE squirrel_pkg sqstdlib_pkg)

add_library(package_host_lib INTERFACE)
# target_link_libraries(package_host_lib INTERFACE squirrel_ifc sqstdlib_ifc)
# target_link_libraries(package_host_lib INTERFACE squirrel sqstdlib)
target_include_directories(package_host_lib
    INTERFACE
        $<TARGET_PROPERTY:squirrel,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:sqstdlib,INTERFACE_INCLUDE_DIRECTORIES>
)

install(TARGETS package_host_so package_host_lib EXPORT squirrel)

#add_library(sq_meta_pkg_root INTERFACE)
#set(sqmetapkg_target sq_meta_pkg_root)

#set(CPKGS_ROOT_DIR ${CMAKE_SOURCE_DIR}/cpkgs)
#MESSAGE( STATUS "CPKGS_ROOT_DIR:    " ${CPKGS_ROOT_DIR} )

add_sqmetaroot(sq_builtin_packages)
set(SQ_STDLIB_EXTRA_LIBS sq_builtin_packages)

add_subdirectory(cpkgs)

get_property(qaz TARGET sq_builtin_packages PROPERTY QAZ)
MESSAGE( STATUS "sq_builtin_packages: ${qaz}")
target_link_libraries(sq_builtin_packages PUBLIC ${qaz})

add_subdirectory(squirrel)

