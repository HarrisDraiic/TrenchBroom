add_library(ArchitectLib STATIC)

target_sources(ArchitectLib
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Error.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MockPlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Protocol.cpp

  PUBLIC FILE_SET headers TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/include/architect/Error.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/architect/MockPlanner.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/architect/Protocol.h
      ${CMAKE_CURRENT_SOURCE_DIR}/include/architect/RoomBlueprint.h
)

target_link_libraries(ArchitectLib
  PRIVATE
    CompilerConfig
    PrecompileQtHeaders
    PrecompileStdHeaders
  PUBLIC
    Qt6::Core
)

add_subdirectory(test)
