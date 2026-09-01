add_library(pocket_save STATIC
    include/pocket/save/Creature.hpp
    include/pocket/save/CreatureSaveParser.hpp
    include/pocket/save/Gen3SaveParser.hpp
    include/pocket/save/CompanionReidentifier.hpp
    include/pocket/save/PendingGameReward.hpp
    include/pocket/save/SaveSessionState.hpp
    include/pocket/save/SaveSessionCoordinator.hpp
    include/pocket/save/FileStabilityVerifier.hpp
    include/pocket/save/SaveBackupRepository.hpp
    src/Gen3SaveParser.cpp
    src/CompanionReidentifier.cpp
    src/PendingGameReward.cpp
    src/SaveSessionCoordinator.cpp
    src/SaveBackupRepository.cpp
    ${CMAKE_SOURCE_DIR}/save/src/SaveFileParser.cpp
    ${CMAKE_SOURCE_DIR}/save/src/SaveMutationPipeline.cpp
)

target_include_directories(pocket_save PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/save/include
    ${CMAKE_SOURCE_DIR}/storage/include
    ${CMAKE_SOURCE_DIR}/companion/include
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(pocket_save PUBLIC
    Qt6::Core
    pocket_core
    pocket_storage
    pocket_companion_core
)
