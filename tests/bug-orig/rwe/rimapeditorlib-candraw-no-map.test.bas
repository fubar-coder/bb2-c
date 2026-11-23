; rimapeditorlib-candraw-no-map (#893) - MapCanDraw works on the used
; map without looking whether it has a map: after Free MapObject (or a
; CreateMap that could not allocate the data) the object is the used map
; with width, height and data 0, and MapCanDraw answers -1 for every
; position, as if the position were outside a map.  A command on a map
; that does not exist; the original has no error check area.  Here with
; runtime error checking.
; Correct: the error check refuses MapCanDraw on the freed map:
; "MapObject has no map data".
DEFTYPE .l

!TestCase{freed_map}
  CreateMap 0,4,4,16,16,"m"
  Free MapObject 0
  !ExpectError{"MapObject has no map data","refused"}
  r=MapCanDraw(3,1,1)
  !Check{0,"not_refused"}
!EndTestCase
