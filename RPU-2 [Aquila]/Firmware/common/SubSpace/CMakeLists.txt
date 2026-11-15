add_library(subspace STATIC subspace.c)
target_include_directories(subspace PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(subspace PUBLIC stm32cubemx)
