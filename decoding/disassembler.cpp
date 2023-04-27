#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits>

using uint32 = uint32_t;
using uint16 = uint16_t;
using int32 = int32_t;
using int16 = int16_t;
using uint8 = uint8_t;
using int8 = int8_t;

const char* GRegTableL[] = {
    "AL",
    "CL",
    "DL",
    "BL",
    "AH",
    "CH",
    "DH",
    "BH"
};

const char* GRegTableX[] = {
    "AX",
    "CX",
    "DX",
    "BX",
    "SP",
    "BP",
    "SI",
    "DI"
};

const char* GRegMemTable[] = {
    "BX + SI", // 3 + 6
    "BX + DI", // 3 + 7
    "BP + SI", // 5 + 6
    "BP + DI", // 5 + 7
    "SI",      // 6
    "DI",      // 7
    "BP",      // 5
    "BX"       // 3
};

uint8 GMemRegTable1[] = { 3, 3, 5, 5, 6, 7, 5, 3 };
uint8 GMemRegTable2[] = { 6, 7, 6, 7, UINT8_MAX, UINT8_MAX, UINT8_MAX, UINT8_MAX };

enum class IName : uint16
{
    MOV,
    ADD,
    SUB,
    CMP,
    JE,
    JL,
    JLE,
    JB,
    JBE,
    JP,
    JO,
    JS,
    JNE,
    JNL,
    JNLE,
    JNB,
    JNBE,
    JNP,
    JNO,
    JNS,
    LOOP,
    LOOPZ,
    LOOPNZ,
    JCXZ,
    UNKNOWN
};

const char* InstrNames[]
{
    "MOV",
    "ADD",
    "SUB",
    "CMP",
    "JE",
    "JL",
    "JLE",
    "JB",
    "JBE",
    "JP",
    "JO",
    "JS",
    "JNE",
    "JNL",
    "JNLE",
    "JNB",
    "JNBE",
    "JNP",
    "JNO",
    "JNS",
    "LOOP",
    "LOOPZ",
    "LOOPNZ",
    "JCXZ"
};

struct Instruction
{
    IName Name = IName::UNKNOWN;
    uint8 RegSrc = UINT8_MAX;
    uint8 MemRegSrc = UINT8_MAX;
    uint16 DisplSrc = UINT16_MAX;
    uint8 RegDst = UINT8_MAX;
    uint8 MemRegDst = UINT8_MAX;
    uint16 DisplDst = UINT16_MAX;

    union
    {
        uint16 Immediate = UINT16_MAX;
        uint16 Displacement;
    };

    uint8 ByteSize = 0;

    bool Wide = true;
};

struct RegisterFile
{
    uint16 GPRs[ sizeof(GRegTableX) / sizeof(GRegTableX[0]) ];
    uint16 IP;
    bool ZF;
    bool SF;
};

struct Storage
{
    RegisterFile RegFile;
    uint8 Memory[ 1 << 16 ];
};

const char* GetRegisterName( uint8 RegID, bool IsWide )
{
    return IsWide ? GRegTableX[ RegID ] : GRegTableL[ RegID ];
}

uint16 CalculateMemoryAddress( const Instruction& MovInstr, const RegisterFile& RegFile )
{
    uint16 Address = UINT16_MAX;
    if ( MovInstr.MemRegDst != UINT8_MAX || MovInstr.MemRegSrc != UINT8_MAX )
    {
        uint8 MemReg = MovInstr.MemRegDst != UINT8_MAX ? MovInstr.MemRegDst : MovInstr.MemRegSrc;
        uint8 Reg = GMemRegTable1[ MemReg ];
        Address = RegFile.GPRs[ Reg ];

        Reg = GMemRegTable2[ MemReg ];
        if ( Reg != UINT8_MAX )
        {
            Address += RegFile.GPRs[ Reg ];
        }
    }

    if ( MovInstr.DisplDst != UINT16_MAX || MovInstr.DisplSrc != UINT16_MAX )
    {
        uint16 Displ = MovInstr.DisplDst != UINT16_MAX ? MovInstr.DisplDst : MovInstr.DisplSrc;
        if ( Address != UINT16_MAX )
        {
            Address += Displ;
        }
        else
        {
            Address = Displ;
        }
    }

    return Address;
}

