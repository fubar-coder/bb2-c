/* libnums.h of the variant bb2-2.10 (the default: blitz2/acidlibs,
   deflibs) - library numbers of Blitz Basic 2, generated in bb2 by
   tools/mklibnums.cmake from the CD's extras/blitzlibs/libnums.bb.
   LIB_xxx = #xxx of the original; numbers decimal as there.  At the end
   (not from libnums.bb) the numbers a library gives literally. */
#ifndef BLITZ_LIBNUMS_H
#define BLITZ_LIBNUMS_H

/* SYSTEM LIBRARIES: */
#define LIB_varslib              65534
#define LIB_wbstartuplib         65533
#define LIB_memlib               65530
#define LIB_intlib               65520
#define LIB_switchlib            65510
#define LIB_runerrlib            65500
#define LIB_arrayslib            65434
#define LIB_staticslib           65335
#define LIB_strings1lib          65235
#define LIB_strings2lib          65135
#define LIB_exitslib             65035
#define LIB_ffplib               64935
#define LIB_lmullib              64835
#define LIB_ldivlib              64735
#define LIB_clrlib               64635
#define LIB_datalib              64535
#define LIB_strcomplib           64435
#define LIB_maxslib              64335
#define LIB_maxlenlib            64135
#define LIB_chipbaselib          64100
#define LIB_floatquicklib        64080
#define LIB_modlib               64070
#define LIB_debuglib             64060
#define LIB_rundebuglib          64050
#define LIB_errtraplib           64040   /* 64090 */
/* AMIGA LIBRARIES: */
#define LIB_intuitionlib           255
#define LIB_graphicslib            254
#define LIB_execlib                253
#define LIB_doslib                 252
#define LIB_diskfontlib            251
#define LIB_graphicslib2           250
#define LIB_doslib2                249
#define LIB_amigaguidelib          248
#define LIB_asllib                 247
#define LIB_battclocklib           246
#define LIB_batmemlib              245
#define LIB_bulletlib              244
#define LIB_cardreslib             243
#define LIB_ciaalib                242
#define LIB_ciablib                241
#define LIB_commoditieslib         240
#define LIB_datatypeslib           239
#define LIB_disklib                238
#define LIB_expansionlib           237
#define LIB_gadtoolslib            236
#define LIB_iconlib                235
#define LIB_iffparselib            234
#define LIB_keymaplib              233
#define LIB_layerslib              232
#define LIB_localelib              231
#define LIB_mathffplib             230
#define LIB_mathieeedoubbaslib     229
#define LIB_mathieeedoubtranslib   228
#define LIB_mathieeesingbaslib     227
#define LIB_mathieeesingtranslib   226
/* #mathtranslib=225: redefined further down, Blitz 2 uses the later value */
#define LIB_misclib                224
#define LIB_potgolib               223
#define LIB_rexxsyslib             222
#define LIB_utilitylib             221
#define LIB_wblib                  220
#define LIB_cdtvlib                219   /* * reserved */
#define LIB_xprzlib                218   /* * reserved */
#define LIB_reqlib                 201   /* * fuzzy */
#define LIB_crmlib                 200
/* BASIC LIBRARIES: */
#define LIB_utillib                194
#define LIB_environlib             193
#define LIB_mathtranslib           192
#define LIB_gameiolib              190
#define LIB_printlib               187
#define LIB_editlib                186
#define LIB_stringfunclib          185
#define LIB_memacclib              180
#define LIB_inputoutputlib         164
#define LIB_iffiolib               162
#define LIB_ilbmifflib             161
#define LIB_bitmaplib              160
#define LIB_screenslib             158
#define LIB_palettelib             156
#define LIB_shapeslib              154
#define LIB_freqlib                152
#define LIB_blitlib                150
#define LIB_qblitlib               148
#define LIB_bblitlib               146
#define LIB_blitzcoplib            144
#define LIB_displaylib             143   /* new Aug 28th (bum5) */
#define LIB_fontlib                142
#define LIB_mygadtoolslib          141   /* new Aug 28th (bum5) */
#define LIB_gadgetslib             140
#define LIB_windowslib             138
#define LIB_menuslib               136
#define LIB_intuifontlib           134
#define LIB_jaglib                 139
#define LIB_segalib                137
#define LIB_consolelib             135
#define LIB_brexxlib               132
#define LIB_rawkeylib              130
#define LIB_linklistlib            128
#define LIB_fileiolib              124
#define LIB_stencillib             122
#define LIB_vallib                 120
#define LIB_sis2dlib               119
#define LIB_shapetrixlib           118
#define LIB_blitzoslib             117   /* new for bum8 */
#define LIB_audiolib               116
#define LIB_iffmakelib             114
#define LIB_spriteslib             112
#define LIB_sortlib                110
#define LIB_mouselib               108
#define LIB_cliargslib             106
#define LIB_collslib               104
#define LIB_fadelib                102
#define LIB_scrolllib              100
#define LIB_blitzkeyslib            98
#define LIB_trackerlib              96
#define LIB_animlib                 94
#define LIB_arexxlib                92
#define LIB_seriallib               90
#define LIB_speaklib                88
#define LIB_renderlib               87   /* * reserved */
#define LIB_lib3d                   86   /* * reserved */
#define LIB_obj3dlib                84   /* * reserved */
#define LIB_medlib                  82
#define LIB_myasllib                80   /* new Aug 28th (bum5) */
#define LIB_datelib                 78
#define LIB_banklib                 76
#define LIB_chunkylib               72
#define LIB_cvlib                   71
#define LIB_romuluscrunchlib        74
#define LIB_romulusprtlib           70
#define LIB_romulusrequesterlib     69
#define LIB_romuluslocalelib        68
#define LIB_romulusconsolelib       67
#define LIB_aaronsiconlib           62   /* in development by Aaron */
#define LIB_emllib                  60   /* in development by paul reece */
#define LIB_neilsciatrklib          56   /* replaced by xbonesciatrklib for bb2.1 */
#define LIB_neilsindicatorlib       55
#define LIB_neilsreqtoolslib        54
#define LIB_fuzziesreqlib           53   /* another req lib??? */
#define LIB_elmoreinclib           111   /* Rich Elmore Libraries */
#define LIB_elmoredoslib           109
#define LIB_elmoresyslib           107
#define LIB_elmoremathlib          105
#define LIB_elmorehardwarelib      103
#define LIB_elmorefuncslib         101
#define LIB_rifnslib                93   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_ritrackdisklib          91   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_ricommoditieslib        89   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_rireqlib                79   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_riamosfunclib           75   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_ripacklib               73   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_rifxlib                 66   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_rigfxlib                65   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_rizonejoylib            64   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_riapplib                61   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_ritooltypes             59   /* *** NEW FROM REFLECTIVE IMAGES *** */
#define LIB_rianimlib               57
#define LIB_riencryptlib            52

/* not in libnums.bb: the numbers libraries give literally */
/* fadelib: !libheader{157,...}; #fadelib = 102 above is used by no library */
#define LIB_fadelib_header         157
/* polylib: #chunkylib=121 in its source; #chunkylib above is 72 */
#define LIB_polylib                121
/* deflibs colorwheel.gadget; libnums.bb: #cdtvlib=219 (reserved) */
#define LIB_colorwheellib          219
/* deflibs mathtrans.library: #mathtranslib=225 in libnums.bb, redefined
   there to 192 (the command library mathtranslib); deflibs keeps 225 */
#define LIB_mathtransoslib         225
/* deflibs xprzmodem.library (#xprzlib=218 is reserved, unused) */
#define LIB_xprzmodemlib             2

#endif
