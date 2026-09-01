add_library(pocket_emulator_api STATIC
    src/EngineResolver.cpp
)

target_include_directories(pocket_emulator_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pocket_emulator_api PUBLIC
    pocket_core
    PocketPartnerEmulator
)
