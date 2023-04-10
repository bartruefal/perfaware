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

const char* regTableL[] = {
    "AL",
    "CL",
    "DL",
    "BL",
    "AH",
    "CH",
    "DH",
    "BH"
};

const char* regTableX[] = {
    "AX",
    "CX",
    "DX",
    "BX",
    "SP",
    "BP",
    "SI",
    "DI"
};

const char* regMemTable[] = {
    "BX + SI",
    "BX + DI",
    "BP + SI",
    "BP + DI",
    "SI",
    "DI",
    "BP",
    "BX"
};

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
    uint16 RegSrc = UINT16_MAX;
    uint16 MemRegSrc = UINT16_MAX;
    uint16 DisplSrc = UINT16_MAX;
    uint16 RegDst = UINT16_MAX;
    uint16 MemRegDst = UINT16_MAX;
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
    uint16 GPRs[ sizeof(regTableX) / sizeof(regTableX[0]) ];
    uint16 IP;
    bool ZF;
    bool SF;
};

const char* reg_name( uint8 reg, bool wide )
{
    return wide ? regTableX[ reg ] : regTableL[ reg ];
}

void decode_reg_r_m( const uint8* instr_ptr, Instruction& instr )
{
    uint8 d = instr_ptr[0] & 0b00000010;
    uint8 w = instr_ptr[0] & 0b00000001;
    uint8 mod = ( instr_ptr[1] & 0b11000000 ) >> 6;
    uint8 reg = ( instr_ptr[1] & 0b00111000 ) >> 3;
    uint8 r_m = ( instr_ptr[1] & 0b00000111 );

    instr.Wide = w;

    switch ( mod )
    {
    case 0b11:
        {
            instr.RegDst = d ? reg : r_m;
            instr.RegSrc = d ? r_m : reg;

            instr.ByteSize = 2;
            return;
        }

    case 0b00:
        {
            if ( r_m == 0b110 )
            {
                if ( d )
                {
                    instr.RegDst = reg;
                    instr.DisplSrc = *(uint16*)&instr_ptr[2];
                }
                else
                {
                    instr.RegSrc = reg;
                    instr.DisplDst = *(uint16*)&instr_ptr[2];
                }

                instr.ByteSize = 4;

                return;
            }

            if ( d )
            {
                instr.RegDst = reg;
                instr.MemRegSrc = r_m;
            }
            else
            {
                instr.RegSrc = reg;
                instr.MemRegDst = r_m;
            }

            instr.ByteSize = 2;
            return;
        }

    case 0b01:
        {
            if ( d )
            {
                instr.RegDst = reg;
                instr.MemRegSrc = r_m;
                instr.DisplSrc = instr_ptr[2];
            }
            else
            {
                instr.RegSrc = reg;
                instr.MemRegDst = r_m;
                instr.DisplDst = instr_ptr[2];
            }

            instr.ByteSize = 3;
            return;
        }

    case 0b10:
        {
            if ( d )
            {
                instr.RegDst = reg;
                instr.MemRegSrc = r_m;
                instr.DisplSrc = *(uint16*)&instr_ptr[2];
            }
            else
            {
                instr.RegSrc = reg;
                instr.MemRegDst = r_m;
                instr.DisplDst = *(uint16*)&instr_ptr[2];
            }

            instr.ByteSize = 4;
            return;
        }

    default:
        {
            printf( "ERROR: Incorrect instruction!\n" );
            return;
        }
    }
}

void decode_imm_to_r_m( const uint8* instr_ptr, Instruction& instr )
{
    uint8 s = instr_ptr[0] & 0b00000010;
    uint8 mod = ( instr_ptr[1] & 0b11000000 ) >> 6;
    uint8 r_m = ( instr_ptr[1] & 0b00000111 );

    bool wide = instr.Wide && !s;

    switch ( mod )
    {
    case 0b11:
        {
            instr.RegDst = r_m;
            instr.Immediate = wide ? *(uint16*)&instr_ptr[2] : instr_ptr[2];

            instr.ByteSize = wide ? 4 : 3;
            return;
        }

    case 0b00:
        {
            if ( r_m == 0b110 )
            {
                instr.DisplDst = instr_ptr[2];
                instr.Immediate = wide ? *(uint16*)&instr_ptr[4] : instr_ptr[4];

                instr.ByteSize = wide ? 6 : 5;
                return;
            }

            instr.MemRegDst = r_m;
            instr.Immediate = wide ? *(uint16*)&instr_ptr[2] : instr_ptr[2];

            instr.ByteSize = wide ? 4 : 3;
            return;
        }

    case 0b01:
        {
            instr.MemRegDst = r_m;
            instr.DisplDst = instr_ptr[2];
            instr.Immediate = wide ? *(uint16*)&instr_ptr[3] : instr_ptr[3];

            instr.ByteSize = wide ? 5 : 4;
            return;
        }

    case 0b10:
        {
            instr.MemRegDst = r_m;
            instr.DisplDst = *(uint16*)&instr_ptr[2];
            instr.Immediate = wide ? *(uint16*)&instr_ptr[4] : instr_ptr[4];

            instr.ByteSize = wide ? 6 : 5;
            return;
        }

    default:
        {
            printf( "ERROR: Incorrect instruction!\n" );
            return;
        }
    }
}

