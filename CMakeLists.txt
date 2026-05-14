cmake_minimum_required(VERSION 3.21)
project(flashcards LANGUAGES CXX VERSION 0.3.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Emit compile_commands.json so VS Code / clangd find Qt headers.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Pdf
    PdfWidgets
    PrintSupport
)

set(APP_DOMAIN_SOURCES
    src/domain/Card.h
    src/domain/CardDeck.h
    src/domain/CardDeck.cpp
    src/domain/CsvLoader.h
    src/domain/CsvLoader.cpp
)

set(APP_RENDER_SOURCES
    src/render/FontProvider.h
    src/render/FontProvider.cpp
    src/render/PdfGenerator.h
    src/render/PdfGenerator.cpp
)

set(APP_UI_SOURCES
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/CardEditorDialog.h
    src/ui/CardEditorDialog.cpp
)

set(APP_APP_SOURCES
    src/app/FlashcardsController.h
    src/app/FlashcardsController.cpp
    src/app/PrintController.h
    src/app/PrintController.cpp
)

# Domain + render + app layers as a static lib so tests can link against
# them without dragging in Qt Widgets / main().
add_library(flashcards_core STATIC
    ${APP_DOMAIN_SOURCES}
    ${APP_RENDER_SOURCES}
    ${APP_APP_SOURCES}
)
target_include_directories(flashcards_core PUBLIC src)
target_link_libraries(flashcards_core PUBLIC
    Qt6::Core
    Qt6::Gui          # QPdfWriter, QFontDatabase, QPainter, QTextDocument
    Qt6::Pdf          # QPdfDocument owned by FlashcardsController
    Qt6::PrintSupport # QPrinter / QPrintDialog used by PrintController
    Qt6::Widgets      # QPrintDialog inherits QWidget
)

