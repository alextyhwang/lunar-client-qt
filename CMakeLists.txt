cmake_minimum_required(VERSION 3.21)
if(WIN32)
    project(atw-client LANGUAGES CXX RC)
else()
    project(atw-client LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

option(INCLUDE_UPDATER "Include update checker" OFF)

set(QT_COMPONENTS Core Gui Widgets Svg)


find_package(Qt6 COMPONENTS
        ${QT_COMPONENTS}
        )
if (NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS
            ${QT_COMPONENTS}
            REQUIRED
            )
    set(QT_VER Qt5)
else()
set(QT_VER Qt6)
endif()

# ---------------------------- Get current version and tag --------------------------------
set(LCQT_VERSION 0.0.1)
set(LCQT_VERSION_TAG v0.0.1)

# Version is set above; git tags can override if desired
if(Git_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --match "v[0-9]*"
            OUTPUT_VARIABLE tag
            OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
    if(tag AND tag MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)")
        set(LCQT_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3})
        if(tag MATCHES "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)")
            set(LCQT_VERSION ${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}.${CMAKE_MATCH_4})
        endif()
        set(LCQT_VERSION_TAG v${LCQT_VERSION})
        message(STATUS "Version from git tag: ${LCQT_VERSION_TAG}")
    endif()
endif()
message(STATUS "ATW Client version: ${LCQT_VERSION_TAG}")

# ----------------------- Parse json config file -----------------------------

file(READ ${CMAKE_CURRENT_LIST_DIR}/config.json CONFIG)

macro(CONFIG_VAR KEY OUTPUT_VAR)
    string(JSON ${OUTPUT_VAR} GET ${CONFIG} ${KEY})
endmacro()

config_var("agentsDestDir" AGENTS_DEST_DIR)
config_var("libsDestDir" LIBS_DEST_DIR)
config_var("githubReleaseApiLink" GITHUB_RELEASE_LINK)

# ----------------------- Replace values in cpp file -----------------------------

configure_file(${CMAKE_SOURCE_DIR}/buildconfig.cpp.in ${CMAKE_BINARY_DIR}/buildconfig.cpp @ONLY)
set(VERSION_FILE ${CMAKE_BINARY_DIR}/buildconfig.cpp)

set_source_files_properties(${VERSION_FILE} PROPERTIES COMPILE_DEFINITIONS "QSL=QStringLiteral")

# ----------------------- Sources ------------------------

set(SOURCE_FILES
        src/main.cpp
        src/gui/mainwindow.cpp
        src/gui/mainwindow.h
        src/gui/pages/logspage.cpp
        src/gui/pages/logspage.h
        src/launch/offlinelauncher.cpp
        src/launch/offlinelauncher.h
        src/gui/pages/gamepage.cpp
        src/gui/pages/gamepage.h
        src/gui/pages/configurationpage.h
        src/gui/pages/agentspage.cpp
        src/gui/pages/agentspage.h
        src/gui/pages/modspage.cpp
        src/gui/pages/modspage.h
        src/gui/pages/helperspage.cpp
        src/gui/pages/helperspage.h
        src/launch/launcher.h
        src/launch/launcher.cpp
        src/config/config.cpp
        src/config/config.h
        src/gui/widgets/filechooser.cpp
        src/gui/widgets/filechooser.h
        src/gui/widgets/widgetutils.cpp
        src/gui/widgets/widgetutils.h
        src/buildconfig.h
        src/gui/agents/agentsmodel.cpp
        src/gui/agents/agentsmodel.h
        src/gui/agents/agentsview.cpp
        src/gui/agents/agentsview.h
        src/gui/mods/modsmodel.cpp
        src/gui/mods/modsmodel.h
        src/gui/mods/modsview.cpp
        src/gui/mods/modsview.h
        src/gui/style/removeoutline.cpp
        src/gui/style/removeoutline.h
        src/util/fs.cpp src/util/fs.h
        src/util/utils.cpp
        src/util/utils.h
)

set(UPDATER_SOURCES src/updater/updatechecker.cpp src/updater/updatechecker.h)

if(INCLUDE_UPDATER)
    list(APPEND SOURCE_FILES ${UPDATER_SOURCES})
endif()

# ------------------------- Dependencies --------------------------

set(QT_COMPONENTS Core Gui Widgets Svg)

if(INCLUDE_UPDATER)
    list(APPEND QT_COMPONENTS Network)
endif()

find_package(${QT_VER} COMPONENTS ${QT_COMPONENTS} REQUIRED)


# ----------------------------- Executable --------------------------------------

add_executable(atw-client WIN32 MACOSX_BUNDLE ${SOURCE_FILES} ${VERSION_FILE} res.qrc)

# ----------------------------- Compile Definitions --------------------------------

if(INCLUDE_UPDATER)
    target_compile_definitions(atw-client PRIVATE INCLUDE_UPDATER)
endif()

# ------------------------- Copy java agents and libraries -----------------------

if(UNIX AND APPLE)
    set(JAR_DEST_DIR ${CMAKE_BINARY_DIR}/atw-client.app/Contents/MacOS)
else()
    set(JAR_DEST_DIR ${CMAKE_BINARY_DIR})
endif()

macro(COPY_JARS S_DIR T_DIR NEW_DIR)
    add_custom_command(
            OUTPUT "${JAR_DEST_DIR}/${T_DIR}"
            COMMAND ${CMAKE_COMMAND}
            -Dsrc_dir=${CMAKE_CURRENT_LIST_DIR}/java/${S_DIR}
            -Ddst_dir=${JAR_DEST_DIR}/${T_DIR}
            -P ${CMAKE_CURRENT_LIST_DIR}/cmake/copy_jars.cmake
            COMMENT "Copying '${S_DIR}' to '${T_DIR}'"
            VERBATIM
    )
    set(${NEW_DIR} "${JAR_DEST_DIR}/${T_DIR}")
endmacro()

copy_jars(agents ${AGENTS_DEST_DIR} COPIED_AGENTS)
copy_jars(libs ${LIBS_DEST_DIR} COPIED_LIBRARIES)

add_custom_target(CopyJars ALL DEPENDS ${COPIED_AGENTS} ${COPIED_LIBRARIES})
add_dependencies(atw-client CopyJars)

# ----------------------- Copy application icon ------------------------------

configure_file(${CMAKE_SOURCE_DIR}/res/icon.ico ${CMAKE_BINARY_DIR}/icon.ico COPYONLY)

# Embed icon in Windows executable (for taskbar pin, Explorer, etc.)
if(WIN32)
    target_sources(atw-client PRIVATE ${CMAKE_SOURCE_DIR}/res/atw-client.rc)
endif()

# ----------------------- Link against libraries ------------------------------

list(TRANSFORM QT_COMPONENTS PREPEND ${QT_VER}:: OUTPUT_VARIABLE QT_LIBS)

target_link_libraries(atw-client ${QT_LIBS})

# ---------------------- Set include directory ---------------------------

target_include_directories(atw-client PRIVATE src)

# ---------------------- Deploy Qt DLLs (Windows) ---------------------------

if(WIN32 AND QT_VER STREQUAL "Qt6")
    get_filename_component(QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
    add_custom_command(TARGET atw-client POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe" --no-translations "$<TARGET_FILE:atw-client>"
        COMMENT "Deploying Qt DLLs with windeployqt..."
    )
elseif(WIN32 AND QT_VER STREQUAL "Qt5")
    get_filename_component(QT_BIN_DIR "${Qt5_DIR}/../../../bin" ABSOLUTE)
    add_custom_command(TARGET atw-client POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe" --no-translations "$<TARGET_FILE:atw-client>"
        COMMENT "Deploying Qt DLLs with windeployqt..."
    )
endif()
