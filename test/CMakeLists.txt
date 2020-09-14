#[[add_executable(helpers_test
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
add_test(NAME scoring_system_test COMMAND scoring_system_test)]]

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
#[[
file(COPY "${PROJECT_SOURCE_DIR}/test/test_data/valid_alignment_file.tsv"
     DESTINATION "${PROJECT_BINARY_DIR}/test/test_data"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ WORLD_EXECUTE)
file(COPY "${PROJECT_SOURCE_DIR}/test/test_data/valid_alignment_file.tsv"
     DESTINATION "${PROJECT_BINARY_DIR}/test_data"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ WORLD_EXECUTE)
file(COPY "${PROJECT_SOURCE_DIR}/test/test_data/empty_data_file.tsv"
     DESTINATION "${PROJECT_BINARY_DIR}/test/test_data"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ WORLD_EXECUTE)
file(COPY "${PROJECT_SOURCE_DIR}/test/test_data/empty_data_file.tsv"
     DESTINATION "${PROJECT_BINARY_DIR}/test_data"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ WORLD_EXECUTE)
file(COPY "${PROJECT_SOURCE_DIR}/test/test_data/valid_alignment_file_with_additional_columns.tsv"
     DESTINATION "${PROJECT_BINARY_DIR}/test/test_data"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ WORLD_EXECUTE)
file(COPY "${PROJECT_SOURCE_DIR}/test/test_data/valid_alignment_file_with_additional_columns.tsv"
     DESTINATION "${PROJECT_BINARY_DIR}/test_data"
     FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                      GROUP_READ GROUP_EXECUTE
                      WORLD_READ WORLD_EXECUTE)
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
add_test(NAME alignment_reader_test COMMAND alignment_reader_test)]]
