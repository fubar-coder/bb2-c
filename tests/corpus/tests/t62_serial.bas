; t62_serial - seriallib with serial.device unit 0 of the emulator (no
; remote end, no loopback): open, parameters, buffer, query/read with
; nothing waiting, SerialEvent, failing opens, a few bytes written with
; DoIO, close.  Offsets: u = read half (request, port +4, IOExtSer +38),
; u+128 = write half.
Dim m.l(15)
b.l=&m(0)
For i=0 To 15 : m(i)=$41424344 : Next
u.l=OpenSerial("serial.device",0,31250,$80)
NPrint "open ",u<>0," request ",Peek.l(u)=u+38," write ",Peek.l(u+128)=u+166
NPrint "port type ",Peek.b(u+12)," reply ",Peek.l(u+52)=u+4," len ",Peek.w(u+56)
NPrint "same device ",Peek.l(u+58)=Peek.l(u+186)," sigs differ ",Peek.b(u+19)<>Peek.b(u+147)
NPrint "baud ",Peek.l(u+98)," serflags ",Hex$(Peek.b(u+117) AND 255)," err ",Peek.b(u+69)
NPrint "lens ",Peek.b(u+114)," ",Peek.b(u+115)," ",Peek.b(u+116)," rbuf ",Peek.l(u+90)
NPrint "read ",ReadSerial(0)," ",ReadSerial(0,0)," actual ",Peek.l(u+70)
s$=ReadSerialString(0)
NPrint "readstring [",s$,"] ",Len(s$)
NPrint "event ",SerialEvent(0)
SetSerialBuffer 0,4096
NPrint "buffer ",Peek.l(u+90)," err ",Peek.b(u+69)
SetSerialLens 0,7,7,2
NPrint "lens ",Peek.b(u+114)," ",Peek.b(u+115)," ",Peek.b(u+116)," err ",Peek.b(u+69)
SetSerialLens 0,8,8,1
Poke.l u+98,9600
SetSerialParams 0
NPrint "params ",Peek.l(u+98)," err ",Peek.b(u+69)
u1.l=OpenSerial("serial.device",1,9600,0)
NPrint "unit 1 ",u1," ",Peek.l(u+256)
u2.l=OpenSerial("nosuch.device",2,9600,0)
NPrint "no device ",u2
WriteSerial 0,65
NPrint "write err ",Peek.b(u+128+69)," actual ",Peek.l(u+128+70)
WriteSerialString 0,"hello",2
NPrint "string err ",Peek.b(u+128+69)," len ",Peek.l(u+128+74)," actual ",Peek.l(u+128+70)
WriteSerialMem 0,b,6
NPrint "mem err ",Peek.b(u+128+69)," len ",Peek.l(u+128+74)," actual ",Peek.l(u+128+70)
CloseSerial 0
NPrint "closed ",Peek.l(u)," ",Peek.l(u+128)
CloseSerial 1
CloseSerial 0
NPrint "done"
End
