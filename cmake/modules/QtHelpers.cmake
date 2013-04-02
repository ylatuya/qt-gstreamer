# - Try to find an installed version of Qt based on QT_VERSION
# Once done this will define
#
#  USE_QT4 - Building with Qt4
#  USE_QT5 - Building with Qt5
#  QT_MIN_VERSION - qt minimum
#
# Copyright (c) 2013, Fluendo S.L. <support@fluendo.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (QT_VERSION EQUAL 5)
  set(USE_QT5 TRUE)
  set(QT_MIN_VERSION 5.0.1)
  message(STATUS "Using Qt5 (min: ${QT_MIN_VERSION})")
elseif (QT_VERSION EQUAL 4)
  set(USE_QT4 TRUE)
  set(QT_MIN_VERSION 4.7)
  message(STATUS "Using Qt4 (min:${QT_MIN_VERSION})")
endif()

if (USE_QT5)
    find_package(Qt5Core REQUIRED)
    find_package(Qt5Gui REQUIRED)
    find_package(Qt5Declarative)
    if (Qt5Declarative_FOUND)
        set(QT_QTDECLARATIVE_FOUND true)
    endif()
    find_package(Qt5OpenGL)
    if (Qt5OpenGL_FOUND)
        set(QT_QTOPENGL_FOUND true)
        set(QT_QTOPENGL_LIBRARIES ${Qt5OpenGL_LIBRARIES})
        set(QT_QTOPENGL_INCLUDE_DIRS ${Qt5OpenGL_INCLUDE_DIRS})
    endif()
    set(QT_FOUND true)
    set(QT_QTCORE_LIBRARIES ${Qt5Core_LIBRARIES})
    set(QT_QTCORE_INCLUDE_DIRS ${Qt5Core_INCLUDE_DIRS})
    set(QT_QTGUI_LIBRARIES ${Qt5Gui_LIBRARIES})
    set(QT_QTGUI_INCLUDE_DIRS ${Qt5Gui_INCLUDE_DIRS})
    set(QT_QTWIDGETS_LIBRARIES ${Qt5Widgets_LIBRARIES})
    set(QT_QTWIDGETS_INCLUDE_DIRS ${Qt5Widgets_INCLUDE_DIRS})
elseif (USE_QT4)
    find_package(Qt4 COMPONENTS QtCore QtGui)
    macro_log_feature(QT4_FOUND "Qt 4" "Required for building everything" "http://qt.nokia.com/" TRUE "${QT_MIN_VERSION}")
    if (QT4_FOUND)
      set(QT_FOUND true)
    endif()
else()
    message(FATAL_ERROR "Qt supported versions are: 4 and 5")
endif()

# qt_helper_wrap_ui(outfiles inputfile ... )
function(QT_HELPER_WRAP_UI outfiles )
    if (USE_QT5)
        qt5_wrap_ui (outfiles )
    elseif (USE_QT4)
        qt4_wrap_ui (outfiles )
    endif()
endfunction()

# qt_helper_add_resources(outfiles inputfile ... )
function(QT_HELPER_ADD_RESOURCES outfiles )
    if (USE_QT5)
        qt5_add_resources (outfiles )
    elseif (USE_QT4)
        qt4_add_resources (outfiles )
    endif()
endfunction()