void decode_jmp( const uint8* instr_ptr, Instruction& instr )
{
    switch ( instr_ptr[0] )
    {
    case 0b01110100:
        instr.Name = IName::JE;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01111100:
        instr.Name = IName::JL;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01111110:
        instr.Name = IName::JLE;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01110010:
        instr.Name = IName::JB;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01110110:
        instr.Name = IName::JBE;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01111010:
        instr.Name = IName::JP;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01110000:
        instr.Name = IName::JO;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01111000:
        instr.Name = IName::JS;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01110101:
        instr.Name = IName::JNE;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01111101:
        instr.Name = IName::JNL;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01111111:
        instr.Name = IName::JNLE;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01110011:
        instr.Name = IName::JNB;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01110111:
        instr.Name = IName::JNBE;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01111011:
        instr.Name = IName::JNP;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01110001:
        instr.Name = IName::JNO;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b01111001:
        instr.Name = IName::JNS;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b11100010:
        instr.Name = IName::LOOP;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b11100001:
        instr.Name = IName::LOOPZ;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b11100000:
        instr.Name = IName::LOOPNZ;
        instr.Displacement = instr_ptr[1];
        break;

    case 0b11100011:
        instr.Name = IName::JCXZ;
        instr.Displacement = instr_ptr[1];
        break;
    }

    instr.ByteSize = 2;
}

void execute_jmp( const Instruction& instr, RegisterFile& reg_file )
{
    switch ( instr.Name )
    {
    case IName::JE:
        {
            if ( reg_file.ZF )
            {
                reg_file.IP += static_cast<int8>( instr.Displacement );
            }

            return;
        }

    case IName::JNBE:
    case IName::JS:
    case IName::JL:
        {
            if ( reg_file.SF )
            {
                reg_file.IP += static_cast<int8>( instr.Displacement );
            }

            return;
        }

    case IName::JNB:
    case IName::JLE:
        {
            if ( reg_file.SF || reg_file.ZF )
            {
                reg_file.IP += static_cast<int8>( instr.Displacement );
            }

            return;
        }

    case IName::JNS:
    case IName::JNLE:
    case IName::JB:
        {
            if ( !reg_file.SF )
            {
                reg_file.IP += static_cast<int8>( instr.Displacement );
            }

            return;
        }

    case IName::JNL:
    case IName::JBE:
        {
            if ( !reg_file.SF || reg_file.ZF )
            {
                reg_file.IP += static_cast<int8>( instr.Displacement );
            }

            return;
        }

    case IName::JNE:
        {
            if ( !reg_file.ZF )
            {
                reg_file.IP += static_cast<int8>( instr.Displacement );
            }

            return;
        }

    default:
        {
            if ( instr.Name >= IName::JE )
            {
                printf( "JMP instruction not implemented!\n" );
            }

            break;
        }
    }
}

