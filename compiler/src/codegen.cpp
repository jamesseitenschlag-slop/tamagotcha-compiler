// codegen.cpp
// created: Feb 07, 2025
// last modified: Feb 12, 2025
// James Ezra Seitenschlag
// codegen.cpp - Codegenerator-Definitionen + Runtime-Bibliothek
#include "codegen.hpp"
#include "sprites_p1.hpp"

static const char* RTR_PRINT_HEX = R"ASM(
__print_hex:
    LD X, 0x3C
    LD A, MX
    LD X, 0xC0
    CP A, 0xA
    JP C, ph_lo
    LD B, 0x9          ; low nibble = n-9 (e.g. 'A'=0x41 -> low 1)
    SUB A, B
    LD MX, A
    INC X
    LD A, 0x4
    LD MX, A
    JP ph_nul
ph_lo:
    LD MX, A
    INC X
    LD A, 0x3
    LD MX, A
ph_nul:
    INC X
    LD MX, 0x0
    INC X
    LD MX, 0x0
    JP __dbg_trigger
)ASM";

static const char* RTR_PRINT_DEC = R"ASM(
__print_dec:
    LD X, 0x3C
    LD A, MX
    LD X, 0xC0
    CP A, 0xA
    JP C, pd_one
    LD MX, 0x1
    INC X
    LD A, 0x3
    LD MX, A
    INC X
    LD X, 0x3C
    LD A, MX
    LD B, 0xA
    SUB A, B
    LD X, 0xC2
    LD MX, A
    INC X
    LD A, 0x3
    LD MX, A
    INC X
    LD MX, 0x0
    INC X
    LD MX, 0x0
    JP __dbg_trigger
pd_one:
    LD MX, A
    INC X
    LD A, 0x3
    LD MX, A
    INC X
    LD MX, 0x0
    INC X
    LD MX, 0x0
    JP __dbg_trigger
)ASM";

static const char* RTR_READ_KEY = R"ASM(
; read_key: A = gedrueckte Tasten als logische Maske.
; Hardware-Verdrahtung (TamaLIB): K02=LEFT(A), K01=MIDDLE(B), K00=RIGHT(C)
; -> logisch: Bit0=A(LEFT), Bit1=B(MIDDLE), Bit2=C(RIGHT)
__read_key:
    LD A, 0xF
    LD XP, A
    LD X, 0x40
    LD A, MX
    NOT A
    AND A, 0x7          ; Bit2=LEFT, Bit1=MID, Bit0=RIGHT
    LD X, 0x3D
    LD MX, A            ; orig sichern (Scratch 0x3D)
    LD A, 0x0           ; Ergebnis aufbauen
    LD X, 0x3D
    FAN MX, 0x2
    JP Z, rk1
    OR A, 0x2           ; MIDDLE -> Bit1
rk1:
    LD X, 0x3D
    FAN MX, 0x4
    JP Z, rk2
    OR A, 0x1           ; LEFT(A) -> Bit0
rk2:
    LD X, 0x3D
    FAN MX, 0x1
    JP Z, rk3
    OR A, 0x4           ; RIGHT(C) -> Bit2
rk3:
    LD B, 0x0
    LD XP, B            ; XP zurueck auf 0
    RET
)ASM";

static const char* RTR_DBG_TRIGGER = R"ASM(
__dbg_trigger:
    LD A, 0xF
    LD XP, A
    LD X, 0x0E
    LD A, 0x1
    LD MX, A
    LD A, 0x0
    LD XP, A
    RET
)ASM";

static const char* RTR_HEXNIB = R"ASM(
; __hexnib: A (0..15) als Hex-Ziffer an Y schreiben (Y += 2), RET
__hexnib:
    CP A, 0xA
    JP C, hn_lo
    LD B, 0x9
    SUB A, B
    LD MY, A
    INC Y
    LD A, 0x4
    LD MY, A
    INC Y
    RET
hn_lo:
    LD MY, A
    INC Y
    LD A, 0x3
    LD MY, A
    INC Y
    RET
)ASM";

static const char* RTR_PRINT_INT = R"ASM(
; __print_int: X -> 2 Nibbles [hi,lo], dezimal 0..255 ausgeben
; M0=hi, M1=lo, M2=hunderter, M3=zehner
__print_int:
    LD A, MX
    LD M0, A
    INC X
    LD A, MX
    LD M1, A
    LD A, 0x0
    LD M2, A
    LD M3, A
pi100:
    LD A, M1
    LD B, 0x4
    SUB A, B
    LD A, M0
    LD B, 0x6
    SBC A, B
    JP C, pi10
    LD A, M1
    LD B, 0x4
    SUB A, B
    LD M1, A
    LD A, M0
    LD B, 0x6
    SBC A, B
    LD M0, A
    INC M2
    JP pi100
pi10:
    LD A, M1
    LD B, 0xA
    SUB A, B
    LD A, M0
    LD B, 0x0
    SBC A, B
    JP C, piout
    LD A, M1
    LD B, 0xA
    SUB A, B
    LD M1, A
    LD A, M0
    LD B, 0x0
    SBC A, B
    LD M0, A
    INC M3
    JP pi10
piout:
    LD Y, 0xC0
    LD A, M2
    CP A, 0x0
    JP NZ, pi3
    LD A, M3
    CP A, 0x0
    JP NZ, pi2
    JP pi1
pi3:
    LD A, M2
    CALL pidig
    LD A, M3
    CALL pidig
    LD A, M1
    JP piw
pi2:
    LD A, M3
    CALL pidig
pi1:
    LD A, M1
piw:
    CALL pidig
    LD A, 0x0
    LD MY, A
    INC Y
    LD MY, A
    JP __dbg_trigger
pidig:
    LD MY, A
    INC Y
    LD A, 0x3
    LD MY, A
    INC Y
    RET
)ASM";

static const char* RTR_PRINT_HEX8 = R"ASM(
; __print_hex8: X -> 2 Nibbles [hi,lo] als 2 Hex-Ziffern
__print_hex8:
    LD Y, 0xC0
    LD A, MX
    CALL __hexnib
    INC X
    LD A, MX
    CALL __hexnib
    LD A, 0x0
    LD MY, A
    INC Y
    LD MY, A
    JP __dbg_trigger
)ASM";

static const char* RTR_PRINT_HEX16 = R"ASM(
; __print_hex16: X -> 4 Nibbles (big-endian) als 4 Hex-Ziffern
__print_hex16:
    LD Y, 0xC0
    LD A, MX
    CALL __hexnib
    INC X
    LD A, MX
    CALL __hexnib
    INC X
    LD A, MX
    CALL __hexnib
    INC X
    LD A, MX
    CALL __hexnib
    LD A, 0x0
    LD MY, A
    INC Y
    LD MY, A
    JP __dbg_trigger
)ASM";

static const char* RTR_PRINT_INT16 = R"ASM(
; __print_int16: X -> 4 Nibbles (big-endian), dezimal 0..65535
; M0..M3 = Zahl (M0=MSB), M4..M7 = Subtrahend, M8 = Zaehler, M9 = Flag
__print_int16:
    LD A, MX
    LD M0, A
    INC X
    LD A, MX
    LD M1, A
    INC X
    LD A, MX
    LD M2, A
    INC X
    LD A, MX
    LD M3, A
    LD Y, 0xC0
    LD A, 0x0
    LD M8, A
    LD M9, A
    ; ---- 10000er: d = 2,7,1,0 ----
    LD A, 0x2
    LD M4, A
    LD A, 0x7
    LD M5, A
    LD A, 0x1
    LD M6, A
    LD A, 0x0
    LD M7, A
p10k_l:
    CALL pge
    JP C, w10k
    CALL psub
    INC M8
    JP p10k_l
w10k:
    LD A, M8
    CP A, 0x0
    JP NZ, w10k_w
    LD A, M9
    CP A, 0x0
    JP Z, p1k
    LD A, 0x0
w10k_w:
    CALL wdig
    JP p1k
wdig:
    LD MY, A
    INC Y
    LD A, 0x3
    LD MY, A
    INC Y
    LD A, 0x1
    LD M9, A
    RET
    ; ---- 1000er: d = 0,3,E,8 ----
p1k:
    LD A, 0x0
    LD M4, A
    LD A, 0x3
    LD M5, A
    LD A, 0xE
    LD M6, A
    LD A, 0x8
    LD M7, A
    LD A, 0x0
    LD M8, A
p1k_l:
    CALL pge
    JP C, w1k
    CALL psub
    INC M8
    JP p1k_l
w1k:
    LD A, M8
    CP A, 0x0
    JP NZ, w1k_w
    LD A, M9
    CP A, 0x0
    JP Z, p100
    LD A, 0x0
w1k_w:
    CALL wdig
    ; ---- 100er: d = 0,0,6,4 ----
p100:
    LD A, 0x0
    LD M4, A
    LD A, 0x0
    LD M5, A
    LD A, 0x6
    LD M6, A
    LD A, 0x4
    LD M7, A
    LD A, 0x0
    LD M8, A
p100_l:
    CALL pge
    JP C, w100
    CALL psub
    INC M8
    JP p100_l
w100:
    LD A, M8
    CP A, 0x0
    JP NZ, w100_w
    LD A, M9
    CP A, 0x0
    JP Z, p10
    LD A, 0x0
w100_w:
    CALL wdig
    ; ---- 10er: d = 0,0,0,A ----
p10:
    LD A, 0x0
    LD M4, A
    LD A, 0x0
    LD M5, A
    LD A, 0x0
    LD M6, A
    LD A, 0xA
    LD M7, A
    LD A, 0x0
    LD M8, A
p10_l:
    CALL pge
    JP C, w10
    CALL psub
    INC M8
    JP p10_l
w10:
    LD A, M8
    CP A, 0x0
    JP NZ, w10_w
    LD A, M9
    CP A, 0x0
    JP Z, p_ein
    LD A, 0x0
w10_w:
    CALL wdig
p_ein:
    LD A, M3
    CALL pidig
    LD A, 0x0
    LD MY, A
    INC Y
    LD MY, A
    JP __dbg_trigger
pidig:
    LD MY, A
    INC Y
    LD A, 0x3
    LD MY, A
    INC Y
    RET
