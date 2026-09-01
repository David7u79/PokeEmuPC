add_library(PocketPartnerStorage STATIC
    src/DatabaseManager.cpp
)

target_include_directories(PocketPartnerStorage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PocketPartnerStorage PUBLIC
    Qt6::Core
    Qt6::Sql
)
