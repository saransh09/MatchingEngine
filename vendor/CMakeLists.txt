# GoogleTest
if (ENABLE_TESTING)
    message(STATUS "Including and building Gtest for testing.")
    add_subdirectory(google_test)
endif ()

# GoogleBenchmark
if (ENABLE_BENCHMARKING)
    message(STATUS "Including and building google-benchmark for benchmarking.")
    add_subdirectory(google_benchmark)
endif ()

# Quill
if (ENABLE_LOGGING)
    set(QUILL_NO_EXCEPTIONS ON)
    add_subdirectory(quill)
    if (CMAKE_BUILD_TYPE IN_LIST DEBUG_BUILDS)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQUILL_COMPLETE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_TRACE_L3")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQUILL_COMPLETE_ACTIVE_LOG_LEVEL=QUILL_COMPLETE_ACTIVE_LOG_LEVEL_WARNING")
    endif()
endif()