pge:
    ; C = 1 wenn M0..M3 < M4..M7
    LD A, M3
    LD B, M7
    SUB A, B
    LD A, M2
    LD B, M6
    SBC A, B
    LD A, M1
    LD B, M5
    SBC A, B
    LD A, M0
    LD B, M4
    SBC A, B
    RET
psub:
    LD A, M3
    LD B, M7
    SUB A, B
    LD M3, A
    LD A, M2
    LD B, M6
    SBC A, B
    LD M2, A
    LD A, M1
    LD B, M5
    SBC A, B
    LD M1, A
    LD A, M0
    LD B, M4
    SBC A, B
    LD M0, A
    RET
)ASM";

static const char* RTR_INC16 = R"ASM(
; __inc16: X -> 4 Nibbles (big-endian) += 1
__inc16:
    LD A, XH
    LD M8, A
    LD A, XL
    LD M9, A
    LD A, MX
    LD M0, A
    INC X
    LD A, MX
    LD M1, A
    INC X
    LD A, MX
    LD M2, A
    INC X
    LD A, MX
    LD M3, A
    LD A, M3
    ADD A, 0x1
    LD M3, A
    CP A, 0x0
    JP NZ, i16w
    LD A, M2
    ADD A, 0x1
    LD M2, A
    CP A, 0x0
    JP NZ, i16w
    LD A, M1
    ADD A, 0x1
    LD M1, A
    CP A, 0x0
    JP NZ, i16w
    LD A, M0
    ADD A, 0x1
    LD M0, A
i16w:
    LD A, M8
    LD XH, A
    LD A, M9
    LD XL, A
    LD A, M0
    LD MX, A
    INC X
    LD A, M1
    LD MX, A
    INC X
    LD A, M2
    LD MX, A
    INC X
    LD A, M3
    LD MX, A
    RET
)ASM";

static const char* RTR_ADD16 = R"ASM(
; __add16: X -> a (big-endian), Y -> b ; a += b
__add16:
    LD A, XH
    LD M8, A
    LD A, XL
    LD M9, A
    LD A, MX
    LD M0, A
    INC X
    LD A, MX
    LD M1, A
    INC X
    LD A, MX
    LD M2, A
    INC X
    LD A, MX
    LD M3, A
    LD A, MY
    LD M4, A
    INC Y
    LD A, MY
    LD M5, A
    INC Y
    LD A, MY
    LD M6, A
    INC Y
    LD A, MY
    LD M7, A
    LD A, M3
    LD B, M7
    ADD A, B
    LD M3, A
    LD A, M2
    LD B, M6
    ADC A, B
    LD M2, A
    LD A, M1
    LD B, M5
    ADC A, B
    LD M1, A
    LD A, M0
    LD B, M4
    ADC A, B
    LD M0, A
    LD A, M8
    LD XH, A
    LD A, M9
    LD XL, A
    LD A, M0
    LD MX, A
    INC X
    LD A, M1
    LD MX, A
    INC X
    LD A, M2
    LD MX, A
    INC X
    LD A, M3
    LD MX, A
    RET
)ASM";

static const char* RTR_SUB16 = R"ASM(
; __sub16: X -> a (big-endian), Y -> b ; a -= b
__sub16:
    LD A, XH
    LD M8, A
    LD A, XL
    LD M9, A
    LD A, MX
    LD M0, A
    INC X
    LD A, MX
    LD M1, A
    INC X
    LD A, MX
    LD M2, A
    INC X
    LD A, MX
    LD M3, A
    LD A, MY
    LD M4, A
    INC Y
    LD A, MY
    LD M5, A
    INC Y
    LD A, MY
    LD M6, A
    INC Y
    LD A, MY
    LD M7, A
    LD A, M3
    LD B, M7
    SUB A, B
    LD M3, A
    LD A, M2
    LD B, M6
    SBC A, B
    LD M2, A
    LD A, M1
    LD B, M5
    SBC A, B
    LD M1, A
    LD A, M0
    LD B, M4
    SBC A, B
    LD M0, A
    LD A, M8
    LD XH, A
    LD A, M9
    LD XL, A
    LD A, M0
    LD MX, A
    INC X
    LD A, M1
    LD MX, A
    INC X
    LD A, M2
    LD MX, A
    INC X
    LD A, M3
    LD MX, A
    RET
)ASM";

static const char* RTR_CMP16 = R"ASM(
; __cmp16: X -> a, Y -> b (big-endian); A = 1 wenn a < b
__cmp16:
    LD A, MX
    LD M0, A
    INC X
    LD A, MX
    LD M1, A
    INC X
    LD A, MX
    LD M2, A
    INC X
    LD A, MX
    LD M3, A
    LD A, MY
    LD M4, A
    INC Y
    LD A, MY
    LD M5, A
    INC Y
    LD A, MY
    LD M6, A
    INC Y
    LD A, MY
    LD M7, A
    LD A, M0
    LD B, M4
    CP A, B
    JP C, cmplt
    JP NZ, cmpge
    LD A, M1
    LD B, M5
    CP A, B
    JP C, cmplt
    JP NZ, cmpge
    LD A, M2
    LD B, M6
    CP A, B
    JP C, cmplt
    JP NZ, cmpge
    LD A, M3
    LD B, M7
    CP A, B
    JP C, cmplt
cmpge:
    LD A, 0x0
    RET
cmplt:
    LD A, 0x1
    RET
)ASM";

static const char* RTR_MUL8 = R"ASM(
; __mul8: 8x8 -> 16-bit.  X -> a (2 Nibbles, big-endian),
; Y -> b (2 Nibbles).  Ergebnis 16-bit in M0..M3 (M0=MSB).
; M4..M7 = a (16-bit, wird geschoben), M8..M9 = b.
__mul8:
    LD A, 0x0
    LD M0, A
    LD M1, A
    LD M2, A
    LD M3, A
    LD A, 0x0           ; a als 16-bit: M4=0, M5=0
    LD M4, A
    LD M5, A
    LD A, MX            ; a: hi -> M6
    LD M6, A
    INC X
    LD A, MX            ; a: lo -> M7
    LD M7, A
    LD A, MY            ; b: hi
    LD M8, A
    INC Y
    LD A, MY            ; b: lo
    LD M9, A
m8_loop:
    ; b == 0 ?
    LD A, M8
    CP A, 0x0
    JP NZ, m8_step
    LD A, M9
    CP A, 0x0
    JP Z, m8_done
m8_step:
    ; if b & 1: res += a
    LD A, M9
    FAN A, 0x1
    JP Z, m8_noadd
    LD A, M3
    LD B, M7
    ADD A, B
    LD M3, A
    LD A, M2
    LD B, M6
    ADC A, B
    LD M2, A
    LD A, M1
    LD B, M5
    ADC A, B
    LD M1, A
    LD A, M0
    LD B, M4
    ADC A, B
    LD M0, A
m8_noadd:
    ; a <<= 1  (16-bit M4..M7)
    RCF
    LD A, M7
    RLC A
    LD M7, A
    LD A, M6
    RLC A
    LD M6, A
    LD A, M5
    RLC A
    LD M5, A
    LD A, M4
    RLC A
    LD M4, A
    ; b >>= 1  (8-bit M8..M9, MSB zuerst)
    RCF
    LD A, M8
    RRC A
    LD M8, A
    LD A, M9
    RRC A
    LD M9, A
    JP m8_loop
m8_done:
    RET
)ASM";

static const char* RTR_RAND16 = R"ASM(
; __rand16: X -> 4 Nibbles (big-endian); x = (5*x + 1) mod 65536
; LCG (a=5, c=1): volle Periode 2^16.  M4..M7 = 4x-Zwischenergebnis.
__rand16:
    LD A, XH
    LD M8, A
    LD A, XL
    LD M9, A
    LD A, MX
    LD M0, A
    INC X
    LD A, MX
    LD M1, A
    INC X
    LD A, MX
    LD M2, A
    INC X
    LD A, MX
    LD M3, A
    ; M4..M7 = x  (4x-Zwischenspeicher)
    LD A, M0
    LD M4, A
    LD A, M1
    LD M5, A
    LD A, M2
    LD M6, A
    LD A, M3
    LD M7, A
    ; M4..M7 <<= 2
    RCF
    LD A, M7
    RLC A
    LD M7, A
    LD A, M6
    RLC A
    LD M6, A
    LD A, M5
    RLC A
    LD M5, A
    LD A, M4
    RLC A
    LD M4, A
    RCF
    LD A, M7
    RLC A
    LD M7, A
    LD A, M6
    RLC A
    LD M6, A
    LD A, M5
    RLC A
    LD M5, A
    LD A, M4
    RLC A
    LD M4, A
    ; x = 4x + x
    LD A, M3
    LD B, M7
    ADD A, B
    LD M3, A
    LD A, M2
    LD B, M6
    ADC A, B
    LD M2, A
    LD A, M1
    LD B, M5
    ADC A, B
    LD M1, A
    LD A, M0
    LD B, M4
    ADC A, B
    LD M0, A
    ; x += 1
    LD A, M3
    ADD A, 0x1
    LD M3, A
    CP A, 0x0
    JP NZ, r16w
    LD A, M2
    ADD A, 0x1
    LD M2, A
    CP A, 0x0
    JP NZ, r16w
    LD A, M1
    ADD A, 0x1
    LD M1, A
    CP A, 0x0
    JP NZ, r16w
    LD A, M0
    ADD A, 0x1
    LD M0, A
r16w:
    LD A, M8
    LD XH, A
    LD A, M9
    LD XL, A
    LD A, M0
    LD MX, A
    INC X
    LD A, M1
    LD MX, A
    INC X
    LD A, M2
    LD MX, A
    INC X
    LD A, M3
    LD MX, A
    RET
)ASM";

static const char* RTR_PRINT_STR = R"ASM(
; __print_str: X -> RAM-String (byte-weise: lo an X, hi an X+1,
; ...), NUL = lo==0 && hi==0.  Copies to debug buffer (0xC0) and triggers.
__print_str:
    LD Y, 0xC0
ps_l:
    LD A, MX            ; lo
    CP A, 0x0
    JP NZ, ps_wr
    ; lo == 0: NUL moeglich -> hi pruefen
    INC X
    LD A, MX
    CP A, 0x0
    JP Z, ps_done
    ; character with lo=0 (rare): write lo=0 and hi!=0
    LD A, 0x0
    LD MY, A
    INC Y
    LD A, MX
    LD MY, A
    INC Y
    INC X
    JP ps_l
