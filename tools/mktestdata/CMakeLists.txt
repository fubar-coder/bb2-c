add_executable(mktestdata main.c)
target_link_libraries(mktestdata PRIVATE lib_mktestdata)
host_readme_test(mktestdata README.md)
