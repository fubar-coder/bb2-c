# model-driver-check.cmake - the driver the build writes for the comparison
# with the prev build (cmake/basic-test-model.cmake basic_test_driver) calls
# every case of the source: its text DRIVER must match WANT.
#   cmake -DDRIVER=NAME.bas -DWANT=REGEX -P model-driver-check.cmake
file(READ ${DRIVER} text)
if(NOT text MATCHES "${WANT}")
	message(FATAL_ERROR "${DRIVER} does not match: ${WANT}")
endif()
message("${DRIVER}: every case is called")
