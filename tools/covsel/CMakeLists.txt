# covsel; its rules are rules.txt here (cmake/coverage-select.cmake)
add_executable(covsel main.c)
target_link_libraries(covsel PRIVATE lib_covsel)
