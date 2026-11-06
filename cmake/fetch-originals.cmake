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
#   archive/aminet/ab2.42-distri/           AmiBlitz 2.42, full distribution
#                                           (Aminet), unpacked
#   archive/bernd/                          Bernd Rösch's AmiBlitz 2 archive
#                                           (AmiBlitz 2.41 ... 3.00 beta 4),
#                                           paths as on the site, every
#                                           LhA archive unpacked to a
#                                           directory of its name without
#                                           .lha
# None of it is in git (.gitignore); archive/README.md lists the sources.
# Already unpacked and checked material is not touched again.
# At the end: the sources and changelogs in git (archive/sources/,
# archive/releases/) must equal the files of the downloaded archives.
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
# AmiBlitz 2.42, the full distribution on Aminet, and its readme (in git
# as archive/sources/ab2.42-distri.readme)
list(APPEND downloads
	"ab2.42-distri.lha|https://aminet.net/dev/blitz/ab2.42-distri.lha|6f3993d1b6d911b90db9cf73a944f08d62a1576e6f8cf5deabe9abb9b3554dac"
	"ab2.42-distri.readme|https://aminet.net/dev/blitz/ab2.42-distri.readme|b515e098cd24f2f6b476068ccdf30107245d7fc69d71b9795df9a26a72f37707")
