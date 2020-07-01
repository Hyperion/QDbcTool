find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)
find_package(Qt5 COMPONENTS Gui REQUIRED)

add_executable(kdbctool
    Alphanum.cpp
    DTObject.cpp
    MainForm.cpp
    export_sql.cpp
    export_csv.cpp
    export_json.cpp
    DTBuild.ui
    MainForm.ui
    AboutForm.ui
    Main.cpp
    DbcTool.qrc
)

target_link_libraries(kdbctool Qt5::Widgets)
target_link_libraries(kdbctool Qt5::Core)
target_link_libraries(kdbctool Qt5::Gui)