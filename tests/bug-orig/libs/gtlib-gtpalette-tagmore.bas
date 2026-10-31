; gtlib-gtpalette-tagmore (#168) - GTPalette with a colour writes it to
; +38 of its static tag list (a layout with two more tags): in the binary
; that is the low word of the TAG_MORE pointer to the GTTags list, not the
; value of GTPA_Color.  The colour is ignored, and a GTTags list given
; before a later GTPalette is reached through the wrong pointer.
; On the Workbench screen: GTPalette with colour 3, its GTPA_Color
; ($80080011, GTGetAttrs into a variable: #328); then GTTags
; GTPA_ColorOffset ($80080012) 2 and a GTPalette without colour, its
; GTPA_ColorOffset and GTPA_Color.
; Correct: colour 3; offset 2 and colour 0 (GTPalette without a colour
; gives GTPA_Color 0, as it always did).  The original gives colour 0 and
; loses the GTTags list (offset 0).
; @configs clib
DEFTYPE .l
WbToScreen 0
GTPalette 0,1,10,10,100,40,"P",0,2,3
c1=GTGetAttrs(0,1,$80080011)
GTTags $80080012,2
GTPalette 0,2,10,60,100,40,"Q",0,2
o2=GTGetAttrs(0,2,$80080012)
c2=GTGetAttrs(0,2,$80080011)
NPrint "colour ",c1
NPrint "offset ",o2
NPrint "colour ",c2
Free GTList 0
End
