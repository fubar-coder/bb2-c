; ridoslib-addnotify-allocmem-request (#619) - AddNotify copies the file
; name into AllocMem(length + 1) and builds the NotifyRequest in
; AllocMem(48) without testing either result.  The program runs through
; inject, which fails the first allocation of 48 bytes (the request):
; the request is built at address 0 (name, full name, id, flags and port
; over the longs 0-16, the ExecBase pointer at 4 included) and passed to
; StartNotify, and the name copy is never freed.  The program saves the
; first 12 longs before the call and would put them back after it, but
; the original halts the system before it prints anything.
; Correct: AddNotify fails (0) without writing to low memory and frees
; the name copy (no allocation left at the end).
; @fixed #619
; @rwe ridoslib
; @pass runwrap=@INJECT@|allocfail=48-48/1 track
DEFTYPE .l
Dim s(11)
For i=0 To 11 : s(i)=Peek.l(i*4) : Next
r=AddNotify(1,"a/b")
c=0
For i=0 To 11
  If Peek.l(i*4)<>s(i) Then c=c+1
  Poke.l i*4,s(i)
Next
NPrint "result ",r
If c=0 Then NPrint "low memory kept" Else NPrint "low memory written"
End
