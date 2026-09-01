add_library(PocketPartnerCore STATIC
    src/CompanionLink.cpp
)

target_include_directories(PocketPartnerCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PocketPartnerCore PUBLIC
    Qt6::Core
    PocketPartnerStorage
)
