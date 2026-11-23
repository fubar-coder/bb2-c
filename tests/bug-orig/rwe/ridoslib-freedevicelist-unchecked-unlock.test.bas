; ridoslib-freedevicelist-unchecked-unlock (#623) - FreeDeviceList calls
; UnLockDosList without testing that the list is locked: a second
; FreeDeviceList after GetDeviceList unlocks once more.  dos.library
; (V47) then releases di_DevLock a second time - exec's ReleaseSemaphore
; sees its nest count go below 0, raises the recoverable alert
; AN_SemCorrupt ($01000008) and initialises the semaphore again - and
; calls Permit for the Forbid of LockDosList: the task's Forbid nest
; count (ExecBase TDNestCnt) ends one below "not forbidden", every later
; Forbid of the task is without effect.  inject's alertlog records the
; alert (exec would show it later, at a random moment); the program
; compares TDNestCnt and puts it back, so the system goes on.
; Correct: the second FreeDeviceList does nothing: no alert, the Forbid
; nest count as before.
DEFTYPE .l

!TestCase{second_free}
  x=Peek.l(4)
  a.b=Peek.b(x+295)
  If GetDeviceList(4)
    FreeDeviceList
  EndIf
  b.b=Peek.b(x+295)
  FreeDeviceList
  c.b=Peek.b(x+295)
  Poke.b x+295,b
  !CheckEq{b,a,"first_nest_count"}
  !CheckEq{c,b,"second_nest_count"}
!EndTestCase
