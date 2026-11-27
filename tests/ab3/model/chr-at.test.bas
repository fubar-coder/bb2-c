; chr-at - character literals among AB3 mode's other forms with @ (spec
; ab3-compat/strings): CNIF @#name (defined?), @@ (line joining) behind a
; literal (after a ',') and inside one, a literal of the character @. The
; case prints the values (asserted in chr-at.test.toml): AmiBlitz3 joins
; with @@ in NPrint's list but not in the braces of a call, so the joined
; line is the NPrint itself.
OPTIMIZE 4
!TestCase{chr_at}
  #chr_guard = 1
  CNIF @#chr_guard = 1
    a.l = @"OK"
  CEND
  CNIF @#chr_none = 1
    a = 0
  CEND
  NPrint Hex$(a)
  NPrint @"A", " ", @@
    @"B"
  c.l = @"@@AB"
  NPrint Hex$(c)
  NPrint Hex$(@"@")
!EndTestCase
