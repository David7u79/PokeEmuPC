add_library(pocket_input STATIC
    include/pocket/input/InputBinding.hpp
    include/pocket/input/ControllerControl.hpp
    include/pocket/input/ControllerLayout.hpp
    include/pocket/input/ControllerMapping.hpp
    src/InputBinding.cpp
    src/ControllerControl.cpp
    src/ControllerLayout.cpp
    src/ControllerMapping.cpp
)

target_include_directories(pocket_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(pocket_input PUBLIC Qt6::Core pocket_emulator_api)
