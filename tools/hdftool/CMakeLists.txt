add_executable(hdftool main.c)
target_link_libraries(hdftool PRIVATE lib_hdf)
host_readme_test(hdftool README.md)
