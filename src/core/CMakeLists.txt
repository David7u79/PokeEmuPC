add_library(pocket_core STATIC
    src/GameId.cpp
    src/GameSystem.cpp
    ${CMAKE_SOURCE_DIR}/core/src/CompanionLink.cpp
)

target_include_directories(pocket_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(pocket_core PUBLIC
    Qt6::Core
)
