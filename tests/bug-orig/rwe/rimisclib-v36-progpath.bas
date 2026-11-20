; rimisclib-v36-progpath (#1150) - ProgPath$, ProgDir$ and ProgName$
; call GetProgramDir, GetCurrentDirName, NameFromLock and GetProgramName
; (dos V36) with dos.library opened in any version: on Kickstart 1.3 (dos
; V34) the calls lie behind dos's jump table, the machine crashes (the
; session ends in a timeout).  The program makes the directory RAM:pp/sub,
; makes it its current directory and calls ProgPath$ there, then ProgDir$
; and ProgName$ (they keep what ProgPath$ found).  The session starts the
; program from CORPUS:out, the shell's command name is that path.
; Correct: below Kickstart 2.0 the directory is the current directory,
; its path built from the directory names ("RAM DISK:pp/sub"), the name
; the last part of the shell's command name.
; @fixed #1150
; @rwe rimisclib
; @machine a500
; @pass timeout=30
DEFTYPE .l
NPrint "before"
k=CreateDir_("RAM:pp")
If k Then UnLock_ k
k=CreateDir_("RAM:pp/sub")
If k Then UnLock_ k
k=Lock_("RAM:pp/sub",-2)
If k=0 Then NPrint "no directory" : End
o=CurrentDir_(k)
p$=ProgPath$
k=CurrentDir_(o)
UnLock_ k
NPrint p$
NPrint ProgDir$
NPrint ProgName$
NPrint "after"
End
