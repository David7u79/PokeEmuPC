add_library(pocket_emulator_api STATIC
    include/pocket/emulator/EmulatorEngine.hpp
    include/pocket/emulator/LibretroEngineBase.hpp
    include/pocket/emulator/MgbaEngine.hpp
    include/pocket/emulator/MelonDsEngine.hpp
    src/LibretroEngineBase.cpp
    src/MgbaEngine.cpp
    src/MelonDsEngine.cpp
    src/LibretroCoreSession.cpp
    src/AudioRingBuffer.cpp
)

target_include_directories(pocket_emulator_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/emulator/include
    ${CMAKE_SOURCE_DIR}/save/include
    ${CMAKE_SOURCE_DIR}/storage/include
    ${CMAKE_SOURCE_DIR}/companion/include
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(pocket_emulator_api PUBLIC
    Qt6::Core
    pocket_save
    pocket_core
)

if (WIN32)
    target_link_libraries(pocket_emulator_api PUBLIC winmm)
endif()
