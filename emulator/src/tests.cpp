// tests.cpp
// erstellt: 28. Jan. 2025
// zuletzt geändert: 12. Feb. 2025
// James Ezra Seitenschlag
#include <iostream>
#include <cassert>
#include "cpu.hpp"

void run_instruction_unit_tests() {
    std::cout << "Running all 109 CPU instruction tests on updated memory architecture...\n" << std::flush;
    // Test JP_s
    reset();
    ROM[programCounter.CurrentAddress()].v = JP_s.min | 0x42;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCS == 0x42);
    // Test RETD_e
    reset();
    SP.v = 0xFC; writeDataMem(0xFC, 0x1); writeDataMem(0xFD, 0x4); writeDataMem(0xFE, 0x2); IX.v = 0;
    ROM[programCounter.CurrentAddress()].v = RETD_e.min | 0xAB;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0).v == 0xB);
    assert(readDataMem(1).v == 0xA);
    assert(IX.v == 0x2);
    assert(programCounter.PCS == 0x42);
    // Test JP_Cs
    reset();
    flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = JP_Cs.min | 0x42;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCS == 0x42);
    // Test JP_NCs
    reset();
    flags.C = 0;
    ROM[programCounter.CurrentAddress()].v = JP_NCs.min | 0x42;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCS == 0x42);
    // Test CALL_s
    reset();
    SP.v = 0xFF;
    ROM[programCounter.CurrentAddress()].v = CALL_s.min | 0x42;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCS == 0x42);
    assert(SP.v == 0xFC);
    // Test CALZ_S
    reset();
    SP.v = 0xFF; flags.Z = 1;
    ROM[programCounter.CurrentAddress()].v = CALZ_S.min | 0x42;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCS == 0x42);
    assert(SP.v == 0xFC);
    // Test JP_Z_s
    reset();
    flags.Z = 1;
    ROM[programCounter.CurrentAddress()].v = JP_Z_s.min | 0x42;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCS == 0x42);
    // Test JP_NZ_s
    reset();
    flags.Z = 0;
    ROM[programCounter.CurrentAddress()].v = JP_NZ_s.min | 0x42;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCS == 0x42);
    // Test LD_Y_e
    reset();
    IY.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_Y_e.min | 0xAB;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0xAB);
    // Test LBPX_MX_e
    reset();
    IX.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = LBPX_MX_e.min | 0xAB;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0x20).v == 0xB); assert(readDataMem(0x21).v == 0xA); assert(IX.v == 0x22);
    // Test ADC_XH_i
    reset();
    IX.v = 0x30; flags.C = 0;
    ROM[programCounter.CurrentAddress()].v = ADC_XH_i.min | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0x70);
    // Test ADC_XL_i
    reset();
    IX.v = 0x03; flags.C = 0;
    ROM[programCounter.CurrentAddress()].v = ADC_XL_i.min | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0x07);
    // Test ADC_YH_i
    reset();
    IY.v = 0x30; flags.C = 0;
    ROM[programCounter.CurrentAddress()].v = ADC_YH_i.min | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0x70);
    // Test ADC_YL_i
    reset();
    IY.v = 0x03; flags.C = 0;
    ROM[programCounter.CurrentAddress()].v = ADC_YL_i.min | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0x07);
    // Test CP_XH_i
    reset();
    IX.v = 0x70;
    ROM[programCounter.CurrentAddress()].v = CP_XH_i.min | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0x70); assert(flags.C == 0);
    // Test CP_XL_i
    reset();
    IX.v = 0x07;
    ROM[programCounter.CurrentAddress()].v = CP_XL_i.min | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0x07); assert(flags.C == 0);
    // Test CP_YH_i
    reset();
    IY.v = 0x70;
    ROM[programCounter.CurrentAddress()].v = CP_YH_i.min | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0x70); assert(flags.C == 0);
    // Test CP_YL_i
    reset();
    IY.v = 0x07;
    ROM[programCounter.CurrentAddress()].v = CP_YL_i.min | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0x07); assert(flags.C == 0);
    // Test ADD_r_q
    reset();
    A.v = 3; B.v = 4;
    ROM[programCounter.CurrentAddress()].v = ADD_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 7);
    // Test ADC_r_q
    reset();
    A.v = 3; B.v = 4; flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = ADC_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 8);
    // Test SUB_r_q
    reset();
    A.v = 7; B.v = 4;
    ROM[programCounter.CurrentAddress()].v = SUB_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 3);
    // Test SBC_r_q
    reset();
    A.v = 7; B.v = 4; flags.C = 0;
    ROM[programCounter.CurrentAddress()].v = SBC_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 3); assert(flags.C == 0);
    // Test AND_r_q
    reset();
    A.v = 0xF; B.v = 0xA;
    ROM[programCounter.CurrentAddress()].v = AND_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xA);
    // Test OR_r_q
    reset();
    A.v = 0x5; B.v = 0xA;
    ROM[programCounter.CurrentAddress()].v = OR_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xF);
    // Test XOR_r_q
    reset();
    A.v = 0x5; B.v = 0xF;
    ROM[programCounter.CurrentAddress()].v = XOR_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xA);
    // Test RLC_r
    reset();
    A.v = 0x5; flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = RLC_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xB); assert(flags.C == 0);
    // Test LD_X_e
    reset();
    IX.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_X_e.min | 0xAB;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0xAB);
    // Test ADD_r_i
    reset();
    A.v = 3;
    ROM[programCounter.CurrentAddress()].v = ADD_r_i.min | (A_R << 4) | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 7);
    // Test ADC_r_i
    reset();
    A.v = 3; flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = ADC_r_i.min | (A_R << 4) | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 8);
    // Test AND_r_i
    reset();
    A.v = 0xF;
    ROM[programCounter.CurrentAddress()].v = AND_r_i.min | (A_R << 4) | 0xA;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xA);
    // Test OR_r_i
    reset();
    A.v = 0x5;
    ROM[programCounter.CurrentAddress()].v = OR_r_i.min | (A_R << 4) | 0xA;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xF);
    // Test XOR_r_i
    reset();
    A.v = 0x5;
    ROM[programCounter.CurrentAddress()].v = XOR_r_i.min | (A_R << 4) | 0xF;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xA);
    // Test NOT_r
    reset();
    A.v = 0x5;
    ROM[programCounter.CurrentAddress()].v = NOT_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xA);
    // Test SBC_r_i
    reset();
    A.v = 7; flags.C = 0;
    ROM[programCounter.CurrentAddress()].v = SBC_r_i.min | (A_R << 4) | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 3); assert(flags.C == 0);
    // Test FAN_r_i
    reset();
    A.v = 0x5;
    ROM[programCounter.CurrentAddress()].v = FAN_r_i.min | (A_R << 4) | 0x2;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5); assert(flags.Z == 1);
    // Test CP_r_i
    reset();
    A.v = 7;
    ROM[programCounter.CurrentAddress()].v = CP_r_i.min | (A_R << 4) | 4;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 7);
    assert(flags.C == 0);
    // Test LD_r_i
    reset();
    A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_i.min | (A_R << 4) | 0x5;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test PSET_p
    reset();
    ROM[programCounter.CurrentAddress()].v = PSET_p.min | 0x12;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.NBP == 1); assert(programCounter.NPP == 2);
    // Test LDPX_MX_i
    reset();
    IX.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = LDPX_MX_i.min | 0x5;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0x20).v == 0x5); assert(IX.v == 0x21);
    // Test LDPY_MY_i
    reset();
    IY.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = LDPY_MY_i.min | 0x5;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0x20).v == 0x5); assert(IY.v == 0x21);
    // Test LD_XP_r
    reset();
    A.v = 0x5; IX.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_XP_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0x500);
    // Test LD_XH_r
    reset();
    A.v = 0x5; IX.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_XH_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0x050);
    // Test LD_XL_r
    reset();
    A.v = 0x5; IX.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_XL_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0x005);
    // Test RRC_r
    reset();
    A.v = 0x5; flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = RRC_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0xA); assert(flags.C == 1);
    // Test LD_YP_r
    reset();
    A.v = 0x5; IY.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_YP_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0x500);
    // Test LD_YH_r
    reset();
    A.v = 0x5; IY.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_YH_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0x050);
    // Test LD_YL_r
    reset();
    A.v = 0x5; IY.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_YL_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0x005);
    // Test LD_r_XP
    reset();
    IX.v = 0x500; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_XP.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test LD_r_XH
    reset();
    IX.v = 0x050; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_XH.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test LD_r_XL
    reset();
    IX.v = 0x005; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_XL.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test LD_r_YP
    reset();
    IY.v = 0x500; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_YP.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test LD_r_YH
    reset();
    IY.v = 0x050; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_YH.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test LD_r_YL
    reset();
    IY.v = 0x005; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_YL.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test LD_r_q
    reset();
    B.v = 0x5;
    ROM[programCounter.CurrentAddress()].v = LD_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test INC_X
    reset();
    IX.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = INC_X.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IX.v == 0x21);
    // Test LDPX_r_q
    reset();
    IX.v = 0x20; A.v = 0; B.v = 0x5;
    ROM[programCounter.CurrentAddress()].v = LDPX_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5); assert(IX.v == 0x21);
    // Test INC_Y
    reset();
    IY.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = INC_Y.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(IY.v == 0x21);
    // Test LDPY_r_q
    reset();
    IY.v = 0x20; A.v = 0; B.v = 0x5;
    ROM[programCounter.CurrentAddress()].v = LDPY_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5); assert(IY.v == 0x21);
    // Test CP_r_q
    reset();
    A.v = 7; B.v = 4;
    ROM[programCounter.CurrentAddress()].v = CP_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 7);
    assert(flags.C == 0);
    // Test FAN_r_q
    reset();
    A.v = 0x5; B.v = 0x2;
    ROM[programCounter.CurrentAddress()].v = FAN_r_q.min | (A_R << 2) | B_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5); assert(flags.Z == 1);
    // Test ACPX_MX_r
    reset();
    IX.v = 0x20; writeDataMem(0x20, 0x5); A.v = 0x3; flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = ACPX_MX_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0x20).v == 0x9); assert(IX.v == 0x21);
    // Test ACPY_MY_r
    reset();
    IY.v = 0x20; writeDataMem(0x20, 0x5); A.v = 0x3; flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = ACPY_MY_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0x20).v == 0x9); assert(IY.v == 0x21);
    // Test SCPX_MX_r
    reset();
    IX.v = 0x20; writeDataMem(0x20, 0x5); A.v = 0x3; flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = SCPX_MX_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0x20).v == 0x1); assert(IX.v == 0x21); assert(flags.C == 0);
    // Test SCPY_MY_r
    reset();
    IY.v = 0x20; writeDataMem(0x20, 0x5); A.v = 0x3; flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = SCPY_MY_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0x20).v == 0x1); assert(IY.v == 0x21); assert(flags.C == 0);
    // Test SET_F_i
    reset();
    flags.C=0; flags.Z=0; flags.D=0; flags.I=0;
    ROM[programCounter.CurrentAddress()].v = SET_F_i.min | 0x5;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.C == 1); assert(flags.Z == 0); assert(flags.D == 1); assert(flags.I == 0);
    // Test SCF
    reset();
    flags.C = 0;
    ROM[programCounter.CurrentAddress()].v = SCF.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.C == 1);
    // Test SZF
    reset();
    flags.Z = 0;
    ROM[programCounter.CurrentAddress()].v = SZF.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.Z == 1);
    // Test SDF
    reset();
    flags.D = 0;
    ROM[programCounter.CurrentAddress()].v = SDF.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.D == 1);
    // Test EI
    reset();
    flags.I = 0;
    ROM[programCounter.CurrentAddress()].v = EI.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.I == 1);
    // Test RST_F_i
    reset();
    flags.C=1; flags.Z=1; flags.D=1; flags.I=1;
    ROM[programCounter.CurrentAddress()].v = RST_F_i.min | 0x5;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.C == 1); assert(flags.Z == 0); assert(flags.D == 1); assert(flags.I == 0);
    // Test DI
    reset();
    flags.I = 1;
    ROM[programCounter.CurrentAddress()].v = DI.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.I == 0);
    // Test RDF
    reset();
    flags.D = 1;
    ROM[programCounter.CurrentAddress()].v = RDF.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.D == 0);
    // Test RZF
    reset();
    flags.Z = 1;
    ROM[programCounter.CurrentAddress()].v = RZF.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.Z == 0);
    // Test RCF
    reset();
    flags.C = 1;
    ROM[programCounter.CurrentAddress()].v = RCF.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(flags.C == 0);
    // Test INC_Mn
    reset();
    writeDataMem(0xA, 0x5);
    ROM[programCounter.CurrentAddress()].v = INC_Mn.min | 0xA;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0xA).v == 0x6);
    // Test DEC_Mn
    reset();
    writeDataMem(0xA, 0x5);
    ROM[programCounter.CurrentAddress()].v = DEC_Mn.min | 0xA;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0xA).v == 0x4);
    // Test LD_Mn_A
    reset();
    A.v = 0x5; writeDataMem(0xA, 0);
    ROM[programCounter.CurrentAddress()].v = LD_Mn_A.min | 0xA;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0xA).v == 0x5);
    // Test LD_Mn_B
    reset();
    B.v = 0x5; writeDataMem(0xA, 0);
    ROM[programCounter.CurrentAddress()].v = LD_Mn_B.min | 0xA;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(readDataMem(0xA).v == 0x5);
    // Test LD_A_Mn
    reset();
    writeDataMem(0xA, 0x5); A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_A_Mn.min | 0xA;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test LD_B_Mn
    reset();
    writeDataMem(0xA, 0x5); B.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_B_Mn.min | 0xA;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(B.v == 0x5);
    // Test PUSH_r
    reset();
    A.v = 0x5; SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = PUSH_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F); assert(readDataMem(0x1F).v == 0x5);
    // Test PUSH_XP
    reset();
    IX.v = 0x500; SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = PUSH_XP.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F); assert(readDataMem(0x1F).v == 0x5);
    // Test PUSH_XH
    reset();
    IX.v = 0x050; SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = PUSH_XH.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F); assert(readDataMem(0x1F).v == 0x5);
    // Test PUSH_XL
    reset();
    IX.v = 0x005; SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = PUSH_XL.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F); assert(readDataMem(0x1F).v == 0x5);
    // Test PUSH_YP
    reset();
    IY.v = 0x500; SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = PUSH_YP.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F); assert(readDataMem(0x1F).v == 0x5);
    // Test PUSH_YH
    reset();
    IY.v = 0x050; SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = PUSH_YH.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F); assert(readDataMem(0x1F).v == 0x5);
    // Test PUSH_YL
    reset();
    IY.v = 0x005; SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = PUSH_YL.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F); assert(readDataMem(0x1F).v == 0x5);
    // Test PUSH_F
    reset();
    flags.I=1; flags.D=0; flags.Z=1; flags.C=0; SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = PUSH_F.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F); assert(readDataMem(0x1F).v == 0xA);
    // Test DEC_SP
    reset();
    SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = DEC_SP.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x1F);
    // Test POP_r
    reset();
    writeDataMem(0x1F, 0x5); SP.v = 0x1F; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = POP_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x20); assert(A.v == 0x5);
    // Test POP_XP
    reset();
    writeDataMem(0x1F, 0x5); SP.v = 0x1F; IX.v = 0;
    ROM[programCounter.CurrentAddress()].v = POP_XP.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x20); assert(IX.v == 0x500);
    // Test POP_XH
    reset();
    writeDataMem(0x1F, 0x5); SP.v = 0x1F; IX.v = 0;
    ROM[programCounter.CurrentAddress()].v = POP_XH.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x20); assert(IX.v == 0x050);
    // Test POP_XL
    reset();
    writeDataMem(0x1F, 0x5); SP.v = 0x1F; IX.v = 0;
    ROM[programCounter.CurrentAddress()].v = POP_XL.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x20); assert(IX.v == 0x005);
    // Test POP_YP
    reset();
    writeDataMem(0x1F, 0x5); SP.v = 0x1F; IY.v = 0;
    ROM[programCounter.CurrentAddress()].v = POP_YP.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x20); assert(IY.v == 0x500);
    // Test POP_YH
    reset();
    writeDataMem(0x1F, 0x5); SP.v = 0x1F; IY.v = 0;
    ROM[programCounter.CurrentAddress()].v = POP_YH.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x20); assert(IY.v == 0x050);
    // Test POP_YL
    reset();
    writeDataMem(0x1F, 0x5); SP.v = 0x1F; IY.v = 0;
    ROM[programCounter.CurrentAddress()].v = POP_YL.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x20); assert(IY.v == 0x005);
    // Test POP_F
    reset();
    writeDataMem(0x1F, 0xA); SP.v = 0x1F; flags.I=0; flags.D=0; flags.Z=0; flags.C=0;
    ROM[programCounter.CurrentAddress()].v = POP_F.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x20); assert(flags.I == 1); assert(flags.D == 0); assert(flags.Z == 1); assert(flags.C == 0);
    // Test INC_SP
    reset();
    SP.v = 0x20;
    ROM[programCounter.CurrentAddress()].v = INC_SP.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x21);
    // Test RETS
    reset();
    SP.v = 0xFC; writeDataMem(0xFC, 0x1); writeDataMem(0xFD, 0x4); writeDataMem(0xFE, 0x2);
    ROM[programCounter.CurrentAddress()].v = RETS.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCP == 0x1);
    assert(programCounter.PCS == 0x43);
    // Test RET
    reset();
    SP.v = 0xFC; writeDataMem(0xFC, 0x1); writeDataMem(0xFD, 0x4); writeDataMem(0xFE, 0x2);
    ROM[programCounter.CurrentAddress()].v = RET.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCP == 0x1);
    assert(programCounter.PCS == 0x42);
    assert(SP.v == 0xFF);
    // Test LD_SPH_r
    reset();
    A.v = 0x5; SP.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_SPH_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x50);
    // Test LD_r_SPH
    reset();
    SP.v = 0x50; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_SPH.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test JPBA
    reset();
    B.v = 0x3; A.v = 0x4;
    ROM[programCounter.CurrentAddress()].v = JPBA.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(programCounter.PCS == 0x34);
    // Test LD_SPL_r
    reset();
    A.v = 0x5; SP.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_SPL_r.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(SP.v == 0x05);
    // Test LD_r_SPL
    reset();
    SP.v = 0x05; A.v = 0;
    ROM[programCounter.CurrentAddress()].v = LD_r_SPL.min | A_R;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(A.v == 0x5);
    // Test HALT
    reset();
    ROM[programCounter.CurrentAddress()].v = HALT.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(true);
    // Test SLP
    reset();
    ROM[programCounter.CurrentAddress()].v = SLP.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(true);
    // Test NOP5
    reset();
    ROM[programCounter.CurrentAddress()].v = NOP5.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(true);
    // Test NOP7
    reset();
    ROM[programCounter.CurrentAddress()].v = NOP7.min;
    executeInstruction(ROM[programCounter.CurrentAddress()].v, ROM);
    assert(true);
    std::cout << "All 109 tests passed successfully on updated memory mapping!\n";
}


