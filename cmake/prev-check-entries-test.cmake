# prev_check_entries with broken input: -DENTRIES=... -DREGISTERED=... -P; the message names the
# entry (the test passes on FATAL_ERROR with it, PASS_REGULAR_EXPRESSION)
include(${CMAKE_CURRENT_LIST_DIR}/prev-check-entries.cmake)
prev_check_entries("${ENTRIES}" "${REGISTERED}")
message(STATUS "prev-check-entries: all entries name a registered test")
