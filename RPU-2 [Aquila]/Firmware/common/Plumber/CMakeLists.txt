add_library(plumber STATIC plumber.c)
target_include_directories(plumber PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(plumber PUBLIC stm32cubemx)
