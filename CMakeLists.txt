cmake_minimum_required(VERSION 3.21)
project(flashcards LANGUAGES CXX VERSION 0.2.8)

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
set(_FLASHCARDS_FONT_REGULAR_TTF "${_FLASHCARDS_FONT_DIR}/NotoSansJP-Regular.ttf")
if(EXISTS "${_FLASHCARDS_FONT_REGULAR_TTF}")
    set(FLASHCARDS_BUNDLED_FONT_FILE "NotoSansJP-Regular.ttf")
else()
    message(FATAL_ERROR
        "\nRequired font file is missing:\n"
        "  ${_FLASHCARDS_FONT_REGULAR_TTF}\n\n"
        "Download the static family from:\n"
        "  https://fonts.google.com/noto/specimen/Noto+Sans+JP\n\n"
        "Then place NotoSansJP-Regular.ttf into resources/fonts/.\n"
        "Do not use the OTF build for release PDFs: Qt-Windows may outline\n"
        "glyphs instead of embedding a font subset.\n"
    )
endif()
message(STATUS "Bundled font: ${FLASHCARDS_BUNDLED_FONT_FILE}")

function(flashcards_add_bundled_font_resource target)
    qt_add_resources(${target} "bundled_font"
        PREFIX "/fonts"
        BASE "${CMAKE_BINARY_DIR}/staged_resources/fonts"
        FILES "${_FLASHCARDS_FONT_STAGED}"
    )
endfunction()

set(_FLASHCARDS_FONT_STAGED "${CMAKE_BINARY_DIR}/staged_resources/fonts/${FLASHCARDS_BUNDLED_FONT_FILE}")
configure_file(
    "${_FLASHCARDS_FONT_DIR}/${FLASHCARDS_BUNDLED_FONT_FILE}"
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