Instruction decode_instruction( const uint8* instr_ptr )
{
    Instruction instr{};
    switch ( instr_ptr[0] >> 2 )
    {
    case 0b100010:
        {
            instr.Name = IName::MOV;
            decode_reg_r_m( instr_ptr, instr );
            return instr;
        }

    case 0b000000:
        {
            instr.Name = IName::ADD;
            decode_reg_r_m( instr_ptr, instr );
            return instr;
        }

    case 0b001010:
        {
            instr.Name = IName::SUB;
            decode_reg_r_m( instr_ptr, instr );
            return instr;
        }

    case 0b001110:
        {
            instr.Name = IName::CMP;
            decode_reg_r_m( instr_ptr, instr );
            return instr;
        }

    case 0b000001:
        {
            instr.Name = IName::ADD;
            instr.Wide = instr_ptr[0] & 0b00000001;

            instr.RegDst = 0;
            instr.Immediate = instr.Wide ? *(uint16*)&instr_ptr[1] : instr_ptr[1];

            instr.ByteSize = instr.Wide ? 3 : 2;

            return instr;
        }

    case 0b001011:
        {
            instr.Name = IName::SUB;
            instr.Wide = instr_ptr[0] & 0b00000001;

            instr.RegDst = 0;
            instr.Immediate = instr.Wide ? *(uint16*)&instr_ptr[1] : instr_ptr[1];

            instr.ByteSize = instr.Wide ? 3 : 2;

            return instr;
        }

    case 0b001111:
        {
            instr.Name = IName::CMP;
            instr.Wide = instr_ptr[0] & 0b00000001;

            instr.RegDst = 0;
            instr.Immediate = instr.Wide ? *(uint16*)&instr_ptr[1] : instr_ptr[1];

            instr.ByteSize = instr.Wide ? 3 : 2;

            return instr;
        }

    case 0b100000:
        {
            instr.Wide = instr_ptr[0] & 0b00000001;
            if ( 0 == ((instr_ptr[1] >> 3) & 0b00111) )
            {
                instr.Name = IName::ADD;
                decode_imm_to_r_m( instr_ptr, instr );
            }
            else if ( 0b101 == ((instr_ptr[1] >> 3) & 0b00111) )
            {
                instr.Name = IName::SUB;
                decode_imm_to_r_m( instr_ptr, instr );
            }
            else if ( 0b111 == ((instr_ptr[1] >> 3) & 0b00111) )
            {
                instr.Name = IName::CMP;
                decode_imm_to_r_m( instr_ptr, instr );
            }

            return instr;
        }

    default:
        break;
    }

    if ( 0b1100011 == (instr_ptr[0] >> 1) )
    {
        instr.Name = IName::MOV;
        decode_imm_to_r_m( instr_ptr, instr );
        return instr;
    }
    else if ( 0b1011 == (instr_ptr[0] >> 4) )
    {
        uint8 reg = instr_ptr[0] & 0b00000111;
        instr.Wide = instr_ptr[0] & 0b00001000;

        instr.Name = IName::MOV;
        instr.RegDst = reg;
        instr.Immediate = instr.Wide ? *(uint16*)&instr_ptr[1] : instr_ptr[1];

        instr.ByteSize = instr.Wide ? 3 : 2;
        return instr;
    }

    decode_jmp( instr_ptr, instr );

    return instr;
}

void print_instruction( const Instruction& instr )
{
    printf( "%s", InstrNames[ (uint16)instr.Name ] );

    if ( instr.Name >= IName::JE )
    {
        printf( " %d\n", static_cast<int8>( instr.Displacement ) );
        return;
    }

    // Print destination
    if ( instr.RegDst != UINT16_MAX )
    {
        printf( " %s", reg_name( instr.RegDst, instr.Wide ) );
    }
    else if ( instr.MemRegDst != UINT16_MAX && instr.DisplDst != UINT16_MAX )
    {
        printf( " [%s + %d]", regMemTable[ instr.MemRegDst ], instr.DisplDst );
    }
    else if ( instr.MemRegDst != UINT16_MAX )
    {
        printf( " [%s]", regMemTable[ instr.MemRegDst ] );
    }
    else if ( instr.DisplDst != UINT16_MAX )
    {
        printf( " [%d]", instr.DisplDst );
    }

    // Print source
    if ( instr.RegSrc != UINT16_MAX )
    {
        printf( ", %s", reg_name( instr.RegSrc, instr.Wide ) );
    }
    else if ( instr.Immediate != UINT16_MAX )
    {
        printf( ", %d", instr.Immediate );
    }
    else if ( instr.MemRegSrc != UINT16_MAX && instr.DisplSrc != UINT16_MAX )
    {
        printf( ", [%s + %d]", regMemTable[ instr.MemRegSrc ], instr.DisplSrc );
    }
    else if ( instr.MemRegSrc != UINT16_MAX )
    {
        printf( ", [%s]", regMemTable[ instr.MemRegSrc ] );
    }
    else if ( instr.DisplSrc != UINT16_MAX )
    {
        printf( ", [%d]", instr.DisplSrc );
    }

    printf( "\n" );
}

void set_flags( uint16 result, RegisterFile& reg_file )
{
    reg_file.ZF = result == 0;
    reg_file.SF = result & 0b1000'0000'0000'0000;

    printf( "ZF: %d\n", reg_file.ZF );
    printf( "SF: %d\n", reg_file.SF );
}

