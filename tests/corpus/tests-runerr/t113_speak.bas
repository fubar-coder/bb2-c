; t113_speak - speaklib: init (translator.library + narrator.device; if one
; is missing: runtime error "Failed To Open Speech Library"), VoiceLoc,
; SetVoice (clipping, values in the narrator request before VoiceLoc),
; Translate$ (phonemes, length includes the 0).  No Speak (would play).
DEFTYPE .l
NPrint "start"
v=VoiceLoc
NPrint "voiceloc odd ",v AND 1
Statement sv{a.l,b.l,c.l,d.l,e.l,f.l}
  SetVoice a,b,c,d,e,f
  v.l=VoiceLoc
  Print "voice ",Peek.w(v-21)," ",Peek.w(v-19)," ",Peek.w(v-17)," ",Peek.w(v-15)," ",Peek.w(v-7)
  NPrint " ",Peek.w(v-5)," masks ",Peek.w(v-9)
End Statement
sv{150,110,1,0,64,22200}
sv{10,20,2,3,100,1000}
sv{500,400,5,7,-5,30000}
sv{40,65,0,1,63,5000}
sv{399,319,1,1,64,27999}
sv{-32768,32767,-1,-1,32767,-32768}
Statement tr{s$}
  t$=Translate$(s$)
  NPrint "tr [",s$,"] len ",Len(t$)," last ",Asc(Right$(t$,1))," [",Left$(t$,Len(t$)-1),"]"
End Statement
tr{"hello"}
tr{"Blitz Basic two"}
tr{""}
tr{"1234"}
tr{"The quick brown fox jumps over the lazy dog."}
NPrint "end"
End
