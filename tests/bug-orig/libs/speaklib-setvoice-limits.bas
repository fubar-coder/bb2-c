; speaklib-setvoice-limits (#279) - SetVoice limits rate (40-400), pitch
; (65-320), volume (at most 64) and sample frequency (5000-28000) with a
; word CMP and BPL/BMI, the sign of the difference without its overflow:
; a large negative value whose difference to the limit overflows counts
; as above the limit and becomes the maximum.  The values are read back
; from the narrator request in front of VoiceLoc, as t113_speak does.
; Correct: -32768 is below every minimum and stays below the maximum:
; rate 40, pitch 65, sample frequency 5000; the volume has no minimum and
; keeps -32768.  -32000 overflows only the difference to the frequency's
; minimum (below -27768): rate 40, pitch 65, volume -32000, frequency
; 5000.  32767 gives the maxima.
; @fixed #279
; @configs clib
DEFTYPE .l
Statement sv{a.l,b.l,e.l,f.l}
  SetVoice a,b,1,0,e,f
  v.l=VoiceLoc
  NPrint "rate ",Peek.w(v-21)," pitch ",Peek.w(v-19)," vol ",Peek.w(v-7)," freq ",Peek.w(v-5)
End Statement
sv{-32768,-32768,-32768,-32768}
sv{-32000,-32000,-32000,-32000}
sv{32767,32767,32767,32767}
End
