add_library(pocket_companion_core STATIC
    ${CMAKE_SOURCE_DIR}/companion/src/CompanionManager.cpp
    ${CMAKE_SOURCE_DIR}/companion/src/ActivityConverter.cpp
)

target_include_directories(pocket_companion_core PUBLIC
    ${CMAKE_SOURCE_DIR}/companion/include
)

target_link_libraries(pocket_companion_core PUBLIC
    Qt6::Core
    pocket_core
    pocket_save
    pocket_storage
)