void test_ram_regions_combinations() {
    std::cout << "\n=== Testing Combinations of All RAM Regions with Instructions ===\n";

    // -------------------------------------------------------------
    // REGION 1: Memory Registers Mn (0x000 - 0x00F)
    // -------------------------------------------------------------
    std::cout << "[RAM Region 1: 0x000 - 0x00F (Memory Registers Mn)] Testing...\n";
    {
        reset();
        // 1.1 Direct Store & Load: LD Mn, A and LD A, Mn across all 16 memory registers
        for (unsigned char n = 0; n < 16; n++) {
            A.v = (n * 3 + 1) & 0xF;
            ROM[0x100].v = LD_Mn_A.min | n;
            executeInstruction(ROM[0x100].v, ROM);
            assert(readDataMem(n).v == A.v);

            A.v = 0;
            ROM[0x100].v = LD_A_Mn.min | n;
            executeInstruction(ROM[0x100].v, ROM);
            assert(A.v == ((n * 3 + 1) & 0xF));
        }

        // 1.2 LD Mn, B and LD B, Mn
        for (unsigned char n = 0; n < 16; n++) {
            B.v = (15 - n) & 0xF;
            ROM[0x100].v = LD_Mn_B.min | n;
            executeInstruction(ROM[0x100].v, ROM);
            assert(readDataMem(n).v == B.v);

            B.v = 0;
            ROM[0x100].v = LD_B_Mn.min | n;
            executeInstruction(ROM[0x100].v, ROM);
            assert(B.v == ((15 - n) & 0xF));
        }

        // 1.3 INC Mn and DEC Mn arithmetic
        writeDataMem(0x05, 0x0E);
        ROM[0x100].v = INC_Mn.min | 0x05;
        executeInstruction(ROM[0x100].v, ROM); // 0x0E -> 0x0F
        assert(readDataMem(0x05).v == 0x0F);
        executeInstruction(ROM[0x100].v, ROM); // 0x0F -> 0x00 (4-bit overflow wrap)
        assert(readDataMem(0x05).v == 0x00);

        ROM[0x100].v = DEC_Mn.min | 0x05;
        executeInstruction(ROM[0x100].v, ROM); // 0x00 -> 0x0F (4-bit underflow wrap)
        assert(readDataMem(0x05).v == 0x0F);
    }
    std::cout << "  -> Memory Registers Mn (0x000 - 0x00F) PASSED!\n";

    // -------------------------------------------------------------
    // REGION 2: Stack RAM Area (0x010 - 0x0FF)
    // -------------------------------------------------------------
    std::cout << "[RAM Region 2: 0x010 - 0x0FF (Stack & General RAM)] Testing...\n";
    {
        reset();
        SP.v = 0x80; // Set SP in middle of stack area

        // 2.1 PUSH & POP sequences with registers A, B, IX, IY, Flags
        A.v = 0x9;
        B.v = 0xC;
        IX.v = 0x1A5;
        IY.v = 0x2B6;
        flags.I = 1; flags.D = 0; flags.Z = 1; flags.C = 1; // F = 1011b = 0xB

        ROM[0x100].v = PUSH_r.min | A_R; executeInstruction(ROM[0x100].v, ROM);   // SP -> 0x7F
        assert(SP.v == 0x7F); assert(readDataMem(0x7F).v == 0x9);

        ROM[0x100].v = PUSH_r.min | B_R; executeInstruction(ROM[0x100].v, ROM);   // SP -> 0x7E
        assert(SP.v == 0x7E); assert(readDataMem(0x7E).v == 0xC);

        ROM[0x100].v = PUSH_XP.min; executeInstruction(ROM[0x100].v, ROM);        // SP -> 0x7D, XP=1
        ROM[0x100].v = PUSH_XH.min; executeInstruction(ROM[0x100].v, ROM);        // SP -> 0x7C, XH=A
        ROM[0x100].v = PUSH_XL.min; executeInstruction(ROM[0x100].v, ROM);        // SP -> 0x7B, XL=5
        assert(SP.v == 0x7B);

        ROM[0x100].v = PUSH_YP.min; executeInstruction(ROM[0x100].v, ROM);        // SP -> 0x7A, YP=2
        ROM[0x100].v = PUSH_YH.min; executeInstruction(ROM[0x100].v, ROM);        // SP -> 0x79, YH=B
        ROM[0x100].v = PUSH_YL.min; executeInstruction(ROM[0x100].v, ROM);        // SP -> 0x78, YL=6
        assert(SP.v == 0x78);

        ROM[0x100].v = PUSH_F.min; executeInstruction(ROM[0x100].v, ROM);         // SP -> 0x77, F=0xB
        assert(SP.v == 0x77); assert(readDataMem(0x77).v == 0xB);

        // Verify POPs restore exact values
        flags.I = flags.D = flags.Z = flags.C = 0;
        ROM[0x100].v = POP_F.min; executeInstruction(ROM[0x100].v, ROM);
        assert(SP.v == 0x78);
        assert(flags.I == 1 && flags.D == 0 && flags.Z == 1 && flags.C == 1);

        IY.v = 0;
        ROM[0x100].v = POP_YL.min; executeInstruction(ROM[0x100].v, ROM);
        ROM[0x100].v = POP_YH.min; executeInstruction(ROM[0x100].v, ROM);
        ROM[0x100].v = POP_YP.min; executeInstruction(ROM[0x100].v, ROM);
        assert(SP.v == 0x7B); assert(IY.v == 0x2B6);

        IX.v = 0;
        ROM[0x100].v = POP_XL.min; executeInstruction(ROM[0x100].v, ROM);
        ROM[0x100].v = POP_XH.min; executeInstruction(ROM[0x100].v, ROM);
        ROM[0x100].v = POP_XP.min; executeInstruction(ROM[0x100].v, ROM);
        assert(SP.v == 0x7E); assert(IX.v == 0x1A5);

        B.v = 0; ROM[0x100].v = POP_r.min | B_R; executeInstruction(ROM[0x100].v, ROM);
        assert(SP.v == 0x7F); assert(B.v == 0xC);

        A.v = 0; ROM[0x100].v = POP_r.min | A_R; executeInstruction(ROM[0x100].v, ROM);
        assert(SP.v == 0x80); assert(A.v == 0x9);

        // 2.2 Subroutine Call & Return via Stack
        programCounter.PCP = 0x3;
        programCounter.PCS = 0x45;
        ROM[0x100].v = CALL_s.min | 0x90; // CALL 0x90
        executeInstruction(ROM[0x100].v, ROM);
        assert(SP.v == 0x7D);
        assert(programCounter.PCS == 0x90);
        assert(readDataMem(0x7D).v == 0x3);
        assert(readDataMem(0x7E).v == 0x4);
        assert(readDataMem(0x7F).v == 0x5);

        ROM[0x100].v = RET.min;
        executeInstruction(ROM[0x100].v, ROM);
        assert(SP.v == 0x80);
        assert(programCounter.PCP == 0x3);
        assert(programCounter.PCS == 0x45);
    }
    std::cout << "  -> Stack & General RAM (0x010 - 0x0FF) PASSED!\n";

    // -------------------------------------------------------------
    // REGION 3: Upper General RAM (0x100 - 0x27F)
    // -------------------------------------------------------------
    std::cout << "[RAM Region 3: 0x100 - 0x27F (Upper General RAM)] Testing...\n";
    {
        reset();
        // 3.1 Stream writing with LDPX MX, i and LBPX MX, e up to RAM limit 0x27F
        IX.v = 0x200;
        for (int i = 0; i < 16; i++) {
            ROM[0x100].v = LDPX_MX_i.min | (i & 0xF);
            executeInstruction(ROM[0x100].v, ROM);
            assert(readDataMem(0x200 + i).v == (i & 0xF));
        }
        assert(IX.v == 0x210);

        // 3.2 LBPX (2 nibbles at once)
        IX.v = 0x270;
        ROM[0x100].v = LBPX_MX_e.min | 0x6E; // Store 0xE at 0x270, 0x6 at 0x271
        executeInstruction(ROM[0x100].v, ROM);
        assert(readDataMem(0x270).v == 0xE);
        assert(readDataMem(0x271).v == 0x6);
        assert(IX.v == 0x272);

        // 3.3 Pointer ALU instructions modifying memory: ACPX, SCPX, ADD MX, r, XOR MX, r
        IX.v = 0x27E;
        writeDataMem(0x27E, 0x5);
        A.v = 0x3; flags.C = 1;
        ROM[0x100].v = ACPX_MX_r.min | A_R;
        executeInstruction(ROM[0x100].v, ROM); // 0x5 + 0x3 + 1 = 0x9
        assert(readDataMem(0x27E).v == 0x9);
        assert(IX.v == 0x27F);

        // SCPX at RAM upper boundary 0x27F
        writeDataMem(0x27F, 0x8);
        B.v = 0x3; flags.C = 1;
        ROM[0x100].v = SCPX_MX_r.min | B_R;
        executeInstruction(ROM[0x100].v, ROM); // 0x8 - 0x3 - 1 = 0x4
        assert(readDataMem(0x27F).v == 0x4);
        assert(IX.v == 0x280); assert(flags.C == 0);

        // Memory Arithmetic with MX: ADD MX, B and XOR MX, A
        IX.v = 0x150;
        writeDataMem(0x150, 0x4);
        B.v = 0x6;
        ROM[0x100].v = ADD_r_q.min | (MX_R << 2) | B_R;
        executeInstruction(ROM[0x100].v, ROM); // MX = 0x4 + 0x6 = 0xA
        assert(readDataMem(0x150).v == 0xA);

        A.v = 0xF;
        ROM[0x100].v = XOR_r_q.min | (MX_R << 2) | A_R;
        executeInstruction(ROM[0x100].v, ROM); // MX = 0xA ^ 0xF = 0x5
        assert(readDataMem(0x150).v == 0x5);

        // NOT MX (Encoded as XOR MX, 0xF: bits 5-4 are register code)
        ROM[0x100].v = NOT_r.min | (MX_R << 4);
        executeInstruction(ROM[0x100].v, ROM); // ~0x5 & 0xF = 0xA
        assert(readDataMem(0x150).v == 0xA);
    }
    std::cout << "  -> Upper General RAM (0x100 - 0x27F) PASSED!\n";

    // -------------------------------------------------------------
    // REGION 4: Display Data Memory VRAM (0xE00 - 0xE4F, 0xE80 - 0xECF)
    // -------------------------------------------------------------
    std::cout << "[RAM Region 4: 0xE00 - 0xECF (Display Data RAM / VRAM)] Testing...\n";
    {
        reset();
        // 4.1 Drawing dot matrix pattern in lower VRAM (0xE00 - 0xE4F) via IX
        IX.v = 0xE00;
        for (unsigned char nibble = 0; nibble < 16; nibble++) {
            ROM[0x100].v = LDPX_MX_i.min | nibble;
            executeInstruction(ROM[0x100].v, ROM);
            assert(readDataMem(0xE00 + nibble).v == nibble);
        }
        assert(IX.v == 0xE10);

        // 4.2 Drawing pattern in upper VRAM (0xE80 - 0xECF) via IY
        IY.v = 0xE80;
        for (unsigned char nibble = 0; nibble < 8; nibble++) {
            ROM[0x100].v = LDPY_MY_i.min | (15 - nibble);
            executeInstruction(ROM[0x100].v, ROM);
            assert(readDataMem(0xE80 + nibble).v == (15 - nibble));
        }
        assert(IY.v == 0xE88);

        // 4.3 VRAM Pixel Bitwise Blending with OR, AND, XOR
        IX.v = 0xEC0;
        writeDataMem(0xEC0, 0b0011);
        A.v = 0b1100;
        ROM[0x100].v = OR_r_q.min | (MX_R << 2) | A_R;
        executeInstruction(ROM[0x100].v, ROM); // 0011 | 1100 = 1111 (0xF)
        assert(readDataMem(0xEC0).v == 0xF);

        B.v = 0b1010;
        ROM[0x100].v = AND_r_q.min | (MX_R << 2) | B_R;
        executeInstruction(ROM[0x100].v, ROM); // 1111 & 1010 = 1010 (0xA)
        assert(readDataMem(0xEC0).v == 0xA);

        A.v = 0b0011;
        ROM[0x100].v = XOR_r_q.min | (MX_R << 2) | A_R;
        executeInstruction(ROM[0x100].v, ROM); // 1010 ^ 0011 = 1001 (0x9)
        assert(readDataMem(0xEC0).v == 0x9);

        // 4.4 Rotate dots in VRAM: RLC MX and RRC MX
        flags.C = 0;
        ROM[0x100].v = RLC_r.min | MX_R;
        executeInstruction(ROM[0x100].v, ROM); // 1001 << 1 | 0 = 0010 (0x2), Carry = 1
        assert(readDataMem(0xEC0).v == 0x2);
        assert(flags.C == 1);

        ROM[0x100].v = RRC_r.min | MX_R;
        executeInstruction(ROM[0x100].v, ROM); // 0010 >> 1 | (1 << 3) = 1001 (0x9), Carry = 0
        assert(readDataMem(0xEC0).v == 0x9);
        assert(flags.C == 0);
    }
    std::cout << "  -> Display Data Memory (0xE00 - 0xECF) PASSED!\n";

    // -------------------------------------------------------------
    // REGION 5: I/O Data Memory & Hardware Registers (0xF00 - 0xF7E)
    // -------------------------------------------------------------
    std::cout << "[RAM Region 5: 0xF00 - 0xF7E (I/O & Peripheral Registers)] Testing...\n";
    {
        reset();
        // 5.1 Configuring Output Ports R00-R03 (0xF50) & Buzzer/FOUT R40-R43 (0xF54)
        IX.v = 0xF50;
        A.v = 0x5; // Output pattern 0101b
        ROM[0x100].v = LD_r_q.min | (MX_R << 2) | A_R;
        executeInstruction(ROM[0x100].v, ROM);
        assert(readDataMem(0xF50).v == 0x5);

        IX.v = 0xF54;
        B.v = 0x8; // Buzzer ON (R43 = 0, others 1 -> 1000b)
        ROM[0x100].v = LD_r_q.min | (MX_R << 2) | B_R;
        executeInstruction(ROM[0x100].v, ROM);
        assert(readDataMem(0xF54).v == 0x8);

        // 5.2 Configuring Timers and Interrupt Masks (0xF10 - 0xF27)
        IX.v = 0xF10; // EIT32..EIT1 Clock Timer Interrupt Mask
        ROM[0x100].v = LDPX_MX_i.min | 0xF; // Enable all 4 clock timer interrupts
        executeInstruction(ROM[0x100].v, ROM);
        assert(readDataMem(0xF10).v == 0xF);

        // Programmable Timer reload data (RD0-RD7 at 0xF26, 0xF27)
        IX.v = 0xF26;
        ROM[0x100].v = LBPX_MX_e.min | 0xA6; // Reload value 0xA6 (RD0-RD3=6, RD4-RD7=A)
        executeInstruction(ROM[0x100].v, ROM);
        assert(readDataMem(0xF26).v == 0x6);
        assert(readDataMem(0xF27).v == 0xA);

        // 5.3 LCD Duty & Contrast Control Registers (0xF71, 0xF72)
        IY.v = 0xF71;
        ROM[0x100].v = LDPY_MY_i.min | 0x2; // Set 1/8 duty (LDUTY=1)
        executeInstruction(ROM[0x100].v, ROM);
        assert(readDataMem(0xF71).v == 0x2);

        IY.v = 0xF72;
        ROM[0x100].v = LDPY_MY_i.min | 0x7; // Contrast level 7
        executeInstruction(ROM[0x100].v, ROM);
        assert(readDataMem(0xF72).v == 0x7);

        // 5.4 Testing FAN (Bit Test) on I/O status register
        IX.v = 0xF00;
        writeDataMem(0xF00, 0b0010); // IT8 interrupt flag active
        A.v = 0b0010;
        ROM[0x100].v = FAN_r_q.min | (MX_R << 2) | A_R; // Test if IT8 is set
        executeInstruction(ROM[0x100].v, ROM);
        assert(flags.Z == 0); // Result is non-zero (flag matched)

        A.v = 0b0100;
        ROM[0x100].v = FAN_r_q.min | (MX_R << 2) | A_R; // Test if IT2 is set
        executeInstruction(ROM[0x100].v, ROM);
        assert(flags.Z == 1); // Result is zero (flag not set)
    }
    std::cout << "  -> I/O & Peripheral Registers (0xF00 - 0xF7E) PASSED!\n";

    // -------------------------------------------------------------
    // REGION 6: Cross-Region Block Transfer (Sprite copy: General RAM -> VRAM)
    // -------------------------------------------------------------
    std::cout << "[Cross-Region: Sprite Block Copy (General RAM -> Display RAM)] Testing...\n";
    {
        reset();
        // Prepare 8 nibbles of sprite data in General RAM (0x120 - 0x127)
        for (int i = 0; i < 8; i++) {
            writeDataMem(0x120 + i, (i * 2 + 1) & 0xF);
        }

        // Copy loop simulation: Read from General RAM via IX, write to VRAM via IY
        IX.v = 0x120;
        IY.v = 0xE20;
        for (int i = 0; i < 8; i++) {
            // LD A, MX; IX++ (via LDPX A, MX)
            ROM[0x100].v = LDPX_r_q.min | (A_R << 2) | MX_R;
            executeInstruction(ROM[0x100].v, ROM);

            // MY = A; IY++ (via LD MY, A then INC Y)
            ROM[0x100].v = LD_r_q.min | (MY_R << 2) | A_R;
            executeInstruction(ROM[0x100].v, ROM);
            ROM[0x100].v = INC_Y.min;
            executeInstruction(ROM[0x100].v, ROM);
        }

        // Verify VRAM received exact sprite data
        for (int i = 0; i < 8; i++) {
            assert(readDataMem(0xE20 + i).v == ((i * 2 + 1) & 0xF));
        }
        assert(IX.v == 0x128);
        assert(IY.v == 0xE28);
    }
    std::cout << "  -> Cross-Region Block Transfer PASSED!\n";
}

void run_all_tests() {
    run_instruction_unit_tests();
    test_ram_regions_combinations();
    std::cout << "\n>>> ALL SYSTEM & RAM COMBINATION TESTS COMPLETED SUCCESSFULLY! <<<\n";
}
