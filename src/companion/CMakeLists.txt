add_library(pocket_companion_core STATIC
    include/pocket/companion/CompanionLink.hpp
    include/pocket/companion/CompanionState.hpp
    include/pocket/companion/CompanionSimulator.hpp
    include/pocket/companion/SpriteKey.hpp
    include/pocket/companion/CreatureSpriteProvider.hpp
    include/pocket/companion/PokeSpriteProvider.hpp
    include/pocket/companion/PkhexSpriteProvider.hpp
    include/pocket/companion/PlaceholderSpriteProvider.hpp
    include/pocket/companion/CompositeSpriteProvider.hpp
    include/pocket/companion/SpriteCache.hpp
    src/CompanionSimulator.cpp
    src/PokeSpriteProvider.cpp
    src/PkhexSpriteProvider.cpp
    src/PlaceholderSpriteProvider.cpp
    src/CompositeSpriteProvider.cpp
    src/SpriteCache.cpp
    ${CMAKE_SOURCE_DIR}/companion/src/CompanionManager.cpp
)

target_include_directories(pocket_companion_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/companion/include
    ${CMAKE_SOURCE_DIR}/core/include
    ${CMAKE_SOURCE_DIR}/storage/include
)

target_link_libraries(pocket_companion_core PUBLIC
    Qt6::Core
    Qt6::Gui
    pocket_core
    pocket_storage
)