ps_wr:
    ; lo (A) nach Y
    LD MY, A
    INC Y
    INC X
    LD A, MX            ; hi
    LD MY, A
    INC Y
    INC X
    JP ps_l
ps_done:
    LD A, 0x0
    LD MY, A
    INC Y
    LD MY, A
    JP __dbg_trigger
)ASM";

static const char* RTR_STRLEN = R"ASM(
; __strlen: X -> RAM string; A = length (characters up to NUL).
__strlen:
    LD A, 0x0
    LD M0, A
sl_l:
    LD A, MX
    CP A, 0x0
    JP NZ, sl_inc
    INC X
    LD A, MX
    CP A, 0x0
    JP Z, sl_done
    INC X               ; character (lo=0, hi!=0)
    INC M0
    JP sl_l
sl_inc:
    INC X
    INC X
    INC M0
    JP sl_l
sl_done:
    LD A, M0
    RET
)ASM";

static const char* RTR_STRCPY = R"ASM(
; __strcpy: X -> dst, Y -> src (RAM-Strings); kopiert bis NUL
; inklusive Terminator.
__strcpy:
sc_l:
    LD A, MY            ; src-lo
    LD MX, A            ; dst-lo
    CP A, 0x0
    JP NZ, sc_next
    ; lo == 0: NUL moeglich
    INC X
    INC Y
    LD A, MY            ; src-hi
    LD MX, A            ; dst-hi
    CP A, 0x0
    JP Z, sc_done       ; NUL kopiert
    INC X
    INC Y
    JP sc_l
sc_next:
    INC X
    INC Y
    LD A, MY            ; src-hi
    LD MX, A            ; dst-hi
    INC X
    INC Y
    JP sc_l
sc_done:
    RET
)ASM";

static const char* RTR_STRCMP = R"ASM(
; __strcmp: X -> a, Y -> b (RAM-Strings).
; A = 0 gleich, 1 wenn a<b, 2 wenn a>b.
__strcmp:
sm_l:
    LD A, MX            ; a-lo
    LD B, MY            ; b-lo
    CP A, B
    JP C, sm_lt
    JP NZ, sm_gt
    ; lo gleich; lo == 0 -> NUL moeglich
    CP A, 0x0
    JP NZ, sm_hi
    ; lo == 0: hi vergleichen
    INC X
    INC Y
    LD A, MX
    LD B, MY
    CP A, B
    JP C, sm_lt
    JP NZ, sm_gt
    LD A, MX
    CP A, 0x0
    JP Z, sm_eq         ; beide NUL -> gleich
    INC X
    INC Y
    JP sm_l
sm_hi:
    INC X
    INC Y
    LD A, MX
    LD B, MY
    CP A, B
    JP C, sm_lt
    JP NZ, sm_gt
    INC X
    INC Y
    JP sm_l
sm_lt:
    LD A, 0x1
    RET
sm_gt:
    LD A, 0x2
    RET
sm_eq:
    LD A, 0x0
    RET
)ASM";

static const char* RTR_MEMSET = R"ASM(
; __memset: X -> dst, M0 = Wert, M1 = n (0..15); schreibt n Nibbles
__memset:
ms_loop:
    LD A, M1
    CP A, 0x0
    JP Z, ms_done
    LD A, M0
    LD MX, A
    INC X
    LD A, M1
    ADD A, 0xF
    LD M1, A
    JP ms_loop
ms_done:
    RET
)ASM";

static const char* RTR_MEMCPY = R"ASM(
; __memcpy: X -> dst, Y -> src, M1 = n (0..15); kopiert n Nibbles
__memcpy:
mc_loop:
    LD A, M1
    CP A, 0x0
    JP Z, mc_done
    LD A, MY
    LD MX, A
    INC X
    INC Y
    LD A, M1
    ADD A, 0xF
    LD M1, A
    JP mc_loop
mc_done:
    RET
)ASM";

static const char* RTR_PRINT_DEC_AT = R"ASM(
; __print_dec_at: 4-bit-Wert aus 0x3C dezimal ab Y schreiben (0..15),
; danach NUL + Debug-Trigger.  Y zeigt auf die naechste freie Position.
__print_dec_at:
    LD X, 0x3C
    LD A, MX
    CP A, 0xA
    JP C, pda_one
    LD A, 0x1               ; Zehner: '1'
    LD MY, A
    INC Y
    LD A, 0x3
    LD MY, A
    INC Y
    LD X, 0x3C
    LD A, MX
    LD B, 0xA
    SUB A, B
    LD MY, A                ; Einer
    INC Y
    LD A, 0x3
    LD MY, A
    INC Y
    JP pda_nul
pda_one:
    LD MY, A
    INC Y
    LD A, 0x3
    LD MY, A
    INC Y
pda_nul:
    LD A, 0x0
    LD MY, A
    INC Y
    LD MY, A
    JP __dbg_trigger
)ASM";

static const char* RTR_CLEAR_VRAM = R"ASM(
__clear_vram:
    LD A, 0xE
    LD XP, A
    LD X, 0x00
cv_loop:
    LD A, 0x0
    LD MX, A
    INC X
    CP XH, 0xD
    JP NZ, cv_loop
    LD A, 0x0
    LD XP, A
    RET
)ASM";

static const char* RTR_SET_ICON = R"ASM(
__set_icon:
    LD A, 0xE
    LD XP, A
    LD X, 0x3C
    LD A, MX
    CP A, 0x4
    JP NC, si_bot
    LD B, 0x1
si_top_l:
    CP A, 0x0
    JP Z, si_top_w
    ADD B, B
    ADD A, 0xF
    JP si_top_l
si_top_w:
    LD X, 0x10
    LD A, B
    LD MX, A
    LD X, 0xB9
    LD A, 0x0
    LD MX, A
    JP si_done
si_bot:
    ADD A, 0xC
    LD B, 0x1
si_bot_l:
    CP A, 0x0
    JP Z, si_bot_w
    ADD B, B
    ADD A, 0xF
    JP si_bot_l
si_bot_w:
    LD X, 0xB9
    LD A, B
    LD MX, A
    LD X, 0x10
    LD A, 0x0
    LD MX, A
si_done:
    LD A, 0x0
    LD XP, A
    RET
)ASM";

const char* runtime_text_def(const char* name) {
    if (strcmp(name, "__print_hex") == 0) return RTR_PRINT_HEX;
    if (strcmp(name, "__print_dec") == 0) return RTR_PRINT_DEC;
    if (strcmp(name, "__print_dec_at") == 0) return RTR_PRINT_DEC_AT;
    if (strcmp(name, "__read_key") == 0) return RTR_READ_KEY;
    if (strcmp(name, "__dbg_trigger") == 0) return RTR_DBG_TRIGGER;
    if (strcmp(name, "__hexnib") == 0) return RTR_HEXNIB;
    if (strcmp(name, "__print_int") == 0) return RTR_PRINT_INT;
    if (strcmp(name, "__print_hex8") == 0) return RTR_PRINT_HEX8;
    if (strcmp(name, "__print_hex16") == 0) return RTR_PRINT_HEX16;
    if (strcmp(name, "__print_int16") == 0) return RTR_PRINT_INT16;
    if (strcmp(name, "__inc16") == 0) return RTR_INC16;
    if (strcmp(name, "__add16") == 0) return RTR_ADD16;
    if (strcmp(name, "__sub16") == 0) return RTR_SUB16;
    if (strcmp(name, "__cmp16") == 0) return RTR_CMP16;
    if (strcmp(name, "__mul8") == 0) return RTR_MUL8;
    if (strcmp(name, "__rand16") == 0) return RTR_RAND16;
    if (strcmp(name, "__print_str") == 0) return RTR_PRINT_STR;
    if (strcmp(name, "__strlen") == 0) return RTR_STRLEN;
    if (strcmp(name, "__strcpy") == 0) return RTR_STRCPY;
    if (strcmp(name, "__strcmp") == 0) return RTR_STRCMP;
    if (strcmp(name, "__memset") == 0) return RTR_MEMSET;
    if (strcmp(name, "__memcpy") == 0) return RTR_MEMCPY;
    if (strcmp(name, "__clear_vram") == 0) return RTR_CLEAR_VRAM;
    if (strcmp(name, "__set_icon") == 0) return RTR_SET_ICON;
    return nullptr;
}
// ===========================================================================


string Gen::newlbl(const string& tag) {
        ++lbl;
        return tag + to_string(lbl);
    }
void Gen::emit(const string& line) { outp->push_back("    " + line); }
void Gen::label(const string& name) { outp->push_back(name + ":"); }
void Gen::rawline(const string& l) { outp->push_back(l); }
void Gen::alloc_var(const string& name, int size) {
        if (vars.count(name))
            throw CompileError("duplicate variable '" + name + "'");
        vars[name] = {var_addr, size};
        varOrder.push_back(name);
        var_addr += size;
    }
void Gen::decl_vars(const vector<pair<string, int>>& vv) {
        for (auto& v : vv) alloc_var(v.first, v.second);
    }
int Gen::var_base(const string& name) {
        auto it = vars.find(name);
        if (it == vars.end())
            throw CompileError("unknown variable '" + name + "'");
        return it->second.first;
    }
