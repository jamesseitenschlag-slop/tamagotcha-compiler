// tama_types.h
// erstellt: 10. Feb. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
/* =============================================================================
 * tama_types.h - SEMANTIC HARDWARE REGISTERS & DIRECT RAM VARIABLES
 * ============================================================================= */
#ifndef TAMA_TYPES_H
#define TAMA_TYPES_H

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;

/* --- 4-bit / 12-bit Hardware CPU Registers --- */
extern uint8_t  REG_A;              /* Accumulator A (4-bit ALU) */
extern uint8_t  REG_B;              /* General Register B (4-bit) */
extern uint8_t  REG_F;              /* Flags Register F */
extern uint16_t REG_INDEX_X;        /* Data Pointer X (12-bit: XP:XH:XL) */
extern uint16_t REG_INDEX_Y;        /* Data Pointer Y (12-bit: YP:YH:YL) */
extern uint8_t  REG_RAM_AT_X;       /* Memory cell [X] (MX) */
extern uint8_t  REG_RAM_AT_Y;       /* Memory cell [Y] (MY) */
extern uint8_t  REG_BANK_XP;        /* RAM Bank Pointer for X */
extern uint8_t  REG_PAGE_XH;        /* RAM Page Pointer for X */
extern uint8_t  REG_OFFSET_XL;      /* RAM Offset for X */
extern uint8_t  REG_BANK_YP;        /* RAM Bank Pointer for Y */
extern uint8_t  REG_PAGE_YH;        /* RAM Page Pointer for Y */
extern uint8_t  REG_OFFSET_YL;      /* RAM Offset for Y */
extern uint8_t  REG_STACK_HIGH;     /* Stack Pointer High Nibble (SPH) */
extern uint8_t  REG_STACK_LOW;      /* Stack Pointer Low Nibble (SPL) */

/* --- CPU Status Flags --- */
extern uint8_t  FLAG_CARRY;         /* Carry / Borrow Flag */
extern uint8_t  FLAG_ZERO;          /* Zero Flag */
extern uint8_t  FLAG_DECIMAL;       /* BCD Decimal Mode Flag */
extern uint8_t  FLAG_INTERRUPT;     /* Global Interrupt Flag */

/* --- Semantic Direct RAM Cell Variables (M0..M15) --- */
extern uint8_t  RAM_UI_MAIN_STATE;      /* M0: Active UI Main State (0..7) */
extern uint8_t  RAM_UI_SUB_STATE;       /* M1: Sub-state / Step counter */
extern uint8_t  RAM_ACTIVE_ICON_INDEX;  /* M2: Currently highlighted Icon */
extern uint8_t  RAM_MENU_SELECTION;     /* M3: Menu selection cursor */
extern uint8_t  RAM_KEY_PRESS_EVENT;    /* M4: Debounced Key Press Event */
extern uint8_t  RAM_KEY_CURRENT_STATE;  /* M5: Current Button State */
extern uint8_t  RAM_TIMER_16HZ_TICK;    /* M6: 16-Hz Timer Counter */
extern uint8_t  RAM_CLOCK_SEC_UNITS;    /* M7: RTC Seconds (Units) */
extern uint8_t  RAM_CLOCK_SEC_TENS;     /* M8: RTC Seconds (Tens) */
extern uint8_t  RAM_CLOCK_MIN_UNITS;    /* M9: RTC Minutes (Units) */
extern uint8_t  RAM_CLOCK_MIN_TENS;     /* M10: RTC Minutes (Tens) */
extern uint8_t  RAM_HUNGER_HEARTS;      /* M11: Hunger Level (0..4 hearts) */
extern uint8_t  RAM_HAPPY_HEARTS;       /* M12: Happiness Level (0..4 hearts) */
extern uint8_t  RAM_DISCIPLINE_BARS;    /* M13: Discipline Level (0..4 bars) */
extern uint8_t  RAM_PET_AGE_YEARS;      /* M14: Pet Age in Years */
extern uint8_t  RAM_POOP_COUNT;         /* M15: Active Poop Count (0..4) */

/* --- Function Prototypes & Intrinsics --- */
void org(int addr);
void word(int w);
void jp(int target);
void call(int target);
void calz(int target);
void pset(int page);
void retd(uint8_t v);
void rets(void);
void push(uint8_t r);
void pop(uint8_t r);
void ei(void);
void di(void);
void scf(void);
void rcf(void);
void szf(void);
void rzf(void);
void sdf(void);
void rdf(void);
void halt(void);
void slp(void);
void nop7(void);
void nop5(void);
void jpba(void);
uint8_t adc(uint8_t a, uint8_t b);
uint8_t sbc(uint8_t a, uint8_t b);
void cp(uint8_t a, uint8_t b);
void fan(uint8_t a, uint8_t b);
void rlc(uint8_t r);
void rrc(uint8_t r);
void acpx(uint8_t dst, uint8_t src);
void acpy(uint8_t dst, uint8_t src);
void scpx(uint8_t dst, uint8_t src);
void scpy(uint8_t dst, uint8_t src);
void rst(uint8_t f, uint8_t v);
void lbpx(uint8_t v);
void ldpx(uint8_t dst, uint8_t src);
void ldpy(uint8_t dst, uint8_t src);

#endif /* TAMA_TYPES_H */
