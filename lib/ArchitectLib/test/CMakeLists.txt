add_executable(ArchitectLibTest)

target_sources(ArchitectLibTest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tst_MockPlanner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tst_Protocol.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/tst_ProfileStore.cpp
)

target_link_libraries(ArchitectLibTest
  PRIVATE
    ArchitectLib
    Catch2::Catch2WithMain
    CompilerConfig
    PrecompileQtHeaders
    PrecompileStdHeaders
    Qt6::Core
)

include(Catch)
catch_discover_tests(ArchitectLibTest)