# Bernd Rösch's AmiBlitz 2 archive, the update location the readme of
# ab2.42-distri names: the releases 2.43 ... 3.00 beta 4 (compiler, PED,
# AcidLibs, DefLibs, residents) and the sources of compiler and PED.
# Download name bernd/PATH -> archive/bernd/PATH (an LhA archive unpacked
# to archive/bernd/PATH without .lha; pedsrc.bb2 is an LhA archive too).
# Not taken: the manual and its derivatives, tools and media of the site
# (archive/README.md); ab3/Source/ab3src_30014.lha (the same file as
# ab3/Exe/AB3_V30014.lha) and ab3/Data/Residents/ (the same files as the
# residents of AmiBlitz3full_beta4.lha).
set(bernd_url https://www.david-mcminn.co.uk/bernd)
set(bernd_files
	"ab2.42_upd_to_2.43.lha|e455c5d7389f547cc571ab9a1949f8de10dc86b3e303d6223920cda7eb4ce6c6"
	"ab2.43b2_upd.lha|ef5b9d09c38984bf843979f4da7411d48023a0ef24fdc9d1c1c0c41d59f413b3"
	"ab2.43.lha|a23ded3bf0b8510b1c9b6ee516731e194e9b19d51fcaa3bd8100cd3adb12275c"
	"ab2.43src.lha|0143d8f7c4f41dd05fa019f2f6844546d248eb621a1b3038f3f38fbfe8c3af06"
	"ab2.45betadir/ab2.45b1.lha|cf9711fc790f06b119d3600bbe3bff3f9ac9aaa41d05dc8313c4d41f22705203"
	"ab2.45betadir/ab2.45b2.lha|4e4b8c5f397ba19d230a700e22a24a02f893f30dc9defd2c632ab39e674dbf5d"
	"ab2.45betadir/ab2.45b3.lha|da2ca816f1ada011b83800c8ef60c707e1df43ba3c780c456a864b06ba5b33e6"
	"ab2.5beta/Amiblitz2517.lha|c9bdfc1d9be5758da838a9fac7b42907159cb47cb31795a3ee96dfad55c0c1c4"
	"ab2.5beta/Amiblitz251.lha|b232a68016771bf464bcd60503f1b0335b8f3bb59a3b02243e3981dcebe1b6fc"
	"ab2.5beta/lib_mapping.txt|96e24d746994d05c2a5b1c259805002c5793a9197c034bc6595fc59f1c26245b"
	"ab2.5beta/PED_V2511.lha|827ff9ccc8db1131d1f55b67e968f534e9f90fb0e591b591fb88e25ca9c185ee"
	"ab2.5beta/PED_V2513.lha|4c96fedc0d9d82995e1d18f66d24e8dab16c01f6f63030d0d65338f16155bf79"
	"ab2.5beta/PED_V2514.lha|91db3f26079ecfe8b0e75a020a760a8aff57ce0e9091f78e95c3fba06bb5d92c"
	"ab2.5beta/PED_V2516.lha|62f04d750575ae647ef868085cc547464eb4ae7207a6c75291a2aa4fa2b1a5f5"
	"ab2.5beta/PED_V2521.lha|da830801914b60cced7f72dcc9ed8c627ca351ad3552ef97d34cc5a93e0d9a88"
	"ab2.5beta/PED_V2522.lha|44876370c5f46264d75b6a36ac66680e56cc2e9fe919fcd740433b31744a3480"
	"ab2.5beta/PED_V2534.lha|e299555d2a06342f03cc6402deb91af6f98bdeac728087f0fd27d1de43735f4f"
	"ab2.5beta/source/amiblitz2_V2517_source.lha|84b0c75deb0e361ec8212204b4364f37d77d83f555bb4f1bf3101c49361b8dea"
	"ab2.5beta/source/amiblitz2_V251_source.lha|a3529e4419d4ce2f75fe34c2aebd4d5f9946343c5d0aab2c288a0e46d4ce0783"
	"ab2.5beta/source/library_offsets.lha|b79af6921abbedaf53715ee78fbb2081bed7b5309dd9189e04ab7e0647eb4ad2"
	"ab2.5beta/source/PED_V2513_source.lha|c946fd57e95079c654dd7fe550434cdc8c07c9ebb5976969b7e6c075cedc4567"
	"ab2.5beta/source/PED_V2514_source.lha|994693c410cc0a73b5446d13b5bace3d6a77063181d32294be2fc37cfb431daf"
	"ab2.5beta/source/PED_V2521_source.lha|8fff94247228e410fcbd630757f1c8ce2d3cf99486493924a5dfe5cdd64af820"
	"ab2.5beta/source/PE_Includes/PE_Tools.lha|bf2cdb4997cec6cb88e4d434e5ee73aac30806a35615cb20062c0534d169f723"
	"ab2.5beta/source/PE_Includes/WZ_ToolsV13.lha|2a84da6f3e4d64aeea553a9f3a8425addf19a0a091fa3a3bbf1b6a81b140e193"
	"ab3/AmiBlitz3full_beta4.lha|c4a91b7cbe222489ad9197462280060bb64e84a6f590869c6a25b949b00dca8f"
	"ab3/Data/lib_mapping.txt|a2d3159bb5902242ff97f7c14ebbe8593ce3e22294fb124973c695e2aaa4ac81"
	"ab3/Exe/AB3_V30010.lha|9da951a61837962535728fd7c66065b0fd14e6f8085d094c933291f673f2c5a4"
	"ab3/Exe/AB3_V30012.lha|0baa1ead0c42e895f5554d35c1bfdabc0259a4bd6de9bd24e2f291f9f0e6f80a"
	"ab3/Exe/AB3_V30013.lha|b90518cfb89cb825ab4f1845e6d4a41dce682d9f9e567fe4dce7eb551c367811"
	"ab3/Exe/AB3_V30014.lha|6060ce7fc3d8c732e5870de30341a4484a4686c099243e6e8b37c7b76d87019d"
	"ab3/Exe/AB3_V30016.lha|10d52f23549f55732ef8f20529120a370445b2f442339f1421cd6e223a1f64ad"
	"ab3/Exe/AB3_V30019.lha|f51c34e15715d6267851d858f160e771ec512d40ca350fc39c168b7609a337d8"
	"ab3/Exe/PED_V30018.lha|cfbd70cc7715307b1004005e2fa3366b9d82570a164dabd5b36394d05b65a4fe"
	"ab3/Exe/PED_V30019.lha|671c57e7c19d37f9afc3ae52e92d53c83f4e997ace9500f521db22a7216a82b0"
	"ab3/Exe/PED_V30020.lha|a58fa1c07e93f44158dd8514e457e2dc354cbca06421fa085d2daa40517ebd32"
	"ab3/Exe/PED_V30021.lha|90df0990cab29c1b3ad69589010d47ff68c59ba7d1e95679a623441da7524561"
	"ab3/Exe/PED_V30024.lha|29cd82aa3badd34edd69e32448a607b4679ac639665525f19dd63bbe91b6d9be"
	"ab3/Exe/PED_V30025.lha|bbd4d638213286287d20fb4df92b6693a19a0123cc7350b777b97f05d642267d"
	"ab3/Exe/PED_V30026b.lha|6460867dee9de540279b0b98c83fa18f44b1a0852279cc4eca78b1cb6500ac0e"
	"ab3/Exe/PED_V30027c.lha|5821eaa00dca9161addc003cdf2803be6bf13dcb381ed545ac498022fc43fcfc"
	"ab3/Exe/PED_V30029.lha|edf8d307e2fcaded447bf50c78b4e5f3619ec9eddf77a79da93ec823be00ea2b"
	"ab3/Exe/PED_V30030.lha|34cec631a0664008793b28c30eb8fec32efbe3b4177d51a90b7994a713f28a43"
	"ab3/Source/ab3scr_30019.lha|08f1fb51a916c153f55c22ca9b098f242a38e8ff5f9085d4bf8993c06c35ed49"
	"ab3/Source/ab3src_30012.lha|b6788ddc28bc33f43d7970465dc783394c1e18a0f91a88f23fadc52a1fc3633b"
	"ab3/Source/ab3src_30015.lha|53904e38d807a5193b7655d8b3ba053dc75f92baeef925615969aa6a62ec8e2c"
	"ab3/Source/backup/ab3_3011.lha|ad4e9826a72e0c9769ba8759413e80fab2931214e11e9f3c359092c228f5c227"
	"ab3/Source/backup/ped_3011.lha|d0abbf37086a10296885bff61793f162f2b8c5ad3869a1bc7007ee1cc8448ba9"
	"ab3/Source/backup/ped_3012.lha|794bd9e753312c7f108fe51f169fd0d49f64d5277d6ab59e47255b33515fc70b"
	"ab3/Source/backup/ted2.wizard|f37f81db683f3b31bcce18b1b2f34080092b3306cc2cad29ed716e16faa5744b"
	"ab3/Source/compiler.wizard|471f0fba346d4245e7ba9d5598d20fbb0cd720d53b39134030d64b5039627722"
	"ab3/Source/library_offsets.bb2|ec99e29b80c1ff21aeb3c45d466e2689dc8100b2218d35d0588a11c63c73fbe3"
	"ab3/Source/pedsrc_30018.lha|c6db2159f2e365123dbffd0feba820e9b9e846aaeb856defa2ad0c9053c3f358"
	"ab3/Source/pedsrc_30023.lha|3f139082a27d5c5a3477c9a359895d76ea9096c8802bde9b3828c1038aada599"
	"ab3/Source/pedsrc_30025a.lha|abb79df56e9e2aec943b1e9873ff0edb4f589e95e827dca52fa5133af672d010"
	"ab3/Source/pedsrc_v30030.lha|638d91fa0c747d2aa26b793f86c1ed465cb26c74b45f3e58ea343d15e60f3000"
	"ab3/Source/PED.wizard|8f7ddd9f71b6bfc71860d6a968980a9c7b2b1580d975b56a355387549ff340f1"
	"ab3/Source/wizard_Tools.lha|6c8bfbf95d23adbf63663f105b1ca3da6de616d309f59bdd70c131304d19fa91"
	"acidlibs.lha|54dc12b0a12988c6b0ad4d6945f156bc48a28b256e052d771719980e3a0f3673"
	"all.res.lha|f0f1351ac08a8329220542a2c053c66d2a05c1c020fe0d9eb2b925793d350bd6"
	"amiblitz2.41src.lha|6edc47830e00c4dccabdb2baf98b9257a2c54b6ba0b603a9bc929c8984260d72"
	"deflibs.lha|ded0cdb322c00bb40933dfcd203c5d661de4c28e15e1011d4cf50806d48826cb"
	"ped.lha|b2d9eb283f5cbdfc0fb72b419d94f0fbd03bc62b3e1c0e96c4b50510fea7e0c8"
	"pedsrc.bb2|2424c5799711b319bf831e2065afa8e0b9d063eb121b3a5b189e100ab61c4620")
foreach(f IN LISTS bernd_files)
	string(REPLACE "|" ";" f "${f}")
	list(GET f 0 path)
	list(GET f 1 sha)
	list(APPEND downloads "bernd/${path}|${bernd_url}/${path}|${sha}")
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

# AmiBlitz 2.42 from Aminet: ab2.42-distri.lha -> archive/aminet/ab2.42-distri/
set(ab242 ${root}/archive/aminet/ab2.42-distri)
unpack(${DOWNLOAD_DIR}/ab2.42-distri.lha ${ab242})
if(unpacked)
	file(GLOB_RECURSE files ${ab242}.part/*)
	list(LENGTH files n)
	if(NOT n EQUAL 1119)
		message(FATAL_ERROR "ab2.42-distri.lha: ${n} files, 1119 expected")
	endif()
	finish(${ab242} ${DOWNLOAD_DIR}/ab2.42-distri.lha)
endif()

# Bernd Rösch's archive: bernd/PATH -> archive/bernd/PATH, LhA archives
# unpacked to archive/bernd/PATH without the extension
foreach(f IN LISTS bernd_files)
	string(REPLACE "|" ";" f "${f}")
	list(GET f 0 path)
	list(GET f 1 sha)
	set(src ${DOWNLOAD_DIR}/bernd/${path})
	set(dest ${root}/archive/bernd/${path})
	if(path MATCHES "\\.lha$" OR path STREQUAL "pedsrc.bb2")
		string(REGEX REPLACE "\\.(lha|bb2)$" "" dest ${dest})
		unpack(${src} ${dest})
		if(unpacked)
			finish(${dest} ${src})
		endif()
		continue()
	endif()
	if(EXISTS ${dest})
		file(SHA256 ${dest} have)
		if(have STREQUAL sha)
			continue()
		endif()
	endif()
	get_filename_component(dir ${dest} DIRECTORY)
	file(MAKE_DIRECTORY ${dir})
	file(COPY_FILE ${src} ${dest})
	message(STATUS "${dest}: copied")
endforeach()

# the sources and changelogs of AmiBlitz in git must be the files of the
# downloaded material: "MINE|ORIGIN[,ORIGIN...]", MINE relative to
# archive/, ORIGIN relative to archive/ or, with download: in front, a
# download; a directory MINE holds exactly the files of its ORIGIN
# directories, a single file MINE equals its ORIGIN
foreach(pair
		"sources/amiblitz2-2.41|bernd/amiblitz2.41src"
		"sources/amiblitz2-2.43b12|bernd/ab2.43src"
		"sources/amiblitz2-2.50beta|bernd/ab2.5beta/source/amiblitz2_V251_source,bernd/ab2.5beta/source/library_offsets"
		"sources/amiblitz2-2.517beta|bernd/ab2.5beta/source/amiblitz2_V2517_source"
		"sources/amiblitz3-3.00.19a|bernd/ab3/Source/ab3scr_30019"
		"sources/ab2.42-distri.readme|download:ab2.42-distri.readme"
		"releases/amiblitz2-2.42/Amiblitz2/news2.4.txt|aminet/ab2.42-distri/Amiblitz2/news2.4.txt"
		"releases/amiblitz2-2.42/Amiblitz2/news2.41b1.txt|aminet/ab2.42-distri/Amiblitz2/news2.41b1.txt"
		"releases/amiblitz2-2.42/Amiblitz2/news2.42.txt|aminet/ab2.42-distri/Amiblitz2/news2.42.txt"
		"releases/amiblitz2-2.42/Amiblitz2/amiblitzHistory.txt|aminet/ab2.42-distri/Amiblitz2/amiblitzHistory.txt"
		"releases/amiblitz2-2.43b2/amiblitz2.43/news2.43b2.txt|bernd/ab2.43b2_upd/amiblitz2.43/news2.43b2.txt"
		"releases/amiblitz2-2.43/amiblitz2.43/news2.43.txt|bernd/ab2.42_upd_to_2.43/amiblitz2.43/news2.43.txt"
		"releases/amiblitz2-2.45b2/amiblitz2.45/news2.45.txt|bernd/ab2.45betadir/ab2.45b2/amiblitz2.45/news2.45.txt"
		"releases/amiblitz3-3.00-beta4/AmiBlitz3/Docs/Changelog.guide|bernd/ab3/AmiBlitz3full_beta4/AmiBlitz3/Docs/Changelog.guide")
	string(REPLACE "|" ";" pair "${pair}")
	list(GET pair 0 mine)
	list(GET pair 1 origins)
	string(REPLACE "," ";" names "${origins}")
	set(origins "")
	foreach(o IN LISTS names)
		if(o MATCHES "^download:(.*)")
			list(APPEND origins ${DOWNLOAD_DIR}/${CMAKE_MATCH_1})
		else()
			list(APPEND origins ${root}/archive/${o})
		endif()
	endforeach()
	set(mine ${root}/archive/${mine})
	if(NOT IS_DIRECTORY ${mine})
		execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${mine} ${origins} RESULT_VARIABLE diff)
		if(diff)
			message(FATAL_ERROR "${mine} differs from ${origins}")
		endif()
		continue()
	endif()
	file(GLOB_RECURSE have RELATIVE ${mine} ${mine}/*)
	set(want "")
	foreach(o IN LISTS origins)
		file(GLOB_RECURSE names RELATIVE ${o} ${o}/*)
		foreach(f IN LISTS names)
			execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${mine}/${f} ${o}/${f} RESULT_VARIABLE diff)
			if(diff)
				message(FATAL_ERROR "${mine}/${f} differs from ${o}/${f} (or is missing)")
			endif()
		endforeach()
		list(APPEND want ${names})
	endforeach()
	list(LENGTH have n_have)
	list(LENGTH want n_want)
	if(NOT n_have EQUAL n_want)
		message(FATAL_ERROR "${mine}: ${n_have} files, the archives have ${n_want}")
	endif()
	message(STATUS "${mine}: equal to the downloaded archives")
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
