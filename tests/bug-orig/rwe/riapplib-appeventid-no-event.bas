; riapplib-appeventid-no-event (#1102) - AppEventID returns 0 when there
; is no current App event, but RIAppLib.guide (node AppEventID) says "-1
; means that no AppEvent occurred"; 0 is also the id of an object (the ids
; of AddAppIcon, AddAppWindow and AddAppMenu start at 0).  The program
; reads AppEventID before any AppEvent, after an AppEvent that found no
; message, for an App message of its own with id 0 (am_ID 0, no
; arguments) and after the AppEvent that replied to it.
; Correct: -1 without an event, 0 for the message of id 0.
; @rwe riapplib
DEFTYPE .l
NPrint "before AppEvent: ",AppEventID
r=AppEvent
NPrint "no message: ",AppEventID
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.b m+8,5 : Poke.w m+18,40
Poke.w m+20,9 : Poke.l m+26,0
PutMsg_ AppMsgPort,m
If AppEvent Then NPrint "message of id ",AppEventID Else NPrint "no message"
r=AppEvent
NPrint "after the last message: ",AppEventID
FreeMem_ m,64
End
