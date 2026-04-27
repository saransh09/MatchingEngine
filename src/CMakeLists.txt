ADD_LIBRARY(core OBJECT
    core/OrderBook.cpp
    core/MatchingEngine.cpp
    core/OrderGenerator.cpp
)
TARGET_INCLUDE_DIRECTORIES(core INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
)
TARGET_LINK_LIBRARIES(core INTERFACE project_warnings)
TARGET_COMPILE_OPTIONS(core PRIVATE -flto)