void Gen::gen_all(Node& prog) {
        vector<Node> funcs2;
        for (auto& d : prog.kids) {
            if (d.k == K_FUNC) funcs2.push_back(d);
            else if (d.k == K_VARS) {
                vector<pair<string, int>> vv;
                for (auto& v : d.vdecls) vv.push_back(v);
                decl_vars(vv);
            }
        }
        bool hasMain = false;
        for (auto& f : funcs2) {
            funcs.insert(f.s);
            if (f.s == "main") hasMain = true;
        }
        if (!hasMain) throw CompileError("no main() function");

        // phase 1: helpers + main into a scratch buffer
        {
            vector<string> scratch;
            vector<string>* save = outp;
            outp = &scratch;
            emit(".org 0x0000");
            rawline("; ---- helper functions ----");
            for (auto& f : funcs2)
                if (f.s != "main") gen_func(f);
            rawline("; ---- main ----");
            for (auto& f : funcs2)
                if (f.s == "main") gen_func(f);
            outp = save;
            bodyBuf = std::move(scratch);
        }

        // --- Multi-Page-Code-Layout: Funktionen ueber Pages verteilen.
        // Page 0 = 0x000..0x0FF frei; ab Page 1 ist 0x100..0x10F gesperrt
        // (Reset-/Interrupt-Vektoren) -> Funktionen ab 0x110.  Cross-Page-
        // User-CALLs bekommen ein PSET; die Runtime folgt nach dem Code.
        auto cw = [](const vector<string>& lines) -> int {
            int n = 0;
            for (auto& ln : lines) {
                string t = ln;
                size_t b = t.find_first_not_of(" ");
                if (b == string::npos) continue;
                t = t.substr(b);
                size_t e = t.find_last_not_of(" ");
                t = t.substr(0, e + 1);
                if (t.empty() || t[0] == ';') continue;
                if (t.back() == ':') continue;
                string low = t;
                transform(low.begin(), low.end(), low.begin(), ::tolower);
                if (low.rfind(".org", 0) == 0 || low.rfind(".word", 0) == 0 ||
                    low.rfind(".fill", 0) == 0)
                    continue;
                n++;
            }
            return n;
        };
        auto pg = [](int a) { return (a >> 8) & 0xF; };
        auto align_next = [](int cur) -> int {
            int nxt = (cur + 0xFF) & ~0xFF;
            if (nxt == 0x100) nxt = 0x110;
            return nxt;
        };
        // bodyBuf in Funktions-Segmente zerlegen
        map<string, vector<string> > seg;
        string curname;
        vector<string> curbuf;
        for (auto& ln : bodyBuf) {
            string t = ln;
            size_t b = t.find_first_not_of(" ");
            string st = (b == string::npos) ? "" : t.substr(b);
            bool isfn = false;
            if (!st.empty() && st.back() == ':') {
                string nm = st.substr(0, st.size() - 1);
                for (auto& f : funcs2)
                    if (f.s == nm) { isfn = true; break; }
                if (isfn) {
                    if (!curname.empty()) seg[curname] = curbuf;
                    curname = nm;
                    curbuf.clear();
                    continue;
                }
            }
            curbuf.push_back(ln);
        }
        if (!curname.empty()) seg[curname] = curbuf;

        // Cross-Page-User-CALLs pro Funktion zaehlen (gleiche Regel wie die
        // .s-Ausgabe: IMMER ein PSET, wenn das Ziel auf einer anderen Page
        // liegt).  Diese PSETs sind echte Woerter und muessen im Layout
        // beruecksichtigt werden (Iteration bis Fixpunkt), sonst kollidieren
        // Funktionen mit der naechsten.
        auto user_psets = [&](const string& name,
                              const map<string, int>& fa) -> int {
            int n = 0;
            auto sit = seg.find(name);
            if (sit == seg.end()) return 0;
            int cpage = pg(fa.at(name));
            for (auto& ln : sit->second) {
                string t = ln;
                size_t b = t.find_first_not_of(" ");
                string st = (b == string::npos) ? "" : t.substr(b);
                if (st.rfind("CALL ", 0) == 0) {
                    string nm = st.substr(5);
                    size_t sp = nm.find_first_of(" ");
                    if (sp != string::npos) nm = nm.substr(0, sp);
                    auto it = fa.find(nm);
                    if (it != fa.end() && pg(it->second) != cpage) n++;
                }
            }
            return n;
        };
        auto lay_out = [&](const map<string, int>& extra,
                           int* endCur) -> map<string, int> {
            map<string, int> res;
            int cur = 0x000;
            for (auto& f : funcs2) {
                const string& name = f.s;
                int w = 0;
                auto sit = seg.find(name);
                if (sit != seg.end()) w = cw(sit->second);
                auto ei = extra.find(name);
                if (ei != extra.end()) w += ei->second;
                if (w > 0x100)
                    throw CompileError("Funktion '" + name +
                                       "' too large for a single page (" +
                                       to_string(w) + " Woerter)");
                int lo = cur & 0xFF;
                if (cur >= 0x100 && cur < 0x110) { cur = 0x110; lo = 0x10; }
                if (lo + w > 0x100) {
                    cur = align_next(cur);
                    lo = cur & 0xFF;
                    if (lo + w > 0x100) cur = align_next(cur);
                }
                res[name] = cur;
                cur += w + 1;
            }
            if (endCur) *endCur = cur;
            return res;
        };
        map<string, int> faddr;
        int cur = 0;
        faddr = lay_out(map<string, int>(), &cur);
        for (int it = 0; it < 10; it++) {
            map<string, int> extra;
            for (auto& f : funcs2) extra[f.s] = user_psets(f.s, faddr);
            map<string, int> nxt = lay_out(extra, nullptr);
            if (nxt == faddr) break;
            faddr = nxt;
        }

        // Code ausgeben: .org je Funktion + Label + PSET vor cross-page CALLs
        vector<string> out;
        for (auto& f : funcs2) {
            const string& name = f.s;
            out.push_back("; ---- " + name + " @ 0x" +
                          hexstr((unsigned)faddr[name], 3).substr(2) + " ----");
            out.push_back("    .org 0x" +
                          hexstr((unsigned)faddr[name], 3).substr(2));
            out.push_back(name + ":");
            int cpage = pg(faddr[name]);
            auto sit = seg.find(name);
            if (sit != seg.end()) {
                for (auto& ln : sit->second) {
                    string t = ln;
                    size_t b = t.find_first_not_of(" ");
                    string st = (b == string::npos) ? "" : t.substr(b);
                    if (st.rfind("CALL ", 0) == 0) {
                        string nm = st.substr(5);
                        size_t sp = nm.find_first_of(" ");
                        if (sp != string::npos) nm = nm.substr(0, sp);
                        auto fa = faddr.find(nm);
                        if (fa != faddr.end() && pg(fa->second) != cpage)
                            out.push_back("    PSET 0x" +
                                hexstr((unsigned)pg(fa->second), 1).substr(2));
                    }
                    out.push_back(ln);
                }
            }
        }

        // Runtime-Layout nach dem Code
        set<string> need = used_runtime;
        if (need.count("__print_hex") || need.count("__print_dec") ||
            need.count("__print_int") || need.count("__print_int16") ||
            need.count("__print_hex8") || need.count("__print_hex16") ||
            need.count("__print_str") || need.count("__print_dec_at"))
            need.insert("__dbg_trigger");
        vector<string> needv(need.begin(), need.end());
        sort(needv.begin(), needv.end());
        auto rt_lines = [&](const string& nm) -> vector<string> {
            if (nm == "__disp_digit") return disp_digit_runtime_lines();
            const char* t = runtime_text(nm.c_str());
            if (!t) throw CompileError("unknown runtime '" + nm + "'");
            vector<string> o;
            split_append(t, o);
            return o;
        };
        auto rt_psets = [&](const string& nm,
                            const map<string, int>& ra) -> int {
            int n = 0;
            int cpage = pg(ra.at(nm));
            for (auto& ln : rt_lines(nm)) {
                string t = ln;
                size_t b = t.find_first_not_of(" ");
                string st = (b == string::npos) ? "" : t.substr(b);
                if ((st.rfind("CALL ", 0) == 0 || st.rfind("JP ", 0) == 0) &&
                    !st.empty()) {
                    string lab = st.substr(st.find_last_of(' ') + 1);
                    auto it = ra.find(lab);
                    if (it != ra.end() && pg(it->second) != cpage) n++;
                }
            }
            return n;
        };
        auto lay_rt = [&](const map<string, int>& extra,
                          int cur0) -> map<string, int> {
            map<string, int> res;
            int rcur = align_next(cur0);
            for (auto& nm : needv) {
                int w = cw(rt_lines(nm));
                auto ei = extra.find(nm);
                if (ei != extra.end()) w += ei->second;
                if (w > 0x100)
                    throw CompileError("runtime '" + nm +
                                       "' too large for a single page (" +
                                       to_string(w) + " Woerter)");
                if (rcur >= 0x100 && rcur < 0x110) rcur = 0x110;
                if ((rcur & 0xFF) + w > 0x100) {
                    rcur = align_next(rcur);
                    if (rcur >= 0x100 && rcur < 0x110) rcur = 0x110;
                    if ((rcur & 0xFF) + w > 0x100) rcur = align_next(rcur);
                }
                res[nm] = rcur;
                rcur += w + 2;
            }
            return res;
        };
        map<string, int> raddr = lay_rt(map<string, int>(), cur);
        for (int it = 0; it < 10; it++) {
            map<string, int> extra;
            for (auto& nm : needv) extra[nm] = rt_psets(nm, raddr);
            map<string, int> nxt = lay_rt(extra, cur);
            if (nxt == raddr) break;
            raddr = nxt;
        }
        // PSET vor Runtime-CALLs im Code auf die echte Seite patchen
        vector<string> out2;
        string prev = "";
        for (auto& line : out) {
            string st = line;
            size_t b = st.find_first_not_of(" ");
            st = (b == string::npos) ? "" : st.substr(b);
            if (st.rfind("CALL ", 0) == 0 && prev == "PSET 0x1") {
                string nm = st.substr(5);
                size_t sp = nm.find_first_of(" ");
                if (sp != string::npos) nm = nm.substr(0, sp);
                auto ra = raddr.find(nm);
                if (ra != raddr.end())
                    out2.back() = "    PSET 0x" +
                        hexstr((unsigned)pg(ra->second), 1).substr(2);
            }
            out2.push_back(line);
            prev = st;
        }
        if (!needv.empty()) {
            out2.push_back("");
            out2.push_back("    .org 0x110   ; runtime library");
            for (auto& nm : needv) {
                vector<string> lines = rt_lines(nm);
                int cpage = pg(raddr[nm]);
                out2.push_back("; ---- " + nm + " @ 0x" +
                    hexstr((unsigned)raddr[nm], 3).substr(2) + " ----");
                out2.push_back("    .org 0x" +
                    hexstr((unsigned)raddr[nm], 3).substr(2));
                for (auto& ln : lines) {
                    string st = ln;
                    size_t b = st.find_first_not_of(" ");
                    st = (b == string::npos) ? "" : st.substr(b);
                    if ((st.rfind("CALL ", 0) == 0 || st.rfind("JP ", 0) == 0) &&
                        !st.empty()) {
                        string lab = st.substr(st.find_last_of(' ') + 1);
                        auto ra = raddr.find(lab);
                        if (ra != raddr.end() && pg(ra->second) != cpage)
                            out2.push_back("    PSET 0x" +
                                hexstr((unsigned)pg(ra->second), 1).substr(2));
                    }
                    out2.push_back(ln);
                }
            }
        }
        outp = nullptr;
        finalAsm = std::move(out2);
    }
