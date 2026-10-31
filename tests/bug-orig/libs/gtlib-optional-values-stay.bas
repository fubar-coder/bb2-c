; gtlib-optional-values-stay (#169) - the optional values of the gadget
; creators (GTCycle/GTMX active, GTScroller Top, GTSlider Level, GTString
; default$, GTListView selected/top) are written into static tag lists of
; the library; the forms without them do not reset them, so a later
; gadget without the value gets the one of the previous gadget.
; On the Workbench screen GTCycle, GTMX and GTString twice each, first
; with the value, then without; the values of both (GTGetAttrs into a
; variable: #328; for GTString its text, of the second its length).
; GTScroller/GTSlider are fixed the same way but not part of the test
; (GTGetAttrs of GTSC_Top/GTSL_Level returns nothing with this GadTools),
; nor is GTListView (it needs a list in the GadTools layout).
; Correct: the first gadgets have their values, the second ones 0 and an
; empty string.  The original gives the second ones the values of the
; first ones (cycle 2, mx 1, the string a pointer into the old string
; work area).
; @configs clib
DEFTYPE .l
WbToScreen 0
GTCycle 0,1,10,10,100,14,"A",0,"x|y|z",2
GTCycle 0,2,10,30,100,14,"B",0,"p|q|r"
GTMX 0,3,10,50,20,10,"C",0,"x|y|z",1
GTMX 0,4,140,50,20,10,"D",0,"p|q|r"
GTString 0,9,10,180,100,14,"I",0,20,"abc"
GTString 0,10,10,200,100,14,"J",0,20
a1=GTGetAttrs(0,1,$8008000F)
a=GTGetAttrs(0,2,$8008000F)
b1=GTGetAttrs(0,3,$8008000A)
b=GTGetAttrs(0,4,$8008000A)
e=GTGetAttrs(0,9,$8008002D)
f=GTGetAttrs(0,10,$8008002D)
NPrint "cycle ",a1," ",a
NPrint "mx ",b1," ",b
If e Then NPrint "string 1 ",Peek$(e)
If f Then NPrint "string 2 ",Len(Peek$(f)) Else NPrint "string 2 none"
Free GTList 0
End
