add_library(max31855 STATIC max31855.c)
target_include_directories(max31855 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(max31855 PUBLIC stm32cubemx)
