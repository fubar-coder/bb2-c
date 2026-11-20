; ritooltypeslib-v36-calls-none: a tool icon (.info, DiskObject version 1)
; of 16 x 8 pixels, one plane, without a default tool and without tool
; types (do_ToolTypes 0: icon.library gives the DiskObject a NULL array)
w $E310 1               ; do_Magic, do_Version
l 0                     ; do_Gadget: NextGadget
w 0 0 16 8              ; LeftEdge, TopEdge, Width, Height
w 4 3 1                 ; Flags (GADGIMAGE), Activation, GadgetType (BOOLGADGET)
l 1 0 0 0 0             ; GadgetRender (present), SelectRender, GadgetText, MutualExclude, SpecialInfo
w 0                     ; GadgetID
l 0                     ; UserData
b 3 0                   ; do_Type (WBTOOL), pad
l 0 0                   ; do_DefaultTool (none), do_ToolTypes (none)
l $80000000 $80000000   ; do_CurrentX, do_CurrentY (no position)
l 0 0 4096              ; do_DrawerData, do_ToolWindow, do_StackSize
; the Image of GadgetRender
w 0 0 16 8 1            ; LeftEdge, TopEdge, Width, Height, Depth
l 1                     ; ImageData (present)
b 1 0                   ; PlanePick, PlaneOnOff
l 0                     ; NextImage
w $FFFF $8001 $8001 $8001 $8001 $8001 $8001 $FFFF
