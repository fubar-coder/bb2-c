; audiolib-loadsound-novhdr-fileopen: an 8SVX without a VHDR chunk (only
; a BODY): LoadSound's search for VHDR fails
form 8SVX
chunk BODY
fill 16 0
end
end