void Gen::split_append(const char* text, vector<string>& out) {
        std::istringstream ss(text);
        string line;
        while (std::getline(ss, line)) out.push_back(line);
    }
vector<string> Gen::disp_digit_runtime_lines() {
        vector<string> L;
        L.push_back("; display_digit: RAM[0x3E] (0-9) an SEG 12-16 / COM8-15");
        L.push_back(";   Bytes bei 0xE98..0xEA0 (XP=0xE, X low = 0x98..0xA0)");
        L.push_back("__disp_digit:");
        L.push_back("    LD A, 0xE");
        L.push_back("    LD XP, A");
        L.push_back("    LD X, 0x98");
        for (int i = 0; i < 5; i++) L.push_back("    LBPX MX, 0x00");
        L.push_back("    LD A, 0x0");
        L.push_back("    LD XP, A");
        L.push_back("    LD X, 0x3E");
        L.push_back("    LD A, MX");
        for (int d = 0; d < 10; d++) {
            char buf[32];
            snprintf(buf, sizeof buf, "    CP A, 0x%X", d);
            L.push_back(buf);
            L.push_back("    JP Z, __d" + to_string(d));
        }
        L.push_back("    JP __disp_ret");
        for (int d = 0; d < 10; d++) {
            vector<int> vals = digit_bytes(d);
            L.push_back("__d" + to_string(d) + ":");
            L.push_back("    LD A, 0xE");
            L.push_back("    LD XP, A");
            L.push_back("    LD X, 0x98");
            for (int v : vals) {
                char buf[32];
                snprintf(buf, sizeof buf, "    LBPX MX, 0x%02X", v);
                L.push_back(buf);
            }
            L.push_back("    JP __disp_ret");
        }
        L.push_back("__disp_ret:");
        L.push_back("    LD A, 0x0");
        L.push_back("    LD XP, A");
        L.push_back("    RET");
        return L;
    }
vector<int> Gen::digit_bytes(int d) {
        char ch = (char)('0' + d);
        const std::array<int, 7>* g = glyph(ch);
        vector<int> out;
        for (int sx = 0; sx < 5; sx++) {
            int low = 0, high = 0;
            for (int sy = 0; sy < 7; sy++) {
                if ((*g)[sy] & (1 << (4 - sx))) {
                    if (sy < 4) low |= 1 << sy;
                    else high |= 1 << (sy - 4);
                }
            }
            out.push_back(low | (high << 4));
        }
        return out;
    }
const char* Gen::runtime_text(const char* name) {
        return runtime_text_def(name);
    }
static string norm_line(const string& l) {
    size_t sc = l.find(';');
    string s = (sc == string::npos) ? l : l.substr(0, sc);
    string res;
    bool space = false;
    for (char c : s) {
        if (isspace((unsigned char)c)) {
            if (!res.empty()) space = true;
        } else {
            if (space) { res += ' '; space = false; }
            res += (char)toupper((unsigned char)c);
        }
    }
    return res;
}

vector<string> Gen::peephole_optimize(const vector<string>& lines) {
    vector<string> cur = lines;
    bool changed = true;
    while (changed) {
        changed = false;
        vector<string> new_lines;
        int known_x = -1;
        size_t i = 0;
        while (i < cur.size()) {
            const string& l = cur[i];
            string nl = norm_line(l);
            if (nl.empty()) {
                new_lines.push_back(l);
                i++;
                continue;
            }
            if (nl.back() == ':') {
                known_x = -1;
                new_lines.push_back(l);
                i++;
                continue;
            }

            // Rule: LD MX, A followed by LD A, MX
            if (nl == "LD MX, A") {
                size_t j = i + 1;
                while (j < cur.size() && norm_line(cur[j]).empty()) j++;
                if (j < cur.size() && norm_line(cur[j]) == "LD A, MX") {
                    new_lines.push_back(l);
                    cur.erase(cur.begin() + j);
                    changed = true;
                    i++;
                    continue;
                }
            }

            // Rule: LD X, imm (redundant load)
            if (nl.rfind("LD X, ", 0) == 0) {
                string arg = nl.substr(6);
                size_t b = arg.find_first_not_of(" ");
                if (b != string::npos) arg = arg.substr(b);
                int val = -1;
                try {
                    if (arg.rfind("0X", 0) == 0) val = std::stoi(arg, nullptr, 16);
                    else val = std::stoi(arg, nullptr, 10);
                } catch (...) {
                    val = -1;
                }
                if (val >= 0) {
                    if (known_x == val) {
                        changed = true;
                        i++;
                        continue;
                    } else {
                        known_x = val;
                        new_lines.push_back(l);
                        i++;
                        continue;
                    }
                }
            }

            // Check if instruction clobbers X
            static const char* prefixes[] = {
                "INC X", "LBPX", "LDPX", "ACPX", "SCPX",
                "LD XP", "LD XH", "LD XL", "POP XP", "POP XH", "POP XL", "POP X",
                "CALL", "JP", "RET", ".ORG"
            };
            for (const char* p : prefixes) {
                if (nl.rfind(p, 0) == 0) {
                    known_x = -1;
                    break;
                }
            }

            // Rule: JP to next label
            if (nl.rfind("JP ", 0) == 0) {
                string target;
                if (nl.rfind("JP Z, ", 0) == 0) target = nl.substr(6);
                else if (nl.rfind("JP NZ, ", 0) == 0) target = nl.substr(7);
                else if (nl.rfind("JP C, ", 0) == 0) target = nl.substr(6);
                else if (nl.rfind("JP NC, ", 0) == 0) target = nl.substr(7);
                else if (nl.find(',') == string::npos) target = nl.substr(3);
                size_t tb = target.find_first_not_of(" ");
                if (tb != string::npos) target = target.substr(tb);
                if (!target.empty()) {
                    size_t j = i + 1;
                    while (j < cur.size() && norm_line(cur[j]).empty()) j++;
                    if (j < cur.size() && norm_line(cur[j]) == target + ":") {
                        changed = true;
                        i++;
                        continue;
                    }
                }
            }

            // Rule: Dead code after unconditional JP or RET
            bool is_uncond_jp = (nl.rfind("JP ", 0) == 0 &&
                                 nl.rfind("JP Z,", 0) != 0 &&
                                 nl.rfind("JP NZ,", 0) != 0 &&
                                 nl.rfind("JP C,", 0) != 0 &&
                                 nl.rfind("JP NC,", 0) != 0);
            if (nl == "RET" || is_uncond_jp) {
                new_lines.push_back(l);
                size_t j = i + 1;
                while (j < cur.size()) {
                    string nj = norm_line(cur[j]);
                    if (nj.empty()) { j++; continue; }
                    if (nj.back() == ':' || nj.rfind(".ORG", 0) == 0) break;
                    j++;
                    changed = true;
                }
                i = j;
                known_x = -1;
                continue;
            }

            new_lines.push_back(l);
            i++;
        }
        cur = std::move(new_lines);
    }
    return cur;
}

