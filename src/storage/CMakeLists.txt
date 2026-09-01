add_library(pocket_storage STATIC
    include/pocket/storage/GameRepository.hpp
    include/pocket/storage/GameMetadataResolver.hpp
    include/pocket/storage/ArtworkProvider.hpp
    include/pocket/storage/ArtworkCache.hpp
    include/pocket/storage/LibretroArtworkProvider.hpp
    src/SchemaMigration.cpp
    src/GameRepository.cpp
    src/GameMetadataResolver.cpp
    src/ArtworkCache.cpp
    src/LibretroArtworkProvider.cpp
)

target_include_directories(pocket_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/storage/include
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(pocket_storage PUBLIC
    Qt6::Core
    Qt6::Sql
    Qt6::Network
    Qt6::Gui
    pocket_core
    PocketPartnerStorage
)
