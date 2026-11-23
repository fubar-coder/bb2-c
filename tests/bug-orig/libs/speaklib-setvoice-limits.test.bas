; speaklib-setvoice-limits (#279) - SetVoice limits rate (40-400), pitch
; (65-320), volume (at most 64) and sample frequency (5000-28000) with a
; word CMP and BPL/BMI, the sign of the difference without its overflow:
; a large negative value whose difference to the limit overflows counts
; as above the limit and becomes the maximum.  The values are read back
; from the narrator request in front of VoiceLoc, as t113_speak does.
; Here: SetVoice with -32768, -32000 and 32767 for all four values.
; Correct: -32768 is below every minimum and stays below the maximum:
; rate 40, pitch 65, sample frequency 5000; the volume has no minimum and
; keeps -32768.  -32000 overflows only the difference to the frequency's
; minimum (below -27768): rate 40, pitch 65, volume -32000, frequency
; 5000.  32767 gives the maxima.
DEFTYPE .l
Statement sv{a.l,b.l,e.l,f.l}
  SetVoice a,b,1,0,e,f
End Statement

!TestCase{minus_32768}
  sv{-32768,-32768,-32768,-32768}
  v.l=VoiceLoc
  !CheckEq{Peek.w(v-21),40,"rate"}
  !CheckEq{Peek.w(v-19),65,"pitch"}
  !CheckEq{Peek.w(v-7),-32768,"volume"}
  !CheckEq{Peek.w(v-5),5000,"frequency"}
!EndTestCase

!TestCase{minus_32000}
  sv{-32000,-32000,-32000,-32000}
  v.l=VoiceLoc
  !CheckEq{Peek.w(v-21),40,"rate"}
  !CheckEq{Peek.w(v-19),65,"pitch"}
  !CheckEq{Peek.w(v-7),-32000,"volume"}
  !CheckEq{Peek.w(v-5),5000,"frequency"}
!EndTestCase

!TestCase{plus_32767}
  sv{32767,32767,32767,32767}
  v.l=VoiceLoc
  !CheckEq{Peek.w(v-21),400,"rate"}
  !CheckEq{Peek.w(v-19),320,"pitch"}
  !CheckEq{Peek.w(v-7),64,"volume"}
  !CheckEq{Peek.w(v-5),28000,"frequency"}
!EndTestCase