void Gen::gen_func(Node& f) {
    vector<string> buf;
    vector<string>* save_out = outp;
    outp = &buf;
    label(f.s);
    gen_block(f.kids[0]);
    emit("RET");
    outp = save_out;
    buf = peephole_optimize(buf);
    for (auto& ln : buf) rawline(ln);
}
void Gen::gen_block(Node& b) {
    for (auto& s : b.kids) gen_stmt(s);
}
void Gen::gen_stmt(Node& s) {
    switch (s.k) {
        case K_BLOCK: gen_block(s); break;
        case K_EMPTY: break;
        case K_LOCALVARS: {
            vector<pair<string, int>> vv;
            for (auto& v : s.vdecls) vv.push_back(v);
            decl_vars(vv);
            break;
        }
        case K_EXPR: gen_expr(s.kids[0], true); break;
        case K_IF: {
            Node& cond = s.kids[0];
            if (cond.k == K_NUM) {
                if (cond.iv != 0) gen_stmt(s.kids[1]);
                else if (s.o2) gen_stmt(s.kids[2]);
            } else {
                string L1 = newlbl("L"), L2 = newlbl("L");
                emit_branch_if_false(cond, L1);
                gen_stmt(s.kids[1]);
                if (s.o2) {
                    emit("JP " + L2);
                    label(L1);
                    gen_stmt(s.kids[2]);
                    label(L2);
                } else {
                    label(L1);
                }
            }
            break;
        }
        case K_WHILE: {
            Node& cond = s.kids[0];
            if (cond.k == K_NUM) {
                if (cond.iv != 0) {
                    string top = newlbl("W"), end = newlbl("W");
                    brk.push_back(end); cont.push_back(top);
                    label(top);
                    gen_stmt(s.kids[1]);
                    emit("JP " + top);
                    label(end);
                    brk.pop_back(); cont.pop_back();
                }
            } else {
                string top = newlbl("W"), end = newlbl("W");
                brk.push_back(end); cont.push_back(top);
                label(top);
                emit_branch_if_false(cond, end);
                gen_stmt(s.kids[1]);
                emit("JP " + top);
                label(end);
                brk.pop_back(); cont.pop_back();
            }
            break;
        }
        case K_DO: {
            string top = newlbl("W"), end = newlbl("W");
            string clbl = newlbl("W");
            brk.push_back(end); cont.push_back(clbl);
            label(top);
            gen_stmt(s.kids[0]);
            label(clbl);
            Node& c = s.kids[1];
            if (c.k == K_NUM && c.iv != 0) emit("JP " + top);
            else if (c.k == K_NUM) { /* do-while(0): einmal */ }
            else {
                emit_branch_if_false(c, end);
                emit("JP " + top);
            }
            label(end);
            brk.pop_back(); cont.pop_back();
            break;
        }
        case K_SWITCH: {
            string end = newlbl("S");
            brk.push_back(end);
            gen_expr(s.kids[0]);
            vector<string> labels;
            size_t defidx = (size_t)-1;
            for (size_t i = 1; i < s.kids.size(); i++) {
                Node& cs = s.kids[i];
                string L = newlbl("C");
                labels.push_back(L);
                if (cs.iv < 0) defidx = i - 1;
                else {
                    if (cs.iv > 15)
                        throw CompileError("case value must be 0..15");
                    emit("CP A, " + hexstr((unsigned)cs.iv, 1));
                    emit("JP Z, " + L);
                }
            }
            if (defidx == (size_t)-1) emit("JP " + end);
            else emit("JP " + labels[defidx]);
            for (size_t i = 1; i < s.kids.size(); i++) {
                label(labels[i - 1]);
                Node& cs = s.kids[i];
                for (auto& st : cs.kids) gen_stmt(st);
            }
            label(end);
            brk.pop_back();
            break;
        }
        case K_BREAK:
            if (brk.empty())
                throw CompileError("'break' nur in Schleife/switch");
            emit("JP " + brk.back());
            break;
        case K_CONTINUE:
            if (cont.empty())
                throw CompileError("'continue' nur in Schleife");
            emit("JP " + cont.back());
            break;
        case K_FOR: {
            int ci = 0;
            if (s.o0) gen_expr(s.kids[ci++], true);
            string top = newlbl("F"), end = newlbl("F");
            string inclbl = s.o2 ? newlbl("F") : top;
            brk.push_back(end); cont.push_back(inclbl);
            label(top);
            if (s.o1) {
                Node& cond = s.kids[ci++];
                if (cond.k == K_NUM) {
                    if (cond.iv == 0) { /* never runs */ }
                } else {
                    emit_branch_if_false(cond, end);
                }
            }
            gen_stmt(s.kids.back());
            if (s.o2) {
                label(inclbl);
                gen_expr(s.kids[ci++], true);
            }
            emit("JP " + top);
            label(end);
            brk.pop_back(); cont.pop_back();
            break;
        }
        case K_RETURN:
            if (s.o0) gen_expr(s.kids[0]);
            emit("RET");
            break;
        default:
            throw CompileError("unknown statement");
    }
}
void Gen::gen_expr(Node& e, bool discard) {
        switch (e.k) {
            case K_NUM:
                emit("LD A, " + hexstr((unsigned)e.iv));
                break;
            case K_VAR: load_a_var(e.s, 0); break;
            case K_ARR: load_a_var(e.s, (int)e.iv); break;
            case K_CALL: emit("CALL " + e.s); break;
            case K_PRINT: gen_dbgprint(e.s); break;
            case K_PRINTNUM: {
                string rt = e.s == "print_hex" ? "__print_hex" : "__print_dec";
                used_runtime.insert(rt);
                gen_expr(e.kids[0]);
                emit("LD X, 0x3C");
                emit("LD MX, A");
                emit("PSET 0x1");
                emit("CALL " + rt);
                break;
            }
            case K_DRAWTEXT: gen_drawtext(e.has_ix, (int)e.ix, (int)e.iy, e.s); break;
            case K_DRAWSPRITE: gen_drawsprite(e.s); break;
            case K_SETICON: gen_seticon(e.kids[0]); break;
            case K_CLEARICONS: gen_clearicons(); break;
            case K_CLEARVRAM: gen_clearvram(); break;
            case K_DISPDIGIT:
                used_runtime.insert("__disp_digit");
                gen_expr(e.kids[0]);
                emit("LD X, 0x3E");
                emit("LD MX, A");
                emit("PSET 0x1");
                emit("CALL __disp_digit");
                break;
            case K_PRINTARR: gen_arrfn(e.s, e.names); break;
            case K_READKEY:
                used_runtime.insert("__read_key");
                emit("PSET 0x1");
                emit("CALL __read_key");
                break;
            case K_UN: {
                gen_expr(e.kids[0]);
                if (e.s == "~") emit("NOT A");
                else if (e.s == "-") {
                    // -A = ~A + 1 (2er-Komplement mod 16)
                    emit("NOT A");
                    emit("ADD A, 0x1");
                } else {   // !
                    string L1 = newlbl("N"), L2 = newlbl("N");
                    emit("CP A, 0x0");
                    emit("JP NZ, " + L1);
                    emit("LD A, 0x1");
                    emit("JP " + L2);
                    label(L1);
                    emit("LD A, 0x0");
                    label(L2);
                }
                break;
            }
            case K_BIN: gen_binop(e.s, e.kids[0], e.kids[1]); break;
            case K_CMP: gen_cmp(e.s, e.kids[0], e.kids[1]); break;
            case K_DEREF:
                load_pointer_x(e.s);
                emit("LD A, MX");
                break;
            case K_SETTEXT: gen_settext(e.s, e.s2); break;
            case K_ASSERTSTMT: gen_assert(e.kids[0]); break;
            case K_PRINTMSG:
                used_runtime.insert("__print_dec_at");
                emit("LD A, 0x0");
                emit("LD XP, A");
                emit("LD X, 0xC0");
                for (unsigned char ch : e.s)
                    emit("LBPX MX, " + hexstr(ch & 0xFF, 2));
                gen_expr(e.kids[0]);
                emit("LD X, 0x3C");
                emit("LD MX, A");
                emit("LD Y, " + hexstr((unsigned)(0xC0 + 2 * e.s.size()), 2));
                emit("PSET 0x1");
                emit("CALL __print_dec_at");
                break;
            case K_MEMFN:
                gen_expr(e.kids[1]);                  // n
                emit("LD M1, A");
                if (e.s == "memset") {
                    gen_expr(e.kids[0]);              // val
                    emit("LD M0, A");
                    emit("LD X, " + hexstr((unsigned)var_base(e.s2), 2));
                    used_runtime.insert("__memset");
                    emit("PSET 0x1");
                    emit("CALL __memset");
                } else {
                    emit("LD X, " + hexstr((unsigned)var_base(e.s2), 2));
                    emit("LD Y, " + hexstr((unsigned)var_base(e.kids[0].s), 2));
                    used_runtime.insert("__memcpy");
                    emit("PSET 0x1");
                    emit("CALL __memcpy");
                }
                break;
            case K_ADDR:
                throw CompileError("'&name' only allowed in assignment (p = &name)");
            case K_ASSIGN: {
                Node& lhs = e.kids[0];
                Node& rhs = e.kids[1];
                // Optimierung: x = x op c -> ALU auf RAM[X] in-place
                if (rhs.k == K_BIN && rhs.kids[1].k == K_NUM &&
                    (rhs.s == "+" || rhs.s == "-" || rhs.s == "&" ||
                     rhs.s == "|" || rhs.s == "^") &&
                    (rhs.kids[0].k == K_VAR || rhs.kids[0].k == K_ARR) &&
                    lhs.k == rhs.kids[0].k &&
                    lhs.s == rhs.kids[0].s &&
                    (lhs.k == K_VAR || lhs.iv == rhs.kids[0].iv)) {
                    int addr = (lhs.k == K_VAR) ? var_base(lhs.s)
                                                : var_base(lhs.s) + (int)lhs.iv;
                    int v = (int)rhs.kids[1].iv & 0xF;
                    string op = rhs.s;
                    if (op == "-") { op = "+"; v = (16 - v) & 0xF; }
                    bool noop = (op == "+" && v == 0) ||
                                (op == "&" && v == 15) ||
                                ((op == "|" || op == "^") && v == 0);
                    emit("LD X, " + hexstr((unsigned)addr, 2));
                    if (!noop) {
                        const char* m = op == "+" ? "ADD" : op == "&" ? "AND" :
                                        op == "|" ? "OR" : "XOR";
                        emit(string(m) + " MX, " + hexstr((unsigned)v, 1));
                    }
                    if (!discard) emit("LD A, MX");
                } else if ((lhs.k == K_VAR || lhs.k == K_ARR) && rhs.k == K_NUM) {
                    int addr = (lhs.k == K_VAR) ? var_base(lhs.s)
                                                : var_base(lhs.s) + (int)lhs.iv;
                    int v = (int)rhs.iv & 0xF;
                    emit("LD X, " + hexstr((unsigned)addr, 2));
                    emit("LD MX, " + hexstr((unsigned)v, 1));
                    if (!discard) emit("LD A, " + hexstr((unsigned)v, 1));
                } else if (rhs.k == K_ADDR) {
                    // p = &name : Zieladresse (2 Nibbles) direkt in lhs
                    int target = var_base(e.kids[1].s);
                    int paddr;
                    if (e.kids[0].k == K_VAR) paddr = var_base(e.kids[0].s);
                    else if (e.kids[0].k == K_ARR)
                        paddr = var_base(e.kids[0].s) + (int)e.kids[0].iv;
                    else throw CompileError("Ziel von '= &name' must be a variable");
                    emit("LD A, " + hexstr((unsigned)(target >> 4), 1));
                    emit("LD X, " + hexstr((unsigned)paddr, 2));
                    emit("LD MX, A");
                    emit("LD A, " + hexstr((unsigned)(target & 0xF), 1));
                    emit("LD X, " + hexstr((unsigned)paddr + 1, 2));
                    emit("LD MX, A");
                } else if (e.kids[0].k == K_DEREF) {
                    // *p = e : e in temp, Zeiger nach Y, dann MY schreiben
                    int t = push_temp();
                    gen_expr(e.kids[1]);
                    emit("LD X, " + hexstr((unsigned)t, 2));
                    emit("LD MX, A");
                    load_pointer_y(e.kids[0].s);
                    emit("LD X, " + hexstr((unsigned)t, 2));
                    emit("LD A, MX");
                    pop_temp();
                    emit("LD MY, A");
                } else {
                    gen_expr(e.kids[1]);
                    store_a(e.kids[0]);
                }
                break;
            }
            case K_PREINC:
            case K_POSTINC: {
                // direct memory inc/dec on the variable
                if (e.kids[0].k != K_VAR)
                    throw CompileError("++/-- only on variables");
                int a = var_base(e.kids[0].s);
                emit("LD X, " + hexstr((unsigned)a, 2));
                emit(e.s == "++" ? "ADD MX, 0x1" : "ADD MX, 0xF");
                if (!discard) emit("LD A, MX");
                break;
            }
            default:
                throw CompileError("unknown expression");
        }
    }
