add_library(PocketPartnerSave STATIC
    src/SaveFileParser.cpp
    src/SaveMutationPipeline.cpp
)

target_include_directories(PocketPartnerSave PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PocketPartnerSave PUBLIC
    Qt6::Core
    PocketPartnerCore
)
