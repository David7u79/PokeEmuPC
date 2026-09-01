add_library(pocket_save STATIC
    ${CMAKE_SOURCE_DIR}/save/src/SaveFileParser.cpp
    ${CMAKE_SOURCE_DIR}/save/src/SaveMutationPipeline.cpp
)

target_include_directories(pocket_save PUBLIC
    ${CMAKE_SOURCE_DIR}/save/include
)

target_link_libraries(pocket_save PUBLIC
    Qt6::Core
    pocket_core
)