void Gen::gen_binop(const string& op, Node& l, Node& r) {
        // A = l op r (4-bit, mod 16) - optimiert: Konstanten, ohne temp
        auto addr_of = [&](Node& e) -> int {
            if (e.k == K_VAR) return var_base(e.s);
            if (e.k == K_ARR) return var_base(e.s) + (int)e.iv;
            throw CompileError("not a var or array");
        };
        // Konstanten-Faltung
        if (l.k == K_NUM && r.k == K_NUM) {
            int a = (int)l.iv, b = (int)r.iv, v = 0;
            if (op == "+") v = (a + b) & 0xF;
            else if (op == "-") v = (a - b) & 0xF;
            else if (op == "&") v = a & b;
            else if (op == "|") v = a | b;
            else if (op == "^") v = a ^ b;
            emit("LD A, " + hexstr((unsigned)v, 1));
            return;
        }
        // rechter Operand konstant
        if (r.k == K_NUM) {
            int v = (int)r.iv & 0xF;
            if (op == "-") v = (16 - v) & 0xF;
            if ((op == "+" || op == "-") && v == 0) { gen_expr(l); return; }
            if (op == "&" && v == 0xF) { gen_expr(l); return; }
            gen_expr(l);
            if (op == "&" && v == 0) { emit("LD A, 0x0"); return; }
            if (v == 0) return;
            const char* m = op == "+" ? "ADD" : op == "-" ? "ADD" :
                            op == "&" ? "AND" : op == "|" ? "OR" : "XOR";
            emit(string(m) + " A, " + hexstr((unsigned)v, 1));
            return;
        }
        // beide einfache Variablen: B = r, A = l
        if ((l.k == K_VAR || l.k == K_ARR) && (r.k == K_VAR || r.k == K_ARR)) {
            emit("LD X, " + hexstr((unsigned)addr_of(r), 2));
            emit("LD B, MX");
            emit("LD X, " + hexstr((unsigned)addr_of(l), 2));
            emit("LD A, MX");
            alu_instr(op);
            return;
        }
        // linker Operand konstant, kommutativ
        if (l.k == K_NUM && (op == "+" || op == "&" || op == "|" || op == "^")) {
            emit("LD B, " + hexstr((unsigned)(l.iv & 0xF), 1));
            gen_expr(r);
            alu_instr(op);
            return;
        }
        // allgemeiner Fall
        int t = push_temp();
        gen_expr(l);
        emit("LD X, " + hexstr((unsigned)t, 2));
        emit("LD MX, A");
        gen_expr(r);
        emit("LD B, A");
        emit("LD X, " + hexstr((unsigned)t, 2));
        emit("LD A, MX");
        pop_temp();
        alu_instr(op);
    }
bool Gen::cmp_simple(Node& c) {
        return (c.kids[0].k == K_NUM || c.kids[0].k == K_VAR || c.kids[0].k == K_ARR) &&
               (c.kids[1].k == K_NUM || c.kids[1].k == K_VAR || c.kids[1].k == K_ARR);
    }
void Gen::emit_branch_if_false(Node& cond, const string& skip) {
    auto addr_of = [&](Node& e) -> int {
        if (e.k == K_VAR) return var_base(e.s);
        if (e.k == K_ARR) return var_base(e.s) + (int)e.iv;
        throw CompileError("not a var or array");
    };

    // Fall 1: Bit-Test: (x & mask) != 0 oder (x & mask) == 0
    if (cond.k == K_CMP && (cond.s == "==" || cond.s == "!=")) {
        string op = cond.s;
        Node* l = &cond.kids[0];
        Node* r = &cond.kids[1];
        if (l->k == K_NUM && l->iv == 0) std::swap(l, r);
        if (r->k == K_NUM && r->iv == 0) {
            if (l->k == K_BIN && l->s == "&") {
                Node* bl = &l->kids[0];
                Node* br = &l->kids[1];
                if (bl->k == K_NUM && (br->k == K_VAR || br->k == K_ARR)) std::swap(bl, br);
                if ((bl->k == K_VAR || bl->k == K_ARR) && br->k == K_NUM) {
                    int addr = addr_of(*bl);
                    int mask = (int)br->iv & 0xF;
                    emit("LD X, " + hexstr((unsigned)addr, 2));
                    emit("FAN MX, " + hexstr((unsigned)mask, 1));
                    emit(op == "!=" ? "JP Z, " + skip : "JP NZ, " + skip);
                    return;
                }
            }
            // Allgemeines expr != 0 oder expr == 0
            gen_expr(*l);
            emit("CP A, 0x0");
            emit(op == "!=" ? "JP Z, " + skip : "JP NZ, " + skip);
            return;
        }
    }

    // Fall 2: reiner Bit-Test: if (x & mask)
    if (cond.k == K_BIN && cond.s == "&") {
        Node* bl = &cond.kids[0];
        Node* br = &cond.kids[1];
        if (bl->k == K_NUM && (br->k == K_VAR || br->k == K_ARR)) std::swap(bl, br);
        if ((bl->k == K_VAR || bl->k == K_ARR) && br->k == K_NUM) {
            int addr = addr_of(*bl);
            int mask = (int)br->iv & 0xF;
            emit("LD X, " + hexstr((unsigned)addr, 2));
            emit("FAN MX, " + hexstr((unsigned)mask, 1));
            emit("JP Z, " + skip);
            return;
        }
    }

    // Fall 3: einfacher Vergleich beider Operanden (var/arr/num)
    if (cond.k == K_CMP && cmp_simple(cond)) {
        emit_cmp_branch(cond, skip);
        return;
    }

    // Fall 4: allgemeiner Vergleich beliebiger Ausdruecke
    if (cond.k == K_CMP) {
        string op = cond.s;
        bool swap = (op == "<=");
        Node& a = swap ? cond.kids[1] : cond.kids[0];
        Node& b = swap ? cond.kids[0] : cond.kids[1];
        int t = push_temp();
        gen_expr(a);
        emit("LD X, " + hexstr((unsigned)t, 2));
        emit("LD MX, A");
        gen_expr(b);
        emit("LD B, A");
        emit("LD X, " + hexstr((unsigned)t, 2));
        emit("LD A, MX");
        pop_temp();
        if (op == "==" || op == "!=") {
            emit("CP A, B");
            emit(op == "==" ? "JP NZ, " + skip : "JP Z, " + skip);
            return;
        }
        emit("SUB A, B");
        if (!swap) {
            if (op == "<") emit("JP NC, " + skip);
            else if (op == ">") { emit("JP C, " + skip); emit("JP Z, " + skip); }
            else emit("JP C, " + skip); // >=
        } else {
            emit("JP C, " + skip); // <=
        }
        return;
    }

    // Fall 5: Wahrheitstest auf beliebigem Ausdruck (if (expr))
    gen_expr(cond);
    emit("CP A, 0x0");
    emit("JP Z, " + skip);
}
void Gen::emit_cmp_branch(Node& c, const string& skip) {
        // Flags fuer Vergleich c setzen; springt zu 'skip', wenn Bedingung FALSCH.
        string op = c.s;
        Node& l = c.kids[0];
        Node& r = c.kids[1];
        bool swap = (op == "<=");            // a<=b <=> nicht(b<a)
        Node& a = swap ? r : l;
        Node& b = swap ? l : r;
        auto addr_of = [&](Node& e) -> int {
            if (e.k == K_VAR) return var_base(e.s);
            if (e.k == K_ARR) return var_base(e.s) + (int)e.iv;
            throw CompileError("not a var or array");
        };
        if (a.k == K_NUM) emit("LD A, " + hexstr((unsigned)(a.iv & 0xF), 1));
        else { emit("LD X, " + hexstr((unsigned)addr_of(a), 2)); emit("LD A, MX"); }
        bool need_b = !(b.k == K_NUM && (op == "==" || op == "!=") && !swap);
        if (need_b) {
            if (b.k == K_NUM) emit("LD B, " + hexstr((unsigned)(b.iv & 0xF), 1));
            else { emit("LD X, " + hexstr((unsigned)addr_of(b), 2)); emit("LD B, MX"); }
        }
        if (op == "==" || op == "!=") {
            if (need_b) emit("CP A, B");
            else emit("CP A, " + hexstr((unsigned)(r.iv & 0xF), 1));
            emit(op == "==" ? "JP NZ, " + skip : "JP Z, " + skip);
            return;
        }
        emit("SUB A, B");
        if (!swap) {
            if (op == "<") emit("JP NC, " + skip);
            else if (op == ">") { emit("JP C, " + skip); emit("JP Z, " + skip); }
            else emit("JP C, " + skip);              // >=
        } else {
            emit("JP C, " + skip);                    // <= (r-l: C<=>l>r)
        }
    }
void Gen::cmp_after_compare(const string& op) {
        if (op == "==") set_a_bool("Z");
        else if (op == "!=") set_a_bool("NZ");
        else if (op == "<") set_bool_from_flags({"C"});
        else if (op == ">") {
            string L1 = newlbl("C"), L2 = newlbl("C");
            emit("JP C, " + L1);
            emit("JP Z, " + L1);
            emit("LD A, 0x1");
            emit("JP " + L2);
            label(L1);
            emit("LD A, 0x0");
            label(L2);
        } else if (op == "<=") {
            set_bool_from_flags({"C", "Z"});
        } else if (op == ">=") {
            string L1 = newlbl("C"), L2 = newlbl("C");
            emit("JP C, " + L1);
            emit("LD A, 0x1");
            emit("JP " + L2);
            label(L1);
            emit("LD A, 0x0");
            label(L2);
        }
    }
void Gen::gen_cmp(const string& op, Node& l, Node& r) {
        // A = l cmp r ? 1 : 0 - optimiert
        auto addr_of = [&](Node& e) -> int {
            if (e.k == K_VAR) return var_base(e.s);
            if (e.k == K_ARR) return var_base(e.s) + (int)e.iv;
            throw CompileError("not a var or array");
        };
        if (l.k == K_NUM && r.k == K_NUM) {
            long long a = l.iv, b = r.iv;
            bool res;
            if (op == "==") res = a == b;
            else if (op == "!=") res = a != b;
            else if (op == "<") res = a < b;
            else if (op == ">") res = a > b;
            else if (op == "<=") res = a <= b;
            else res = a >= b;
            emit("LD A, " + hexstr(res ? 1u : 0u, 1));
            return;
        }
        if (r.k == K_NUM) {
            gen_expr(l);
            if (op == "==" || op == "!=") {
                emit("CP A, " + hexstr((unsigned)(r.iv & 0xF), 1));
                cmp_after_compare(op);
                return;
            }
            emit("LD B, " + hexstr((unsigned)(r.iv & 0xF), 1));
            emit("SUB A, B");
            cmp_after_compare(op);
            return;
        }
        if ((l.k == K_VAR || l.k == K_ARR) && (r.k == K_VAR || r.k == K_ARR)) {
            emit("LD X, " + hexstr((unsigned)addr_of(r), 2));
            emit("LD B, MX");
            emit("LD X, " + hexstr((unsigned)addr_of(l), 2));
            emit("LD A, MX");
            if (op == "==" || op == "!=") emit("CP A, B");
            else emit("SUB A, B");
            cmp_after_compare(op);
            return;
        }
        int t = push_temp();
        gen_expr(l);
        emit("LD X, " + hexstr((unsigned)t, 2));
        emit("LD MX, A");
        gen_expr(r);
        emit("LD B, A");
        emit("LD X, " + hexstr((unsigned)t, 2));
        emit("LD A, MX");
        pop_temp();
        if (op == "==" || op == "!=") emit("CP A, B");
        else emit("SUB A, B");
        cmp_after_compare(op);
    }
