; gtlib-a3-tag-list (#328) - GTGetAttrs and GTSetAttrs (and GTChangeList,
; GTSetHighlight, GTSetString, GTSetInteger) pass their tag list to
; GadTools in a3, the write pointer of the string work area, and leave
; a3 there.  As a function in a string expression the rest of the
; expression is then written at the tag list in the library, not behind
; the part before: the result has the right length but other contents.
; On the Workbench screen two cycle gadgets (active 2 and 1); the
; characters of the results as codes.
; Correct: Str$(GTGetAttrs(...))+"/"+Str$(GTGetAttrs(...)) is "2/1"
; (50 47 49), "v"+Str$(GTSetAttrs(...))+"w" is "v0w" (118 48 119).
; @fixed #328
; @configs clib
DEFTYPE .l
WbToScreen 0
GTCycle 0,1,10,10,100,14,"A",0,"x|y|z",2
GTCycle 0,2,10,30,100,14,"B",0,"p|q|r",1
s$=Str$(GTGetAttrs(0,1,$8008000F))+"/"+Str$(GTGetAttrs(0,2,$8008000F))
NPrint Len(s$)," ",Asc(Mid$(s$,1))," ",Asc(Mid$(s$,2))," ",Asc(Mid$(s$,3))
s$="v"+Str$(GTSetAttrs(0,1,$8008000F,0))+"w"
NPrint Len(s$)," ",Asc(Mid$(s$,1))," ",Asc(Mid$(s$,2))," ",Asc(Mid$(s$,3))
NPrint GTGetAttrs(0,1,$8008000F)
Free GTList 0
End
