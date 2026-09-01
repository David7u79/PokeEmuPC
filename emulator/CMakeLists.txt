add_library(PocketPartnerEmulator STATIC
    src/NullEmulatorEngine.cpp
)

target_include_directories(PocketPartnerEmulator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(PocketPartnerEmulator PUBLIC
    Qt6::Core
    PocketPartnerCore
)