void DecodeRegToRegMem( const uint8* InstrPtr, Instruction& Instr )
{
    uint8 d = InstrPtr[0] & 0b00000010;
    uint8 w = InstrPtr[0] & 0b00000001;
    uint8 mod = ( InstrPtr[1] & 0b11000000 ) >> 6;
    uint8 reg = ( InstrPtr[1] & 0b00111000 ) >> 3;
    uint8 r_m = ( InstrPtr[1] & 0b00000111 );

    Instr.Wide = w;

    switch ( mod )
    {
    case 0b11:
        {
            Instr.RegDst = d ? reg : r_m;
            Instr.RegSrc = d ? r_m : reg;

            Instr.ByteSize = 2;
            return;
        }

    case 0b00:
        {
            if ( r_m == 0b110 )
            {
                if ( d )
                {
                    Instr.RegDst = reg;
                    Instr.DisplSrc = *(uint16*)&InstrPtr[2];
                }
                else
                {
                    Instr.RegSrc = reg;
                    Instr.DisplDst = *(uint16*)&InstrPtr[2];
                }

                Instr.ByteSize = 4;

                return;
            }

            if ( d )
            {
                Instr.RegDst = reg;
                Instr.MemRegSrc = r_m;
            }
            else
            {
                Instr.RegSrc = reg;
                Instr.MemRegDst = r_m;
            }

            Instr.ByteSize = 2;
            return;
        }

    case 0b01:
        {
            if ( d )
            {
                Instr.RegDst = reg;
                Instr.MemRegSrc = r_m;
                Instr.DisplSrc = InstrPtr[2];
            }
            else
            {
                Instr.RegSrc = reg;
                Instr.MemRegDst = r_m;
                Instr.DisplDst = InstrPtr[2];
            }

            Instr.ByteSize = 3;
            return;
        }

    case 0b10:
        {
            if ( d )
            {
                Instr.RegDst = reg;
                Instr.MemRegSrc = r_m;
                Instr.DisplSrc = *(uint16*)&InstrPtr[2];
            }
            else
            {
                Instr.RegSrc = reg;
                Instr.MemRegDst = r_m;
                Instr.DisplDst = *(uint16*)&InstrPtr[2];
            }

            Instr.ByteSize = 4;
            return;
        }

    default:
        {
            printf( "ERROR: Incorrect instruction!\n" );
            return;
        }
    }
}

void DecodeImmToRegMem( const uint8* InstrPtr, Instruction& Instr, bool IsWide )
{
    uint8 mod = ( InstrPtr[1] & 0b11000000 ) >> 6;
    uint8 r_m = ( InstrPtr[1] & 0b00000111 );

    switch ( mod )
    {
    case 0b11:
        {
            Instr.RegDst = r_m;
            Instr.Immediate = IsWide ? *(uint16*)&InstrPtr[2] : InstrPtr[2];

            Instr.ByteSize = IsWide ? 4 : 3;
            return;
        }

    case 0b00:
        {
            if ( r_m == 0b110 )
            {
                Instr.DisplDst = *(uint16*)&InstrPtr[2];
                Instr.Immediate = IsWide ? *(uint16*)&InstrPtr[4] : InstrPtr[4];

                Instr.ByteSize = IsWide ? 6 : 5;
                return;
            }

            Instr.MemRegDst = r_m;
            Instr.Immediate = IsWide ? *(uint16*)&InstrPtr[2] : InstrPtr[2];

            Instr.ByteSize = IsWide ? 4 : 3;
            return;
        }

    case 0b01:
        {
            Instr.MemRegDst = r_m;
            Instr.DisplDst = InstrPtr[2];
            Instr.Immediate = IsWide ? *(uint16*)&InstrPtr[3] : InstrPtr[3];

            Instr.ByteSize = IsWide ? 5 : 4;
            return;
        }

    case 0b10:
        {
            Instr.MemRegDst = r_m;
            Instr.DisplDst = *(uint16*)&InstrPtr[2];
            Instr.Immediate = IsWide ? *(uint16*)&InstrPtr[4] : InstrPtr[4];

            Instr.ByteSize = IsWide ? 6 : 5;
            return;
        }

    default:
        {
            printf( "ERROR: Incorrect instruction!\n" );
            return;
        }
    }
}

