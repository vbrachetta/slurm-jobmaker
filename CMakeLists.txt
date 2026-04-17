cmake_minimum_required(VERSION 3.16)
project(SlurmJobMaker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(SlurmJobMaker main.cpp)
target_link_libraries(SlurmJobMaker PRIVATE Qt6::Widgets)

# Binary
install(TARGETS SlurmJobMaker DESTINATION bin)

# Icons
install(FILES assets/icons/16x16/slurm-jobmaker.png
        DESTINATION share/icons/hicolor/16x16/apps)
install(FILES assets/icons/32x32/slurm-jobmaker.png
        DESTINATION share/icons/hicolor/32x32/apps)
install(FILES assets/icons/48x48/slurm-jobmaker.png
        DESTINATION share/icons/hicolor/48x48/apps)
install(FILES assets/icons/128x128/slurm-jobmaker.png
        DESTINATION share/icons/hicolor/128x128/apps)
install(FILES assets/icons/256x256/slurm-jobmaker.png
        DESTINATION share/icons/hicolor/256x256/apps)

# Desktop entry
install(FILES packaging/debian/slurm-jobmaker.desktop
        DESTINATION share/applications)
