add_executable(helpers_test
        "${PROJECT_SOURCE_DIR}/test/helpers_test.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(helpers_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME helpers_test COMMAND helpers_test)

add_executable(alignment_test
        "${PROJECT_SOURCE_DIR}/test/alignment_test.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc"
        "${PROJECT_SOURCE_DIR}/src/scoring_system.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(alignment_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME alignment_test COMMAND alignment_test)

add_executable(scoring_system_test
        "${PROJECT_SOURCE_DIR}/test/scoring_system_test.cc"
        "${PROJECT_SOURCE_DIR}/src/scoring_system.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(scoring_system_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME scoring_system_test COMMAND scoring_system_test)

add_executable(alignment_batch_test
        "${PROJECT_SOURCE_DIR}/test/alignment_batch_test.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment_batch.cc"
        "${PROJECT_SOURCE_DIR}/src/scoring_system.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(alignment_batch_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME alignment_batch_test COMMAND alignment_batch_test)

add_executable(alignment_reader_test
        "${PROJECT_SOURCE_DIR}/test/alignment_reader_test.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment_reader.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment_batch.cc"
        "${PROJECT_SOURCE_DIR}/src/scoring_system.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(alignment_reader_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME alignment_reader_test COMMAND alignment_reader_test)

add_executable(paste_output_test
        "${PROJECT_SOURCE_DIR}/test/paste_output_test.cc"
        "${PROJECT_SOURCE_DIR}/src/paste_output.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment_batch.cc"
        "${PROJECT_SOURCE_DIR}/src/scoring_system.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(paste_output_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME paste_output_test COMMAND paste_output_test)

add_executable(stats_collector_test
        "${PROJECT_SOURCE_DIR}/test/stats_collector_test.cc"
        "${PROJECT_SOURCE_DIR}/src/stats_collector.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment_batch.cc"
        "${PROJECT_SOURCE_DIR}/src/scoring_system.cc"
        "${PROJECT_SOURCE_DIR}/src/alignment.cc"
        "${PROJECT_SOURCE_DIR}/src/helpers.cc")
target_include_directories(stats_collector_test PUBLIC
        "${PROJECT_SOURCE_DIR}/test"
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/lib/catch/include")
add_test(NAME stats_collector_test COMMAND stats_collector_test)
