set(sources
    main.cpp
    TestColorProfileUtils.cpp
    TestContentSpanFinder.cpp
    TestDeskewParams.cpp
    TestObliqueFinder.cpp
    TestImageId.cpp
    TestImageMetadataCopier.cpp
    TestMargins.cpp
    TestPageId.cpp
    TestPageRange.cpp
    TestPageSequence.cpp
    TestSelectContentApply.cpp
    TestSmartFilenameOrdering.cpp
    TestUnits.cpp)

add_executable(core_tests ${sources})
target_link_libraries(
    core_tests
    PRIVATE core Boost::unit_test_framework ${EXTRA_LIBS})

add_test(NAME core_tests COMMAND core_tests --log_level=message)

if (TARGET LCMS2::LCMS2)
  target_compile_definitions(core_tests PRIVATE SCANTAILOR_HAVE_LCMS2)
endif()