void DecodeJump( const uint8* InstrPtr, Instruction& Instr )
{
    switch ( InstrPtr[0] )
    {
    case 0b01110100:
        Instr.Name = IName::JE;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01111100:
        Instr.Name = IName::JL;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01111110:
        Instr.Name = IName::JLE;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01110010:
        Instr.Name = IName::JB;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01110110:
        Instr.Name = IName::JBE;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01111010:
        Instr.Name = IName::JP;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01110000:
        Instr.Name = IName::JO;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01111000:
        Instr.Name = IName::JS;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01110101:
        Instr.Name = IName::JNE;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01111101:
        Instr.Name = IName::JNL;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01111111:
        Instr.Name = IName::JNLE;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01110011:
        Instr.Name = IName::JNB;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01110111:
        Instr.Name = IName::JNBE;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01111011:
        Instr.Name = IName::JNP;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01110001:
        Instr.Name = IName::JNO;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b01111001:
        Instr.Name = IName::JNS;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b11100010:
        Instr.Name = IName::LOOP;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b11100001:
        Instr.Name = IName::LOOPZ;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b11100000:
        Instr.Name = IName::LOOPNZ;
        Instr.Displacement = InstrPtr[1];
        break;

    case 0b11100011:
        Instr.Name = IName::JCXZ;
        Instr.Displacement = InstrPtr[1];
        break;
    }

    Instr.ByteSize = 2;
}

void ExecuteJump( const Instruction& Instr, RegisterFile& RegFile )
{
    switch ( Instr.Name )
    {
    case IName::JE:
        {
            if ( RegFile.ZF )
            {
                RegFile.IP += static_cast<int8>( Instr.Displacement );
            }

            return;
        }

    case IName::JNBE:
    case IName::JS:
    case IName::JL:
        {
            if ( RegFile.SF )
            {
                RegFile.IP += static_cast<int8>( Instr.Displacement );
            }

            return;
        }

    case IName::JNB:
    case IName::JLE:
        {
            if ( RegFile.SF || RegFile.ZF )
            {
                RegFile.IP += static_cast<int8>( Instr.Displacement );
            }

            return;
        }

    case IName::JNS:
    case IName::JNLE:
    case IName::JB:
        {
            if ( !RegFile.SF )
            {
                RegFile.IP += static_cast<int8>( Instr.Displacement );
            }

            return;
        }

    case IName::JNL:
    case IName::JBE:
        {
            if ( !RegFile.SF || RegFile.ZF )
            {
                RegFile.IP += static_cast<int8>( Instr.Displacement );
            }

            return;
        }

    case IName::JNE:
        {
            if ( !RegFile.ZF )
            {
                RegFile.IP += static_cast<int8>( Instr.Displacement );
            }

            return;
        }

    default:
        {
            if ( Instr.Name >= IName::JE )
            {
                printf( "ERROR: JMP instruction not implemented!\n" );
            }

            break;
        }
    }
}

