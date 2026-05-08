find_package(Qt6 REQUIRED COMPONENTS Test Pdf)

function(flashcards_add_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE flashcards_core Qt6::Test ${ARGN})
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endfunction()

flashcards_add_test(test_csv_loader)
flashcards_add_test(test_card_deck)
flashcards_add_test(test_pdf_generator Qt6::Pdf)
flashcards_add_test(test_flashcards_controller Qt6::Pdf)
flashcards_add_test(test_print_controller Qt6::Pdf Qt6::PrintSupport)
