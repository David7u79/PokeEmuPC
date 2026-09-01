add_library(pocket_storage STATIC
    src/SchemaMigration.cpp
    src/GameRepository.cpp
)

target_include_directories(pocket_storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(pocket_storage PUBLIC
    Qt6::Core
    Qt6::Sql
    pocket_core
    PocketPartnerStorage
)
