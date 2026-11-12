; spriteslib-loadsprites-word-offset (#999) - LoadSprites Sprite#,File$
; loads from the given sprite up to the last sprite of the object table,
; computed as table + (maximum - 1) << 4 with a word add: from a maximum
; of 2049 on the last sprite lies before the table, and only the first
; sprite is loaded.
; Here: the sprite maximum is raised to 2100; three sprites of 8, 9 and 10
; lines are saved, freed and loaded again with LoadSprites 0,File$
; (sprites 0 to 2 lie at the start of the table: the compiler addresses
; them alike with and without #939).  Printed: the height word (+4, lines
; + 1) of a sprite with data, else "-".
; Correct: all three sprites are back.
; @fixed #999
; @pass jobline=max=112|2100
DEFTYPE .l
Statement heights{t$}
  Print t$
  For s=0 To 2
    a=Addr Sprite(s)
    If Peek.l(a) Then Print " ",Peek.w(a+4) Else Print " -"
  Next
  NPrint ""
End Statement
BitMap 0,16,10,2
Use BitMap 0
Boxf 0,0,15,9,3
For s=0 To 2
  GetaShape s,0,0,16,8+s
  GetaSprite s,s
Next
heights{"made"}
SaveSprites 0,2,"RAM:bo999.spr"
For s=0 To 2 : Free Sprite s : Next
heights{"freed"}
LoadSprites 0,"RAM:bo999.spr"
heights{"loadsprites"}
KillFile "RAM:bo999.spr"
End
