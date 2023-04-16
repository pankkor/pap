; ========================================================================
;
; (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.
;
; This software is provided 'as-is', without any express or implied
; warranty. In no event will the authors be held liable for any damages
; arising from the use of this software.
;
; Please see https://computerenhance.com for further information
;
; ========================================================================

; ========================================================================
; LISTING 42
; ========================================================================

;
; NOTE(casey): This is not meant to be a real compliance test for 8086
; disassemblers. It's just a reasonable selection of opcodes and patterns
; to use as a first pass in making sure a disassembler handles a large
; cross-section of the encoding. To be absolutely certain you haven't
; missed something, you would need a more exhaustive listing!
;

bits 16

retf 17556
ret 17560
retf
ret

call [bp+si-0x3a]
call far [bp+si-0x3a]
jmp [di]
jmp far [di]

jmp 21862:30600