void execute_instruction( const Instruction& instr, RegisterFile& reg_file )
{
    switch ( instr.Name )
    {
    case IName::MOV:
        {
            if ( instr.RegDst != UINT16_MAX && instr.RegSrc != UINT16_MAX )
            {
                uint16 prev = reg_file.GPRs[ instr.RegDst ];
                reg_file.GPRs[ instr.RegDst ] = reg_file.GPRs[ instr.RegSrc ];
                printf( "%s: 0x%04x => 0x%04x\n", regTableX[ instr.RegDst ], prev, reg_file.GPRs[ instr.RegDst ] );
            }
            else if ( instr.RegDst != UINT16_MAX && instr.Immediate != UINT16_MAX )
            {
                uint16 prev = reg_file.GPRs[ instr.RegDst ];
                reg_file.GPRs[ instr.RegDst ] = instr.Immediate;
                printf( "%s: 0x%04x => 0x%04x\n", regTableX[ instr.RegDst ], prev, reg_file.GPRs[ instr.RegDst ] );
            }

            break;
        }

    case IName::ADD:
        {
            if ( instr.RegDst != UINT16_MAX && instr.RegSrc != UINT16_MAX )
            {
                uint16 prev = reg_file.GPRs[ instr.RegDst ];
                reg_file.GPRs[ instr.RegDst ] += reg_file.GPRs[ instr.RegSrc ];
                printf( "%s: 0x%04x => 0x%04x\n", regTableX[ instr.RegDst ], prev, reg_file.GPRs[ instr.RegDst ] );
            }
            else if ( instr.RegDst != UINT16_MAX && instr.Immediate != UINT16_MAX )
            {
                uint16 prev = reg_file.GPRs[ instr.RegDst ];
                reg_file.GPRs[ instr.RegDst ] += instr.Immediate;
                printf( "%s: 0x%04x => 0x%04x\n", regTableX[ instr.RegDst ], prev, reg_file.GPRs[ instr.RegDst ] );
            }

            if ( instr.RegDst != UINT16_MAX )
            {
                set_flags( reg_file.GPRs[ instr.RegDst ], reg_file );
            }

            break;
        }

    case IName::SUB:
        {
            if ( instr.RegDst != UINT16_MAX && instr.RegSrc != UINT16_MAX )
            {
                uint16 prev = reg_file.GPRs[ instr.RegDst ];
                reg_file.GPRs[ instr.RegDst ] -= reg_file.GPRs[ instr.RegSrc ];
                printf( "%s: 0x%04x => 0x%04x\n", regTableX[ instr.RegDst ], prev, reg_file.GPRs[ instr.RegDst ] );
            }
            else if ( instr.RegDst != UINT16_MAX && instr.Immediate != UINT16_MAX )
            {
                uint16 prev = reg_file.GPRs[ instr.RegDst ];
                reg_file.GPRs[ instr.RegDst ] -= instr.Immediate;
                printf( "%s: 0x%04x => 0x%04x\n", regTableX[ instr.RegDst ], prev, reg_file.GPRs[ instr.RegDst ] );
            }

            if ( instr.RegDst != UINT16_MAX )
            {
                set_flags( reg_file.GPRs[ instr.RegDst ], reg_file );
            }

            break;
        }

    case IName::CMP:
        {
            if ( instr.RegDst != UINT16_MAX && instr.RegSrc != UINT16_MAX )
            {
                uint16 res = reg_file.GPRs[ instr.RegDst ] - reg_file.GPRs[ instr.RegSrc ];
                set_flags( res, reg_file );
            }
            else if ( instr.RegDst != UINT16_MAX && instr.Immediate != UINT16_MAX )
            {
                uint16 res = reg_file.GPRs[ instr.RegDst ] - instr.Immediate;
                set_flags( res, reg_file );
            }

            break;
        }

    default:
        break;
    }

    reg_file.IP += instr.ByteSize;

    execute_jmp( instr, reg_file );
}

void simulate( const uint8* instr_stream, int size, RegisterFile& reg_file )
{
    for (;;)
    {
        Instruction instr = decode_instruction( instr_stream + reg_file.IP );
        print_instruction( instr );
        execute_instruction( instr, reg_file );

        printf( "----------------\n" );

        if ( reg_file.IP >= size )
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

    const char* file_name = argv[1];

    FILE* input_file = fopen( file_name, "rb" );
    fseek( input_file, 0L, SEEK_END );
    int file_size = ftell(input_file);
    fseek( input_file, 0L, SEEK_SET );

    uint8* buf = (uint8*)malloc( file_size );
    fread( buf, file_size, 1, input_file );

    RegisterFile reg_file{};
    simulate( buf, file_size, reg_file );

    free( buf );

    printf( "\n\nFinal registers:\n" );

    for ( int i = 0; i < sizeof(regTableX) / sizeof(regTableX[0]); i++ )
    {
        printf( "%s: 0x%04x\n", regTableX[i], reg_file.GPRs[i] );
    }

    printf( "ZF: %d\n", reg_file.ZF );
    printf( "SF: %d\n", reg_file.SF );
    printf( "IP: %d\n", reg_file.IP );

    return fclose( input_file );
}