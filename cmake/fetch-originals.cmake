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
#   archive/amiblitz3/svn-r279/             single files of AmiBlitz3's SVN
#                                           (SourceForge amiblitz3/code,
#                                           revision 279), paths as there
#   archive/amiblitz3/github-b1a9dee/       single files of AmiBlitz3 on
#                                           GitHub (commit b1a9dee150ef),
#                                           paths as there
#   archive/amiblitz3/github-58a11658/      single files of AmiBlitz3
#                                           v3.14.0 on GitHub (commit
#                                           58a1165840a7), paths as there:
#                                           BigLibs, residents, compiler
#                                           and its ab3pft.library,
#                                           test sources of the AB3 mode,
#                                           includes used by them
# None of it is in git (.gitignore); archive/README.md lists the sources.
# Already unpacked and checked material is not touched again.
# At the end: archive/sources/ted-1.1 and blitz2-1.95 (in git) must equal
# the files of bb2src.zip in the Aminet archive.
cmake_minimum_required(VERSION 4.2.3)

get_filename_component(root ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
if(NOT DOWNLOAD_DIR)
	set(DOWNLOAD_DIR ${root}/.cache/downloads)
endif()

# name  url  sha256
set(downloads
	"ubb21.iso|https://archive.org/download/ultimate-blitz-basic-v2_1/Ultimate%20Blitz%20Basic%202.1%20%281997%29%28Acid%20Software%29.iso|205a16fa83403d61a785733cec2518bcc1fe5a6fae5b7da4da6329bd32c99166"
	"amibl2src.lha|https://aminet.net/dev/blitz/amibl2src.lha|b71e6445d0838b15416f5b55c5be422f07bd4fae5da5c065ce329042067628ff"
	"ab3-RIAmosFuncLib.obj|https://sourceforge.net/p/amiblitz3/code/279/tree/BlitzLibs/UserLibs/userlibs/RIAmosFuncLib.obj?format=raw|0863e9766957ee548cd7d9393e85d80c28a896a208d183487dbde5fe3aa45ea2")
# AmiBlitz3 sources with an embedded .xtra header (tools/xtra), GitHub at a
# fixed commit: download name ab3gh-NAME -> archive/amiblitz3/github-b1a9dee/PATH
set(ab3gh_commit b1a9dee150ef4d2793d9745a49a958fdb9178124)
set(ab3gh_files
	"Sourcecodes/Amiblitz3/Tests/compilersignals.ab3|294459511eaeab48a9fb853ee383627323e58699b84ae528e29c492b87e74374"
	"Sourcecodes/Amiblitz3/BlitzLibs/Blitzlibs/LotanMemLib.ab3|aea86f40bcc938448b487359807c596c934d9093e0124342c709735d5c635902"
	"Sourcecodes/Amiblitz3/BlitzLibs/Blitzlibs/ElmoreDosLib.ab3|e431124dbb97010063dfa37c451208cc356f2a54185d466801a0fae4a12a59ce"
	"Docs/Blitzlibs/examples/RIAppLib3.ab3|8b2ecd015833b4a5d3e64fed7819d909788cc99183c56e568a6120f6afc3d5d7"
	"Sourcecodes/Amiblitz3/Compiler/CLIModeSupport.ab3|cd2c926d71c4d6beeec5d6993feb20d8039477d2961ca0d669e3efa8746f8c15")
foreach(f IN LISTS ab3gh_files)
	string(REPLACE "|" ";" f "${f}")
	list(GET f 0 path)
	list(GET f 1 sha)
	get_filename_component(name ${path} NAME)
	list(APPEND downloads "ab3gh-${name}|https://raw.githubusercontent.com/AmiBlitz/AmiBlitz3/${ab3gh_commit}/${path}|${sha}")
endforeach()
# AmiBlitz3 v3.14.0 (the state the AB3 mode of blitz2-ab3 is built against,
# docs/ab3/variant.md): its binary BigLibs, a resident, its compiler (the
# reference of the tests) with the library it opens (ab3pft.library, the
# prefix tree of its tokeniser) and test programs, GitHub at the release
# commit:
# download name ab3v314-NAME -> archive/amiblitz3/github-58a11658/PATH
set(ab3v314_commit 58a1165840a7be19d408e727d2fe444c29da91b3)
set(ab3v314_files
	"BlitzLibs/baseLibs|23ac17b06b60916076586cb56a4c391b8137ef107a1ee3a9bfca6680a4030e87"
	"BlitzLibs/utilLibs|9b0f6d2c8cc95785b1cbc2e1d590f4bbf52a15d6d71bb0e2581c963268a2c04f"
	"BlitzLibs/OsLibs|e7eab41bebfb456ecd88464948e5273c66b1e79dcd5a9740868120506fa83a4e"
	"BlitzLibs/csLibs|940ea4fc66656df12f73c0566d68e7f463e5521ba455eeb4ef94806eb63fd037"
	"BlitzLibs/Residents/all.res|87ce951e1fd25984f110c9f6158084b1685d4eaf1dae6619efcc4b854655dc70"
	"Amiblitz3|4120e8ecae94d2489899175d0758e6f1481e24196ee6c7e922d88e7b2bb51787"
	"Libs/ab3pft.library|c766118fcedb7eed90f0c7dfe0e742ff55aa51e352318a431c8a7bf425dbf713"
	"Sourcecodes/Amiblitz3/Tests/atoi.ab3|cb0c4f3df66d6b12ca679c0f90a72d212292f54ddc72ee7b6336c48e0e39d121"
	"Sourcecodes/Amiblitz3/Tests/compilersignals.ab3|294459511eaeab48a9fb853ee383627323e58699b84ae528e29c492b87e74374"
	"Sourcecodes/Amiblitz3/Tests/data_restore_in_function.ab3|a6c2575ccc664efd904b016e1deb30d0d047aacc831129eb956cf661d299a824"
	"Sourcecodes/Amiblitz3/Tests/exec_data.ab3|e059325a2fe72346f1bc51610acab238950231e5509c5c9bc9ac1d2b33bae802"
	"Sourcecodes/Amiblitz3/Tests/localization.ab3|680c91ea170ff0aa3c888cf55fdfddf1c94cb488ecbe71b4cfce19eecfb0eca4"
	"Sourcecodes/Amiblitz3/Tests/fabrizio_speedquestion.ab3|7b310c6a88d28f10cc67485b2f038026b6c8428ce2aea6d2dc46dd5fdd1d7f7d"
	"Sourcecodes/Amiblitz3/Tests/memoryleak_test.ab3|55575c695af83140801dc609ecd136a20645efba467893ce05f53a087ecc08a9"
	"Sourcecodes/Amiblitz3/Tests/setint.ab3|bf120eaad18b23b4e0b8f450826d460e22dfdac9d1a1ef64bb04b364d8f2205a"
	"Sourcecodes/Amiblitz3/Tests/test_newtypes.ab3|a40aafb02addd7534e64506329f066f021f65dc88d9d4ab3af444b737ae66873"
	"Sourcecodes/Amiblitz3/Tests/trim.ab3|f71b479bca8a0a3ec613d17fa549d71076ca3bd04bf4de5274c96784d1cf3ecd"
	"Sourcecodes/Amiblitz3/Tests/hex_error.ab3|3c08a8db340365e0fc9a0a3b39f0db05d89efb29bb410d567232d5a17d2da508"
	"Sourcecodes/Amiblitz3/Tests/hexdigits.ab3|b8800234f7c2f5683d4b8dda352f272067a761e6d437aa4c922b34765c96c223"
	"Sourcecodes/Amiblitz3/Tests/int_problem.ab3|addd87e4454c4ea7ef06e82d47dbedc6763dcb83f2e27efd6b27e03d31324a65"
	"Sourcecodes/Amiblitz3/Tests/int_tests.ab3|6453ac0aa8e6c0518898dad6774cea63d485495ce8f0666b8f8ca61625f62133"
	"Sourcecodes/Amiblitz3/Tests/jsr.ab3|a7e39e00557fb2d374b2b6c20b9b42f2d8bba3dc4d90286f710bb9c4befae4cb"
	"Sourcecodes/Amiblitz3/Tests/macro_test.ab3|f8aa4c99c6bfd5783ec73bd9312c8bfb35d817166ae5797f1efebaa3e7dd11fa"
	"Sourcecodes/Amiblitz3/Tests/unknown_function.ab3|7a12a957665f3168f7da33cda38691668c59f772ffd0325c1f6bfa15c27ba24d"
	"Sourcecodes/Amiblitz3/Tests/unknown_function_include.ab3|e5ab24146a409e01d63625d8fbce805e61a8755d2b76dd05a89ce1b8aef8b9ed"
	"Sourcecodes/Amiblitz3/Tests/kick13.ab3|4ee0f1d98eaa505679da143b85c5b53211f6abdcdcf8495040009a4052ffdb3a"
	"Sourcecodes/Amiblitz3/Tests/windowtags.ab3|994f756fab869fcdf976ffe9dd18fbb42af857e809de5c99944fe40844dc38cb"
	"Sourcecodes/Amiblitz3/Tests/function_problem.ab3|0b60e8d257c761a69aff04c4a0c541e9c423a1fbdfec0e813e1c71dea44cb171"
	"Sourcecodes/Amiblitz3/Tests/serial_read.ab3|1a198072c6f6e7fe43be08a5f20653de55aeef06dfa9ecd12b7d112de46b8821"
	"Sourcecodes/Includes/datetime.include.ab3|2c421f2f596275b4298d7c9319fd90373b2af747525dc71d741a838bac720c8b"
	"Sourcecodes/Includes/longlong.include.ab3|0e1c7c65ef1921d5766e2de6c306beac6dbba85e05a58ed8ca98963f76046a50"
	"Sourcecodes/Includes/error.include.ab3|d2fab91bc78b37b8c752067f8a36cd5d9054cfcf2365e8ef845401086bfa14b8"
	"Sourcecodes/Includes/serial_device.include.ab3|d4b2005daf2adea40d146f76db759602ac9794ef6b35075c040c80e8ba03df04")
foreach(f IN LISTS ab3v314_files)
	string(REPLACE "|" ";" f "${f}")
	list(GET f 0 path)
	list(GET f 1 sha)
	get_filename_component(name ${path} NAME)
	list(APPEND downloads "ab3v314-${name}|https://raw.githubusercontent.com/AmiBlitz/AmiBlitz3/${ab3v314_commit}/${path}|${sha}")
endforeach()

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
		# bblibsrc.lha stores bblibsrc/oldgtlib.xtra with an MS-DOS
		# separator; the libarchive of CMake up to at least 4.2.3 (not that
		# of 4.4.5) extracts it as one name "bblibsrc\oldgtlib.xtra" next to
		# the directory: put such entries where they belong
		file(GLOB entries RELATIVE ${dest}.part ${dest}.part/*)
		foreach(e IN LISTS entries)
			string(REPLACE "\\" "/" to "${e}")
			string(FIND "${to}" "/" pos)
			if(pos GREATER -1)
				string(REPLACE "/" "\\" from "${to}")
				get_filename_component(dir ${dest}.part/${to} DIRECTORY)
				file(MAKE_DIRECTORY ${dir})
				file(RENAME ${dest}.part/${from} ${dest}.part/${to})
			endif()
		endforeach()
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

# single files of AmiBlitz3's SVN (revision 279): download name ab3-NAME ->
# archive/amiblitz3/svn-r279/PATH
foreach(f "ab3-RIAmosFuncLib.obj|BlitzLibs/UserLibs/userlibs/RIAmosFuncLib.obj")
	string(REPLACE "|" ";" f "${f}")
	list(GET f 0 name)
	list(GET f 1 path)
	set(dest ${root}/archive/amiblitz3/svn-r279/${path})
	if(NOT EXISTS ${dest})
		get_filename_component(dir ${dest} DIRECTORY)
		file(MAKE_DIRECTORY ${dir})
		file(COPY_FILE ${DOWNLOAD_DIR}/${name} ${dest})
		message(STATUS "${dest}: copied")
	endif()
endforeach()

# the AmiBlitz3 sources from GitHub: ab3gh-NAME -> archive/amiblitz3/github-b1a9dee/PATH
foreach(f IN LISTS ab3gh_files)
	string(REPLACE "|" ";" f "${f}")
	list(GET f 0 path)
	get_filename_component(name ${path} NAME)
	set(dest ${root}/archive/amiblitz3/github-b1a9dee/${path})
	if(NOT EXISTS ${dest})
		get_filename_component(dir ${dest} DIRECTORY)
		file(MAKE_DIRECTORY ${dir})
		file(COPY_FILE ${DOWNLOAD_DIR}/ab3gh-${name} ${dest})
		message(STATUS "${dest}: copied")
	endif()
endforeach()

# AmiBlitz3 v3.14.0 from GitHub: ab3v314-NAME -> archive/amiblitz3/github-58a11658/PATH
foreach(f IN LISTS ab3v314_files)
	string(REPLACE "|" ";" f "${f}")
	list(GET f 0 path)
	list(GET f 1 sha)
	get_filename_component(name ${path} NAME)
	set(dest ${root}/archive/amiblitz3/github-58a11658/${path})
	if(EXISTS ${dest})
		file(SHA256 ${dest} have)
		if(have STREQUAL sha)
			continue()
		endif()
	endif()
	get_filename_component(dir ${dest} DIRECTORY)
	file(MAKE_DIRECTORY ${dir})
	file(COPY_FILE ${DOWNLOAD_DIR}/ab3v314-${name} ${dest})
	message(STATUS "${dest}: copied")
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
