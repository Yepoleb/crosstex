cmake_minimum_required(VERSION 3.1.0)
project(crosstex)

set(SOURCES
    src/BC1.cpp
    src/BC2.cpp
    src/BC3.cpp
    src/BC123_shared.cpp
    src/BC4.cpp
    src/BC5.cpp
    src/BC45_shared.cpp
    src/BC6H.cpp
    src/BC7.cpp
    src/BC67_shared.cpp)

option(BUILD_SHARED_LIBS "Build library as a shared object")
add_library(crosstex ${SOURCES})
target_include_directories(crosstex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/crosstex)
target_include_directories(crosstex INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
set_property(TARGET crosstex PROPERTY POSITION_INDEPENDENT_CODE True)

install(TARGETS crosstex EXPORT crosstexTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(DIRECTORY include/crosstex DESTINATION include)

export(EXPORT crosstexTargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/crosstex/crosstexTargets.cmake"
    NAMESPACE Upstream::
)
configure_file(cmake/crosstexConfig.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/crosstex/crosstexConfig.cmake"
    COPYONLY
)

set(ConfigPackageLocation lib/cmake/crosstex)
install(EXPORT crosstexTargets
    FILE crosstexTargets.cmake
    NAMESPACE Upstream::
    DESTINATION ${ConfigPackageLocation}
)
install(FILES cmake/crosstexConfig.cmake DESTINATION ${ConfigPackageLocation})
