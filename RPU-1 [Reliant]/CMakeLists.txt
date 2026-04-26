cmake_minimum_required(VERSION 3.0)
project(ControlRoom)
find_package(ftxui 5 REQUIRED)
add_executable(ControlRoom src/control_room/main.cpp src/control_room/ui.cpp)
target_link_libraries(ControlRoom 
PRIVATE ftxui::screen 
PRIVATE ftxui::dom
PRIVATE ftxui::component)


