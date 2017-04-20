cmake_minimum_required(VERSION 3.0.0)
#cmake_policy(SET CMP0063 NEW)
set(QT_REQUIRED_VERSION "5.6.0")
set(KF5_REQUIRED_VERSION "5.16.0")

#requires okular-dev extra-cmake-modules libkf5coreaddons-dev libkf5xmlgui-dev libkf5config-dev

#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

message(STATUS "MODULES:${CMAKE_MODULE_PATH}")


#find_package(Poppler REQUIRED)

#find_package(Qt4 REQUIRED)
#find_package(KDE4 REQUIRED)

find_package(ECM 5.19.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMAddAppIcon)
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)

include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(WriteBasicConfigVersionFile)
include(CheckFunctionExists)

include(KDEInstallDirs)
include(KDECMakeSettings)

kde_enable_exceptions()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules)

#find_package(ECM 5.19.0 CONFIG REQUIRED)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Core DBus Test Widgets PrintSupport Svg Qml Quick)
find_package(KF5 REQUIRED COMPONENTS Config CoreAddons XmlGui)
find_package(Boost COMPONENTS regex REQUIRED)
find_package(Okular5 COMPONENTS core REQUIRED)

#include_directories(${POPPLER_INCLUDE_DIRS} ${QT_INCLUDE_DIR})
#include(KDE4Defaults)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

#include_directories("/usr/include/okular" ${QT_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR} "/usr/include/KF5/KConfigCore/" ${Qt5Core_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} "/usr/include/okular" "/usr/include/okular/core")

#set(CMAKE_CXX_FLAGS "-pthread -std=c++11 -O2 -Wall -Wextra")
#set(CMAKE_CXX_FLAGS "-pthread -std=c++11 -O3 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "-pthread -std=c++11 -g -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} -pthread -std=c++11 -g -Wall -Wextra")

add_definitions(${Qt5Core_DEFINITIONS})
add_definitions(${Qt5Widgets_DEFINITIONS})
#add_definitions()

#MESSAGE( STATUS "Inlcudes: " ${GLIB2_INCLUDE_DIR} )

#get_cmake_property(_variableNames VARIABLES)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()

add_subdirectory(src)
