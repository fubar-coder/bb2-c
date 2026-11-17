; ridoslib-getpathname-first-char (#624) - GetPathName searches the last
; / or : from the end with a counter that starts at the length + 1 and is
; decremented before each character is read: when it reaches 0 the
; search ends without looking at the first character.  A path whose only
; separator is its first character gives "", like a name without one.
; Correct: GetPathName(":dir") is ":", GetPathName("/abc") is "/";
; ":c/dir" (a separator after the first character) is ":c/" in both.
; @fixed #624
; @rwe ridoslib
; @pass timeout=30
NPrint "[",GetPathName(":dir"),"]"
NPrint "[",GetPathName("/abc"),"]"
NPrint "[",GetPathName(":c/dir"),"]"
End
