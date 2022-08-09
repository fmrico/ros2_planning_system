set(TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test_data)

install(DIRECTORY
  ${TEST_DATA_DIR}
  DESTINATION share/${PROJECT_NAME}
)

add_subdirectory(unit)