Instruction DecodeInstruction( const uint8* InstrPtr )
{
    Instruction Instr{};
    switch ( InstrPtr[0] >> 2 )
    {
    case 0b100010:
        {
            Instr.Name = IName::MOV;
            DecodeRegToRegMem( InstrPtr, Instr );
            return Instr;
        }

    case 0b000000:
        {
            Instr.Name = IName::ADD;
            DecodeRegToRegMem( InstrPtr, Instr );
            return Instr;
        }

    case 0b001010:
        {
            Instr.Name = IName::SUB;
            DecodeRegToRegMem( InstrPtr, Instr );
            return Instr;
        }

    case 0b001110:
        {
            Instr.Name = IName::CMP;
            DecodeRegToRegMem( InstrPtr, Instr );
            return Instr;
        }

    case 0b000001:
        {
            Instr.Name = IName::ADD;
            Instr.Wide = InstrPtr[0] & 0b00000001;

            Instr.RegDst = 0;
            Instr.Immediate = Instr.Wide ? *(uint16*)&InstrPtr[1] : InstrPtr[1];

            Instr.ByteSize = Instr.Wide ? 3 : 2;

            return Instr;
        }

    case 0b001011:
        {
            Instr.Name = IName::SUB;
            Instr.Wide = InstrPtr[0] & 0b00000001;

            Instr.RegDst = 0;
            Instr.Immediate = Instr.Wide ? *(uint16*)&InstrPtr[1] : InstrPtr[1];

            Instr.ByteSize = Instr.Wide ? 3 : 2;

            return Instr;
        }

    case 0b001111:
        {
            Instr.Name = IName::CMP;
            Instr.Wide = InstrPtr[0] & 0b00000001;

            Instr.RegDst = 0;
            Instr.Immediate = Instr.Wide ? *(uint16*)&InstrPtr[1] : InstrPtr[1];

            Instr.ByteSize = Instr.Wide ? 3 : 2;

            return Instr;
        }

    case 0b100000:
        {
            Instr.Wide = InstrPtr[0] & 0b00000001;
            if ( 0 == ((InstrPtr[1] >> 3) & 0b00111) )
            {
                Instr.Name = IName::ADD;
                DecodeImmToRegMem( InstrPtr, Instr, Instr.Wide && !(InstrPtr[0] & 0b00000010) );
            }
            else if ( 0b101 == ((InstrPtr[1] >> 3) & 0b00111) )
            {
                Instr.Name = IName::SUB;
                DecodeImmToRegMem( InstrPtr, Instr, Instr.Wide && !(InstrPtr[0] & 0b00000010) );
            }
            else if ( 0b111 == ((InstrPtr[1] >> 3) & 0b00111) )
            {
                Instr.Name = IName::CMP;
                DecodeImmToRegMem( InstrPtr, Instr, Instr.Wide && !(InstrPtr[0] & 0b00000010) );
            }

            return Instr;
        }

    default:
        break;
    }

    if ( 0b1100011 == (InstrPtr[0] >> 1) )
    {
        Instr.Name = IName::MOV;
        Instr.Wide = InstrPtr[0] & 0b00000001;
        DecodeImmToRegMem( InstrPtr, Instr, Instr.Wide );
        return Instr;
    }
    else if ( 0b1011 == (InstrPtr[0] >> 4) )
    {
        uint8 Reg = InstrPtr[0] & 0b00000111;
        Instr.Wide = InstrPtr[0] & 0b00001000;

        Instr.Name = IName::MOV;
        Instr.RegDst = Reg;
        Instr.Immediate = Instr.Wide ? *(uint16*)&InstrPtr[1] : InstrPtr[1];

        Instr.ByteSize = Instr.Wide ? 3 : 2;
        return Instr;
    }

    DecodeJump( InstrPtr, Instr );

    return Instr;
}

void PrintInstruction( const Instruction& Instr )
{
    printf( "%s", InstrNames[ (uint16)Instr.Name ] );

    if ( Instr.Name >= IName::JE )
    {
        printf( " %d\n", static_cast<int8>( Instr.Displacement ) );
        return;
    }

    // Print destination
    if ( Instr.RegDst != UINT8_MAX )
    {
        printf( " %s", GetRegisterName( Instr.RegDst, Instr.Wide ) );
    }
    else if ( Instr.MemRegDst != UINT8_MAX && Instr.DisplDst != UINT8_MAX )
    {
        printf( " [%s + %d]", GRegMemTable[ Instr.MemRegDst ], Instr.DisplDst );
    }
    else if ( Instr.MemRegDst != UINT8_MAX )
    {
        printf( " [%s]", GRegMemTable[ Instr.MemRegDst ] );
    }
    else if ( Instr.DisplDst != UINT16_MAX )
    {
        printf( " [%d]", Instr.DisplDst );
    }

    // Print source
    if ( Instr.RegSrc != UINT8_MAX )
    {
        printf( ", %s", GetRegisterName( Instr.RegSrc, Instr.Wide ) );
    }
    else if ( Instr.Immediate != UINT16_MAX )
    {
        printf( ", %d", Instr.Immediate );
    }
    else if ( Instr.MemRegSrc != UINT8_MAX && Instr.DisplSrc != UINT8_MAX )
    {
        printf( ", [%s + %d]", GRegMemTable[ Instr.MemRegSrc ], Instr.DisplSrc );
    }
    else if ( Instr.MemRegSrc != UINT8_MAX )
    {
        printf( ", [%s]", GRegMemTable[ Instr.MemRegSrc ] );
    }
    else if ( Instr.DisplSrc != UINT16_MAX )
    {
        printf( ", [%d]", Instr.DisplSrc );
    }

    printf( "\n" );
}

