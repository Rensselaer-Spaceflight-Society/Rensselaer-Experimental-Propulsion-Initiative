add_library(hx711 STATIC hx711.c)
target_include_directories(hx711 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hx711 PUBLIC stm32cubemx)
