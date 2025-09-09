cmake_minimum_required(VERSION 3.12)
project(SimpleMapResolver)

set(HFS "/opt/hfs21.0" CACHE PATH "Houdini install prefix")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Find system Python 3.11 dev headers
find_package(Python3 3.11 COMPONENTS Development REQUIRED)

include_directories(
  "${HFS}/toolkit/include"
  ${Python3_INCLUDE_DIRS}              # <- adds /usr/include/python3.11
)

link_directories("${HFS}/dsolib")

add_library(SimpleMapResolver SHARED SimpleMapResolver.cpp)
target_link_libraries(SimpleMapResolver pxr_arch pxr_tf pxr_ar)

set_target_properties(SimpleMapResolver PROPERTIES
  BUILD_WITH_INSTALL_RPATH ON
  INSTALL_RPATH "${HFS}/dsolib"
)
install(TARGETS SimpleMapResolver LIBRARY DESTINATION ".")
install(FILES plugInfo.json DESTINATION ".")
