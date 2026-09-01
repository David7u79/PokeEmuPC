add_library(PocketPartnerCompanion STATIC
    src/CompanionManager.cpp
    src/ActivityConverter.cpp
)

target_include_directories(PocketPartnerCompanion PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PocketPartnerCompanion PUBLIC
    Qt6::Core
    PocketPartnerCore
    PocketPartnerSave
    PocketPartnerStorage
)