void Gen::set_a_bool(const string& cond) {
        string L1 = newlbl("C"), L2 = newlbl("C");
        emit("JP " + cond + ", " + L1);
        emit("LD A, 0x0");
        emit("JP " + L2);
        label(L1);
        emit("LD A, 0x1");
        label(L2);
    }
void Gen::set_bool_from_flags(const vector<string>& true_when) {
        string L1 = newlbl("C"), L2 = newlbl("C");
        for (auto& cond : true_when) emit("JP " + cond + ", " + L1);
        emit("LD A, 0x0");
        emit("JP " + L2);
        label(L1);
        emit("LD A, 0x1");
        label(L2);
    }
void Gen::alu_instr(const string& op) {
        const char* m = op == "+" ? "ADD" : op == "-" ? "SUB" :
                        op == "&" ? "AND" : op == "|" ? "OR" : "XOR";
        emit(string(m) + " A, B");
    }
int Gen::push_temp() {
        if (depth >= 4)
            throw CompileError(
                "expression too complex (max 4 temporaries)");
        int t = temp_addr + depth;
        depth++;
        return t;
    }
void Gen::pop_temp() { depth--; }
void Gen::load_a_var(const string& name, int idx) {
        int base = var_base(name);
        int addr = base + idx;
        if (addr > 0xFF) throw CompileError("variable address > 0xFF");
        emit("LD X, " + hexstr((unsigned)addr, 2));
        emit("LD A, MX");
    }
void Gen::store_a(Node& lhs) {
        int addr;
        if (lhs.k == K_VAR) addr = var_base(lhs.s);
        else if (lhs.k == K_ARR) addr = var_base(lhs.s) + (int)lhs.iv;
        else throw CompileError("left side of '=' must be a variable");
        emit("LD X, " + hexstr((unsigned)addr, 2));
        emit("LD MX, A");
    }
void Gen::load_pointer_x(const string& name) {
        int base = var_base(name);
        emit("LD X, " + hexstr((unsigned)base, 2));
        emit("LD A, MX");
        emit("LD B, A");                 // B = p[0]
        emit("INC X");
        emit("LD A, MX");                // A = p[1]
        emit("LD XL, A");
        emit("LD A, B");
        emit("LD XH, A");                // X = p[0]<<4 | p[1]
    }
void Gen::load_pointer_y(const string& name) {
        int base = var_base(name);
        emit("LD X, " + hexstr((unsigned)base, 2));
        emit("LD A, MX");
        emit("LD B, A");                 // B = p[0]
        emit("INC X");
        emit("LD A, MX");                // A = p[1]
        emit("LD YL, A");
        emit("LD A, B");
        emit("LD YH, A");                // Y = p[0]<<4 | p[1]
    }
void Gen::gen_settext(const string& name, const string& text) {
        int base = var_base(name);
        auto it = vars.find(name);
        int size = (it == vars.end()) ? 0 : it->second.second;
        if ((int)text.size() * 2 + 2 > size)
            throw CompileError("set_text: buffer '" + name + "' too small (" +
                               to_string(size) + " Nibbles, needs " +
                               to_string((int)text.size() * 2 + 2) + ")");
        emit("LD A, 0x0");
        emit("LD XP, A");
        emit("LD X, " + hexstr((unsigned)base, 2));
        for (unsigned char ch : text)
            emit("LBPX MX, " + hexstr(ch & 0xFF, 2));
        emit("LBPX MX, 0x00");
    }
void Gen::gen_assert(Node& e) {
        string L1 = newlbl("A"), L2 = newlbl("A");
        gen_expr(e);
        emit("CP A, 0x0");
        emit("JP NZ, " + L1);
        gen_dbgprint("assert fail");
        label(L2);
        emit("JP " + L2);
        label(L1);
    }
void Gen::gen_arrfn(const string& fn, const vector<string>& names) {
        // print_int / print_hex8/16 / print_int16 / inc16 / add16 / sub16 /
        // cmp16 mit Array-Argumenten (big-endian).
        // Uebergabe: X = Adresse des 1. Arrays, Y = 2. Array.  Runtimes
        // arbeiten in-place (add16/sub16/inc16) und nutzen Mn (RAM 0x00..09).
        string rt = "__" + fn;
        for (size_t i = 0; i < names.size(); i++) {
            int need;
            if (fn == "mul8")
                need = (i < 2) ? 2 : 4;      // a2/b2: 8-bit, dst4: 16-bit
            else
                need = (fn == "print_int" || fn == "print_hex8") ? 2 : 4;
            auto it = vars.find(names[i]);
            if (it == vars.end() || it->second.second < need)
                throw CompileError(fn + "(): '" + names[i] +
                                   "' requires an array with >= " +
                                   to_string(need) + " elements");
        }
        used_runtime.insert(rt);
        if (fn == "print_hex8" || fn == "print_hex16")
            used_runtime.insert("__hexnib");
        emit("LD X, " + hexstr((unsigned)var_base(names[0]), 2));
        if (names.size() > 1)
            emit("LD Y, " + hexstr((unsigned)var_base(names[1]), 2));
        emit("PSET 0x1");
        emit("CALL " + rt);
        if (fn == "mul8") {
            int base = var_base(names[2]);
            for (int i = 0; i < 4; i++) {
                emit("LD A, M" + to_string(i));
                emit("LD X, " + hexstr((unsigned)(base + i), 2));
                emit("LD MX, A");
            }
        }
    }

void Gen::gen_drawtext(bool hasx, int x, int y, const string& text) {
        int w = text_width(text);
        if (w > 32)
            throw CompileError(
                "draw_text: '" + text + "' ist " + to_string(w) +
                "px wide (>32) - max " + to_string(max_chars()) +
                " characters per line");
        int x0 = hasx ? x : (32 - w) / 2;
        if (x0 < 0 || x0 + w > 32)
            throw CompileError(
                "draw_text: '" + text + "' does not fit (x0=" +
                to_string(x0) + ", widee=" + to_string(w) + ")");
        map<int, int> bmap = lcd_bytes_for_text(text, x0, y);
        if (bmap.empty()) return;
        emit("LD A, 0xE");
        emit("LD XP, A");
        vector<int> addrs;
        for (auto& kv : bmap) addrs.push_back(kv.first);
        vector<vector<int>> runs;
        vector<int> cur = {addrs[0]};
        for (size_t i = 1; i < addrs.size(); i++) {
            if (addrs[i] == cur.back() + 2) cur.push_back(addrs[i]);
            else { runs.push_back(cur); cur = {addrs[i]}; }
        }
        runs.push_back(cur);
        for (auto& run : runs) {
            emit("LD X, " + hexstr((unsigned)(run[0] & 0xFF), 2));
            for (int a : run)
                emit("LBPX MX, " + hexstr((unsigned)bmap[a], 2));
        }
        emit("LD A, 0x0");
        emit("LD XP, A");
    }
void Gen::gen_dbgprint(const string& text) {
        emit("LD A, 0x0");
        emit("LD XP, A");
        emit("LD X, 0xC0");
        for (unsigned char ch : text)
            emit("LBPX MX, " + hexstr(ch & 0xFF, 2));
        emit("LBPX MX, 0x00");
        emit("LD A, 0xF");
        emit("LD XP, A");
        emit("LD X, 0x0E");
        emit("LD A, 0x1");
        emit("LD MX, A");
        emit("LD A, 0x0");
        emit("LD XP, A");
    }

void Gen::gen_drawsprite(const string& name) {
    string n = name;
    for (char& c : n) c = (char)toupper((unsigned char)c);
    auto it = SPRITES_P1.find(n);
    if (it == SPRITES_P1.end())
        throw CompileError("unknown sprite '" + n + "'");
    const auto& spr = it->second;
    emit("LD A, 0xE");
    emit("LD XP, A");
    // COM 0..7 (Bank 0)
    emit("LD X, 0x12");
    for (int c = 0; c < 8; c++)
        emit("LBPX MX, " + hexstr(spr.hi[c], 2));
    for (int c = 8; c < 16; c++) {
        int seg = COL_TO_SEG[8 + c];
        emit("LD X, " + hexstr((unsigned)(seg * 2), 2));
        emit("LBPX MX, " + hexstr(spr.hi[c], 2));
    }
    // COM 8..15 (Bank 1)
    emit("LD X, 0x92");
    for (int c = 0; c < 8; c++)
        emit("LBPX MX, " + hexstr(spr.lo[c], 2));
    for (int c = 8; c < 16; c++) {
        int seg = COL_TO_SEG[8 + c];
        emit("LD X, " + hexstr((unsigned)(0x80 + seg * 2), 2));
        emit("LBPX MX, " + hexstr(spr.lo[c], 2));
    }
    emit("LD A, 0x0");
    emit("LD XP, A");
}

void Gen::gen_seticon(Node& expr) {
    gen_expr(expr);
    used_runtime.insert("__set_icon");
    emit("LD X, 0x3C");
    emit("LD MX, A");
    emit("PSET 0x1");
    emit("CALL __set_icon");
}

void Gen::gen_clearicons() {
    emit("LD A, 0xE");
    emit("LD XP, A");
    emit("LD X, 0x10");
    emit("LD MX, 0x0");
    emit("LD X, 0xB9");
    emit("LD MX, 0x0");
    emit("LD A, 0x0");
    emit("LD XP, A");
}

void Gen::gen_clearvram() {
    used_runtime.insert("__clear_vram");
    emit("PSET 0x1");
    emit("CALL __clear_vram");
}
