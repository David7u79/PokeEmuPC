add_library(pocket_core STATIC
    include/pocket/core/IpcMessage.hpp
    include/pocket/core/IpcServer.hpp
    include/pocket/core/IpcClient.hpp
    include/pocket/core/RomFingerprint.hpp
    src/GameId.cpp
    src/GameSystem.cpp
    src/IpcServer.cpp
    src/IpcClient.cpp
    src/RomFingerprint.cpp
    ${CMAKE_SOURCE_DIR}/core/src/CompanionLink.cpp
)

target_include_directories(pocket_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/core/include
)

target_link_libraries(pocket_core PUBLIC
    Qt6::Core
    Qt6::Network
)
