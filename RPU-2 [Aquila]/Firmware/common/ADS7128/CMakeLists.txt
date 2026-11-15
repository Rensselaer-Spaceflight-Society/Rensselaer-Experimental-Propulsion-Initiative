
add_library(ads7128 STATIC ads7128.c)
target_include_directories(ads7128 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ads7128 PUBLIC stm32cubemx)
