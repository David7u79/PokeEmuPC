add_library(pocket_emulator_api STATIC
    include/pocket/emulator/Ilibretro.h
    include/pocket/emulator/PersistentGameSave.hpp
    include/pocket/emulator/SaveState.hpp
    include/pocket/emulator/EmulatorEngine.hpp
    include/pocket/emulator/MgbaEngine.hpp
    include/pocket/emulator/EngineResolver.hpp
    src/MgbaEngine.cpp
    src/EngineResolver.cpp
)

target_include_directories(pocket_emulator_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/emulator/include
    ${CMAKE_SOURCE_DIR}/src/core/include
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(pocket_emulator_api PUBLIC
    Qt6::Core
    pocket_core
)
