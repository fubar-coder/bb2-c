; rimisclib-fontprefs-search-behind-file (#745) - DefaultFontHeight and
; DefaultFontName look for a font type by stepping 164 bytes (the size of
; a FONT chunk) from the first FONT chunk to the end of the 16 KB buffer
; of OpenFontPrefs, not to the end of the file: behind the file the
; buffer holds what the memory held before (AllocMem without
; MEMF_CLEAR), and a type that is not in the file is "found" there.  The
; program fills a 16 KB block with the words 3, frees it, so that
; OpenFontPrefs gets that memory, and asks for type 3 (the font prefs of
; the emulated system have the types 0 to 2).
; Correct: type 3 is not in the file: DefaultFontHeight(3) is 0.
; @fixed #745
; @rwe rimisclib
; @pass timeout=30
DEFTYPE .l
m=AllocMem_(16384,0)
If m=0 Then NPrint "no memory" : End
For i=0 To 16380 Step 4 : Poke.l m+i,$00030003 : Next
FreeMem_ m,16384
x.w=OpenFontPrefs
h.w=DefaultFontHeight(3)
CloseFontPrefs
NPrint x," ",h
End
