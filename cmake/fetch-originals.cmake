# Download the original material the tests need but the repository does not
# contain (third-party content of the CD, the Aminet archive of the
# sources), check it and unpack it - with CMake only.
#
#   cmake -P cmake/fetch-originals.cmake [-DDOWNLOAD_DIR=DIR]
#   cmake --build --preset amiga --target fetch-originals
#
#   .cache/downloads/                       the downloads (SHA-256 checked)
#   archive/cd/ultimate-blitz-basic-2.1/    the CD, file names as on the CD
#                                           (ISO 9660, upper case; the
#                                           Linux kernel shows them lower
#                                           case with map=normal)
#   archive/aminet/amibl2src/               amibl2src.lha with bb2src.zip
#                                           unpacked (bb2src/),
#                                           bblibsrc.lha (bblibsrc/) and
#                                           RWELibSourc.lha (rwelibsrc/)
# None of it is in git (.gitignore); archive/README.md lists the sources.
# Already unpacked and checked material is not touched again.
# At the end: archive/sources/ted-1.1 and blitz2-1.95 (in git) must equal
# the files of bb2src.zip in the Aminet archive.
cmake_minimum_required(VERSION 3.29)

get_filename_component(root ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
if(NOT DOWNLOAD_DIR)
	set(DOWNLOAD_DIR ${root}/.cache/downloads)
endif()

# name  url  sha256
set(downloads
	"ubb21.iso|https://archive.org/download/ultimate-blitz-basic-v2_1/Ultimate%20Blitz%20Basic%202.1%20%281997%29%28Acid%20Software%29.iso|205a16fa83403d61a785733cec2518bcc1fe5a6fae5b7da4da6329bd32c99166"
	"amibl2src.lha|https://aminet.net/dev/blitz/amibl2src.lha|b71e6445d0838b15416f5b55c5be422f07bd4fae5da5c065ce329042067628ff")

file(MAKE_DIRECTORY ${DOWNLOAD_DIR})
foreach(d IN LISTS downloads)
	string(REPLACE "|" ";" d "${d}")
	list(GET d 0 name)
	list(GET d 1 url)
	list(GET d 2 sha)
	set(path ${DOWNLOAD_DIR}/${name})
	if(EXISTS ${path})
		file(SHA256 ${path} have)
		if(have STREQUAL sha)
			message(STATUS "${name}: present")
			continue()
		endif()
		file(REMOVE ${path})
	endif()
	message(STATUS "${name}: downloading ${url}")
	file(DOWNLOAD ${url} ${path}.part EXPECTED_HASH SHA256=${sha} STATUS st TLS_VERIFY ON)
	list(GET st 0 code)
	if(code)
		file(REMOVE ${path}.part)
		message(FATAL_ERROR "${name}: download failed: ${st}")
	endif()
	file(RENAME ${path}.part ${path})
endforeach()

# unpack ARCHIVE into DEST once: into DEST.part first, DEST.sha256 marks a
# complete unpacking of exactly this archive
function(unpack archive dest)
	file(SHA256 ${archive} sha)
	if(EXISTS ${dest}.sha256)
		file(READ ${dest}.sha256 done)
		string(STRIP "${done}" done)
		if(done STREQUAL sha AND IS_DIRECTORY ${dest})
			message(STATUS "${dest}: present")
			set(unpacked FALSE PARENT_SCOPE)
			return()
		endif()
	endif()
	file(REMOVE_RECURSE ${dest} ${dest}.part ${dest}.sha256)
	file(MAKE_DIRECTORY ${dest}.part)
	file(ARCHIVE_EXTRACT INPUT ${archive} DESTINATION ${dest}.part)
	set(unpacked TRUE PARENT_SCOPE)
endfunction()

function(finish dest archive)
	file(RENAME ${dest}.part ${dest})
	file(SHA256 ${archive} sha)
	file(WRITE ${dest}.sha256 "${sha}\n")
	message(STATUS "${dest}: unpacked")
endfunction()

# ------------------------------------------------------------------- CD
set(cd ${root}/archive/cd/ultimate-blitz-basic-2.1)
unpack(${DOWNLOAD_DIR}/ubb21.iso ${cd})
if(unpacked)
	# the three names with Latin-1 characters (lower case c cedilla $E7,
	# n tilde $F1 as on the CD) -> UTF-8, nothing else changes
	string(ASCII 231 c_cedilla)
	string(ASCII 241 n_tilde)
	foreach(r
			"BUI6/BLITZBANK/CATALOGS/FRAN${c_cedilla}AIS|BUI6/BLITZBANK/CATALOGS/FRANçAIS"
			"BUI6/FRONTENDV1.0/CATALOGS/ESPA${n_tilde}OL|BUI6/FRONTENDV1.0/CATALOGS/ESPAñOL"
			"BUI7/FRONTENDV1.2/CATALOGS/ESPA${n_tilde}OL|BUI7/FRONTENDV1.2/CATALOGS/ESPAñOL")
		string(REPLACE "|" ";" r "${r}")
		list(GET r 0 from)
		list(GET r 1 to)
		if(NOT IS_DIRECTORY ${cd}.part/${from})
			message(FATAL_ERROR "CD: ${from} not found (other image?)")
		endif()
		file(RENAME ${cd}.part/${from} ${cd}.part/${to})
	endforeach()
	file(GLOB_RECURSE files ${cd}.part/*)
	list(LENGTH files n)
	if(NOT n EQUAL 4310)
		message(FATAL_ERROR "CD: ${n} files, 4310 expected")
	endif()
	finish(${cd} ${DOWNLOAD_DIR}/ubb21.iso)
endif()

# --------------------------------------------------------------- Aminet
set(aminet ${root}/archive/aminet/amibl2src)
unpack(${DOWNLOAD_DIR}/amibl2src.lha ${aminet})
if(unpacked)
	set(zip ${aminet}.part/amiblitz2source/bb2src.zip)
	if(NOT EXISTS ${zip})
		message(FATAL_ERROR "amibl2src: amiblitz2source/bb2src.zip not found")
	endif()
	file(MAKE_DIRECTORY ${aminet}.part/bb2src)
	file(ARCHIVE_EXTRACT INPUT ${zip} DESTINATION ${aminet}.part/bb2src)
	finish(${aminet} ${DOWNLOAD_DIR}/amibl2src.lha)
endif()

# the Blitz sources of the libraries, tokenised as TED saves them:
# bblibsrc.lha (its directory bblibsrc/, the libraries of blitz2 1.95) to
# bblibsrc/, RWELibSourc.lha (its directory Libs/, the libraries Red When
# Excited released in September 1999, not part of blitz2 2.10) to
# rwelibsrc/.  Inside ${aminet}: unpacking the Aminet archive again also
# redoes this.
foreach(l "bblibsrc|bblibsrc.lha|bblibsrc|209" "rwelibsrc|RWELibSourc.lha|Libs|287")
	string(REPLACE "|" ";" l "${l}")
	list(GET l 0 name)
	list(GET l 1 lha)
	list(GET l 2 top)
	list(GET l 3 count)
	set(dest ${aminet}/${name})
	unpack(${aminet}/amiblitz2source/${lha} ${dest})
	if(unpacked)
		if(NOT IS_DIRECTORY ${dest}.part/${top})
			message(FATAL_ERROR "${lha}: directory ${top}/ not found")
		endif()
		file(REMOVE_RECURSE ${dest}.files)
		file(RENAME ${dest}.part/${top} ${dest}.files)
		file(REMOVE_RECURSE ${dest}.part)
		file(RENAME ${dest}.files ${dest}.part)
		file(GLOB_RECURSE files ${dest}.part/*)
		list(LENGTH files n)
		if(NOT n EQUAL count)
			message(FATAL_ERROR "${lha}: ${n} files, ${count} expected")
		endif()
		finish(${dest} ${aminet}/amiblitz2source/${lha})
	endif()
endforeach()

# the sources in git must be the ones of the archive
foreach(pair "ted-1.1|tedt/" "blitz2-1.95|")
	string(REPLACE "|" ";" pair "${pair}")
	list(GET pair 0 dir)
	list(GET pair 1 sub)
	file(GLOB names RELATIVE ${root}/archive/sources/${dir} ${root}/archive/sources/${dir}/*)
	foreach(f IN LISTS names)
		execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files
			${root}/archive/sources/${dir}/${f} ${aminet}/bb2src/${sub}${f} RESULT_VARIABLE diff)
		if(diff)
			message(FATAL_ERROR "archive/sources/${dir}/${f} differs from the Aminet archive")
		endif()
	endforeach()
	message(STATUS "archive/sources/${dir}: equal to amibl2src.lha bb2src.zip:${sub}")
endforeach()
