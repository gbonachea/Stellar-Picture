cmake_minimum_required(VERSION 3.16)
project(StellarPicture VERSION 1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)
add_executable(stellar-picture
    src/main.cpp src/mainwindow.cpp src/imageview.cpp
    icons/icons.qrc)
target_link_libraries(stellar-picture PRIVATE Qt6::Widgets Qt6::Svg)
install(TARGETS stellar-picture RUNTIME DESTINATION bin)
install(FILES resources/stellar-picture.desktop DESTINATION share/applications)
install(FILES icons/stellarpicture.png DESTINATION share/icons/hicolor/512x512/apps RENAME stellar-picture.png)