void SetFlags( uint16 Result, RegisterFile& RegFile )
{
    RegFile.ZF = Result == 0;
    RegFile.SF = Result & 0b1000'0000'0000'0000;

    printf( "ZF: %d\n", RegFile.ZF );
    printf( "SF: %d\n", RegFile.SF );
}

void ExecuteInstruction( const Instruction& Instr, Storage& Strg )
{
    switch ( Instr.Name )
    {
    case IName::MOV:
        {
            if ( Instr.RegDst != UINT8_MAX && Instr.RegSrc != UINT8_MAX )
            {
                uint16 Prev = Strg.RegFile.GPRs[ Instr.RegDst ];
                Strg.RegFile.GPRs[ Instr.RegDst ] = Strg.RegFile.GPRs[ Instr.RegSrc ];
                printf( "%s: 0x%04x => 0x%04x\n", GRegTableX[ Instr.RegDst ], Prev, Strg.RegFile.GPRs[ Instr.RegDst ] );
            }
            else if ( Instr.RegDst != UINT8_MAX && Instr.Immediate != UINT16_MAX )
            {
                uint16 Prev = Strg.RegFile.GPRs[ Instr.RegDst ];
                Strg.RegFile.GPRs[ Instr.RegDst ] = Instr.Immediate;
                printf( "%s: 0x%04x => 0x%04x\n", GRegTableX[ Instr.RegDst ], Prev, Strg.RegFile.GPRs[ Instr.RegDst ] );
            }
            else if ( ( Instr.MemRegDst != UINT8_MAX || Instr.DisplDst != UINT16_MAX ) && ( Instr.RegSrc != UINT8_MAX || Instr.Immediate != UINT16_MAX ) )
            {
                uint16 Address = CalculateMemoryAddress( Instr, Strg.RegFile );
                if ( Address == UINT16_MAX )
                {
                    printf( "ERROR: Invalid memory address!\n" );
                    break;
                }

                uint16 Value = Instr.RegSrc != UINT8_MAX ? Strg.RegFile.GPRs[ Instr.RegSrc ] : Instr.Immediate;
                Strg.Memory[ Address ] = (uint8)( Value & 0x00ff );
                printf( "Memory[%d] = %d\n", Address, Strg.Memory[ Address ] );

                if ( Instr.Wide )
                {
                    Strg.Memory[ Address + 1 ] = (uint8)( Value >> 8 );
                    printf( "Memory[%d] = %d\n", Address + 1, Strg.Memory[ Address + 1 ] );
                }
            }
            else if ( Instr.RegDst != UINT8_MAX && ( Instr.MemRegSrc != UINT8_MAX || Instr.DisplSrc != UINT16_MAX ) )
            {
                uint16 Address = CalculateMemoryAddress( Instr, Strg.RegFile );
                if ( Address == UINT16_MAX )
                {
                    printf( "ERROR: Invalid memory address!\n" );
                    break;
                }

                uint16 ValueL = Strg.Memory[ Address ];
                uint16 ValueH = Strg.Memory[ Address + 1 ];

                Strg.RegFile.GPRs[ Instr.RegDst ] = ( ValueH << 8 ) | ( ValueL & 0x00ff );

                printf( "%s = %d\n", GetRegisterName( Instr.RegDst, Instr.Wide ), Strg.RegFile.GPRs[ Instr.RegDst ] );
            }

            break;
        }

    case IName::ADD:
        {
            if ( Instr.RegDst != UINT8_MAX && Instr.RegSrc != UINT8_MAX )
            {
                uint16 Prev = Strg.RegFile.GPRs[ Instr.RegDst ];
                Strg.RegFile.GPRs[ Instr.RegDst ] += Strg.RegFile.GPRs[ Instr.RegSrc ];
                printf( "%s: 0x%04x => 0x%04x\n", GRegTableX[ Instr.RegDst ], Prev, Strg.RegFile.GPRs[ Instr.RegDst ] );
            }
            else if ( Instr.RegDst != UINT8_MAX && Instr.Immediate != UINT16_MAX )
            {
                uint16 Prev = Strg.RegFile.GPRs[ Instr.RegDst ];
                Strg.RegFile.GPRs[ Instr.RegDst ] += Instr.Immediate;
                printf( "%s: 0x%04x => 0x%04x\n", GRegTableX[ Instr.RegDst ], Prev, Strg.RegFile.GPRs[ Instr.RegDst ] );
            }

            if ( Instr.RegDst != UINT8_MAX )
            {
                SetFlags( Strg.RegFile.GPRs[ Instr.RegDst ], Strg.RegFile );
            }

            break;
        }

    case IName::SUB:
        {
            if ( Instr.RegDst != UINT8_MAX && Instr.RegSrc != UINT8_MAX )
            {
                uint16 Prev = Strg.RegFile.GPRs[ Instr.RegDst ];
                Strg.RegFile.GPRs[ Instr.RegDst ] -= Strg.RegFile.GPRs[ Instr.RegSrc ];
                printf( "%s: 0x%04x => 0x%04x\n", GRegTableX[ Instr.RegDst ], Prev, Strg.RegFile.GPRs[ Instr.RegDst ] );
            }
            else if ( Instr.RegDst != UINT8_MAX && Instr.Immediate != UINT16_MAX )
            {
                uint16 Prev = Strg.RegFile.GPRs[ Instr.RegDst ];
                Strg.RegFile.GPRs[ Instr.RegDst ] -= Instr.Immediate;
                printf( "%s: 0x%04x => 0x%04x\n", GRegTableX[ Instr.RegDst ], Prev, Strg.RegFile.GPRs[ Instr.RegDst ] );
            }

            if ( Instr.RegDst != UINT8_MAX )
            {
                SetFlags( Strg.RegFile.GPRs[ Instr.RegDst ], Strg.RegFile );
            }

            break;
        }

    case IName::CMP:
        {
            if ( Instr.RegDst != UINT8_MAX && Instr.RegSrc != UINT8_MAX )
            {
                uint16 Res = Strg.RegFile.GPRs[ Instr.RegDst ] - Strg.RegFile.GPRs[ Instr.RegSrc ];
                SetFlags( Res, Strg.RegFile );
            }
            else if ( Instr.RegDst != UINT8_MAX && Instr.Immediate != UINT16_MAX )
            {
                uint16 Res = Strg.RegFile.GPRs[ Instr.RegDst ] - Instr.Immediate;
                SetFlags( Res, Strg.RegFile );
            }

            break;
        }

    default:
        break;
    }

    Strg.RegFile.IP += Instr.ByteSize;

    ExecuteJump( Instr, Strg.RegFile );
}

