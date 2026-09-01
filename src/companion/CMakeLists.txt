add_library(pocket_companion_core STATIC
    include/pocket/companion/IClock.hpp
    include/pocket/companion/CompanionBond.hpp
    include/pocket/companion/GameFriendship.hpp
    include/pocket/companion/CompanionState.hpp
    include/pocket/companion/CompanionCommands.hpp
    include/pocket/companion/CompanionSimulator.hpp
    src/CompanionSimulator.cpp
    ${CMAKE_SOURCE_DIR}/companion/src/ActivityConverter.cpp
    ${CMAKE_SOURCE_DIR}/companion/src/CompanionManager.cpp
)

target_include_directories(pocket_companion_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/companion/include
    ${CMAKE_SOURCE_DIR}/storage/include
    ${CMAKE_SOURCE_DIR}/save/include
)

target_link_libraries(pocket_companion_core PUBLIC
    Qt6::Core
    pocket_core
    pocket_storage
    pocket_save
)
