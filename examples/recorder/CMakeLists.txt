project(qtgst-example-recorder)

if (NOT BUILDING_QTGSTREAMER)
    find_package(QtGStreamer REQUIRED)

    if (${CMAKE_VERSION} VERSION_LESS "2.8.6")
        find_package(Automoc4 REQUIRED)
    else()
        set(CMAKE_AUTOMOC TRUE)

        macro(automoc4_add_executable)
            add_executable(${ARGV})
        endmacro()
    endif()
endif()

include_directories(${QTGSTREAMER_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR} ${QT_QTGUI_INCLUDE_DIRS})
add_definitions(${QTGSTREAMER_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QTGSTREAMER_FLAGS}")

if (USE_QT5)
qt5_wrap_ui(recorder_UI_SRCS recorder.ui)
else()
qt4_wrap_ui(recorder_UI_SRCS recorder.ui)
endif()
automoc4_add_executable(recorder main.cpp ${recorder_UI_SRCS})
target_link_libraries(recorder ${QTGSTREAMER_LIBRARIES} ${QT_QTGUI_LIBRARIES} ${QT_QTWIDGETS_LIBRARIES})
