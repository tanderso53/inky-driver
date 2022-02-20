cmake_minimum_required(VERSION 3.21)

project(pimoroni-inky-driver)

if(NOT(DEFINED PIMORONI_INKY_BUILD_TESTING))

  set(PIMORONI_INKY_BUILD_TESTING false)

endif()

if(NOT(DEFINED PIMORONI_INKY_BUILD_INST_CODE))

  set(PIMORONI_INKY_BUILD_INST_CODE false)

endif()

add_library(pimoroni-inky-driver INTERFACE)

target_sources(pimoroni-inky-driver INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/src/inky.c)

target_include_directories(pimoroni-inky-driver INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/include)

set_target_properties(pimoroni-inky-driver PROPERTIES
  PUBLIC_HEADER ${CMAKE_CURRENT_LIST_DIR}/include/inky.h)

set_target_properties(pimoroni-inky-driver PROPERTIES
  PRIVATE_HEADER ${CMAKE_CURRENT_LIST_DIR}/include/inky-api.h)

#########
# Tests #
#########

if(PIMORONI_INKY_BUILD_TESTING)

  enable_testing()

  add_executable(inky-fb-test
    ${CMAKE_CURRENT_LIST_DIR}/tests/fb-test.c
    ${CMAKE_CURRENT_LIST_DIR}/lib/munit/munit.c)

  target_link_libraries(inky-fb-test PRIVATE
    pimoroni-inky-driver)

  target_include_directories(inky-fb-test PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/lib)

  add_test(NAME pimoroni-inky-test-suite
    COMMAND $<TARGET_FILE:inky-fb-test>)

  if(PIMORONI_INKY_BUILD_INST_CODE)

    # Enable coverage instrumentation if coverage tools are present
    find_program(CC_COV clang REQUIRED)
    find_program(PROFILER llvm-profdata REQUIRED)
    find_program(LCOV llvm-cov REQUIRED)
    find_program(JQ jq REQUIRED)

    set(CC ${CC_COV})

    set_source_files_properties(src/inky.c #tests/pm2_5-api-tests.c
      PROPERTIES
      COMPILE_OPTIONS "-fprofile-instr-generate;-fcoverage-mapping")

    target_link_options(inky-fb-test PRIVATE
      -fprofile-instr-generate -fcoverage-mapping)

    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)

    add_custom_target(inky-fb-test-coverage
      LLVM_PROFILE_FILE=coverage.profraw $<TARGET_FILE:inky-fb-test>
      COMMAND ${PROFILER} merge -sparse coverage.profraw -o coverage.profdata
      COMMAND ${LCOV} export $<TARGET_FILE:inky-fb-test>
      -instr-profile=coverage.profdata > coverage.json
      COMMAND cat coverage.json | jq ".data[0].totals.lines.percent" |
      sed -E "s/(.*)/Coverage: \\1/"
      BYPRODUCTS coverage.profraw coverage.profdata coverage.json
      VERBATIM)

    add_dependencies(inky-fb-test-coverage inky-fb-test)

  endif()

endif()
