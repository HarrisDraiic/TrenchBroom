add_executable(ArchitectRuntime)

target_sources(ArchitectRuntime PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
)

target_link_libraries(ArchitectRuntime
  PRIVATE
    ArchitectLib
    CompilerConfig
    PrecompileQtHeaders
    PrecompileStdHeaders
    Qt6::Core
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD")
  set_target_properties(
    ArchitectRuntime PROPERTIES OUTPUT_NAME "${TB_ARCHITECT_RUNTIME_LINUX_EXECUTABLE_NAME}")
else()
  set_target_properties(
    ArchitectRuntime PROPERTIES OUTPUT_NAME "${TB_ARCHITECT_RUNTIME_EXECUTABLE_NAME}")
endif()
