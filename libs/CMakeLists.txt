# The libraries blitz2 reads at startup: blitz2:acidlibs (command
# libraries) and blitz2:deflibs (AmigaOS call declarations), each one BigLib
# file joined from one library file per C source (cmake/BlitzLibrary.cmake).
add_subdirectory(acidlibs)
add_subdirectory(deflibs)