# Bundled CJK font: required at build time, embedded into the binary so the
# release artefact has no runtime font dependency. Use the glyf-based TTF:
# CFF/OTF embedding is unreliable on Qt-Windows PDF builds.
set(_FLASHCARDS_FONT_DIR "${CMAKE_SOURCE_DIR}/resources/fonts")
set(FLASHCARDS_BUNDLED_FONT_FILE "NotoSansJP-Regular.ttf")
set(_FLASHCARDS_FONT_SOURCE_REGULAR "${_FLASHCARDS_FONT_DIR}/${FLASHCARDS_BUNDLED_FONT_FILE}")
set(_FLASHCARDS_FONT_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/downloaded_resources/fonts")
set(_FLASHCARDS_FONT_GENERATED_REGULAR "${_FLASHCARDS_FONT_DOWNLOAD_DIR}/${FLASHCARDS_BUNDLED_FONT_FILE}")
set(_FLASHCARDS_FONT_VARIABLE_TTF "${_FLASHCARDS_FONT_DOWNLOAD_DIR}/NotoSansJP-VF.ttf")
set(_FLASHCARDS_FONT_VARIABLE_URL
    "https://github.com/notofonts/noto-cjk/raw/main/Sans/Variable/TTF/Subset/NotoSansJP-VF.ttf"
)

if(EXISTS "${_FLASHCARDS_FONT_SOURCE_REGULAR}")
    set(_FLASHCARDS_FONT_INPUT "${_FLASHCARDS_FONT_SOURCE_REGULAR}")
else()
    file(MAKE_DIRECTORY "${_FLASHCARDS_FONT_DOWNLOAD_DIR}")

    if(NOT EXISTS "${_FLASHCARDS_FONT_GENERATED_REGULAR}")
        if(NOT EXISTS "${_FLASHCARDS_FONT_VARIABLE_TTF}")
            message(STATUS "Downloading bundled font source: ${_FLASHCARDS_FONT_VARIABLE_URL}")
            file(DOWNLOAD
                "${_FLASHCARDS_FONT_VARIABLE_URL}"
                "${_FLASHCARDS_FONT_VARIABLE_TTF}"
                STATUS _FLASHCARDS_FONT_DOWNLOAD_STATUS
                SHOW_PROGRESS
                TLS_VERIFY ON
            )
            list(GET _FLASHCARDS_FONT_DOWNLOAD_STATUS 0 _FLASHCARDS_FONT_DOWNLOAD_CODE)
            list(GET _FLASHCARDS_FONT_DOWNLOAD_STATUS 1 _FLASHCARDS_FONT_DOWNLOAD_MESSAGE)
            if(NOT _FLASHCARDS_FONT_DOWNLOAD_CODE EQUAL 0)
                message(FATAL_ERROR
                    "Failed to download Noto Sans JP TTF:\n"
                    "  ${_FLASHCARDS_FONT_DOWNLOAD_MESSAGE}\n"
                    "URL:\n"
                    "  ${_FLASHCARDS_FONT_VARIABLE_URL}"
                )
            endif()
        endif()

        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        execute_process(
            COMMAND
                "${Python3_EXECUTABLE}" -m fontTools.varLib.instancer
                "${_FLASHCARDS_FONT_VARIABLE_TTF}"
                wght=400
                --static
                --update-name-table
                --output "${_FLASHCARDS_FONT_GENERATED_REGULAR}"
            RESULT_VARIABLE _FLASHCARDS_FONTTOOLS_RESULT
            OUTPUT_VARIABLE _FLASHCARDS_FONTTOOLS_OUTPUT
            ERROR_VARIABLE _FLASHCARDS_FONTTOOLS_ERROR
        )
        if(NOT _FLASHCARDS_FONTTOOLS_RESULT EQUAL 0)
            message(FATAL_ERROR
                "Failed to generate NotoSansJP-Regular.ttf from the downloaded VF TTF.\n"
                "Install Python fonttools, then rerun CMake:\n"
                "  python -m pip install fonttools\n\n"
                "fonttools stdout:\n${_FLASHCARDS_FONTTOOLS_OUTPUT}\n"
                "fonttools stderr:\n${_FLASHCARDS_FONTTOOLS_ERROR}"
            )
        endif()
    endif()

    set(_FLASHCARDS_FONT_INPUT "${_FLASHCARDS_FONT_GENERATED_REGULAR}")
endif()
message(STATUS "Bundled font: ${FLASHCARDS_BUNDLED_FONT_FILE} (${_FLASHCARDS_FONT_INPUT})")

function(flashcards_add_bundled_font_resource target)
    qt_add_resources(${target} "bundled_font"
        PREFIX "/fonts"
        BASE "${CMAKE_BINARY_DIR}/staged_resources/fonts"
        FILES "${_FLASHCARDS_FONT_STAGED}"
    )
endfunction()

set(_FLASHCARDS_FONT_STAGED "${CMAKE_BINARY_DIR}/staged_resources/fonts/${FLASHCARDS_BUNDLED_FONT_FILE}")
configure_file(
    "${_FLASHCARDS_FONT_INPUT}"
    "${_FLASHCARDS_FONT_STAGED}"
    COPYONLY
)

qt_add_executable(flashcards
    src/main.cpp
    ${APP_UI_SOURCES}
)
flashcards_add_bundled_font_resource(flashcards)

string(TIMESTAMP JP_CARDS_BUILD_DATE "%Y-%m-%d %H:%M" UTC)
target_compile_definitions(flashcards PRIVATE
    JP_CARDS_VERSION="${PROJECT_VERSION}"
    JP_CARDS_BUILD_DATE="${JP_CARDS_BUILD_DATE} UTC"
)

target_include_directories(flashcards PRIVATE src)

target_link_libraries(flashcards PRIVATE
    flashcards_core
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Pdf
    Qt6::PdfWidgets
    Qt6::PrintSupport
)

set_target_properties(flashcards PROPERTIES
    OUTPUT_NAME jp-cards
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE OFF
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
