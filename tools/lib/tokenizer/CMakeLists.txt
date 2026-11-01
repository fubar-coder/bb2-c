# the tokeniser of shared/tokenizer (tokenizer.h), compiled for the host:
# the library of the tests (tools/tests/test_tokenizer) and of the tools
get_filename_component(TOKENIZER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../shared/tokenizer ABSOLUTE)
add_library(lib_tokenizer STATIC ${TOKENIZER_DIR}/tokenizer.c)
set_target_properties(lib_tokenizer PROPERTIES OUTPUT_NAME tokenizer C_STANDARD 99 C_EXTENSIONS OFF)
target_compile_options(lib_tokenizer PRIVATE -pedantic)
target_include_directories(lib_tokenizer PUBLIC ${TOKENIZER_DIR})