void Simulate8086( Storage& Strg )
{
    const uint16 ProgramSize = *(uint16*)&Strg.Memory[0];

    for (;;)
    {
        Instruction Instr = DecodeInstruction( Strg.Memory + 2 + Strg.RegFile.IP );
        PrintInstruction( Instr );
        ExecuteInstruction( Instr, Strg );

        printf( "----------------\n" );

        if ( Strg.RegFile.IP >= ProgramSize )
        {
            break;
        }
    }
}

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        return -1;
    }

    const char* FileName = argv[1];

    FILE* InputFile = fopen( FileName, "rb" );
    fseek( InputFile, 0L, SEEK_END );
    long FileSize = ftell( InputFile );
    fseek( InputFile, 0L, SEEK_SET );

    if ( FileSize >= UINT16_MAX )
    {
        printf( "ERROR: the program is too long!" );
        return fclose( InputFile );
    }

    const uint16 ProgramSize = (uint16)FileSize;

    Storage Strg{};

    // Copy the program size into the first 2 bytes of memory
    memcpy( &Strg.Memory[0], &ProgramSize, 2 );

    // Copy the program itself into memory starting from byte 2
    fread( &Strg.Memory[2], ProgramSize, 1, InputFile );
    fclose( InputFile );

    Simulate8086( Strg );

    printf( "\n\nFinal registers:\n" );

    for ( int i = 0; i < sizeof(GRegTableX) / sizeof(GRegTableX[0]); i++ )
    {
        printf( "%s: 0x%04x\n", GRegTableX[i], Strg.RegFile.GPRs[i] );
    }

    printf( "ZF: %d\n", Strg.RegFile.ZF );
    printf( "SF: %d\n", Strg.RegFile.SF );
    printf( "IP: %d\n", Strg.RegFile.IP );

    FILE* DumpFile = fopen( "mem_dump.bin", "wb" );
    fwrite( Strg.Memory, sizeof( Strg.Memory ), 1, DumpFile );

    return fclose( DumpFile );
}