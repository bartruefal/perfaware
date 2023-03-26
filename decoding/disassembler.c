#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define bool unsigned char

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

unsigned short register_file[ sizeof(regTableX) / sizeof(regTableX[0]) ];

const char* reg_name( unsigned char reg, unsigned char w )
{
    return w ? regTableX[ reg ] : regTableL[ reg ];
}

int value_cast( const unsigned char* reg, unsigned char w )
{
    return w ? *(const unsigned short*)reg : *reg;
}

int instr_reg_r_m( const char* instr_name, const unsigned char* instr, bool execute )
{
    unsigned char d = instr[0] & 0b00000010;
    unsigned char w = instr[0] & 0b00000001;
    unsigned char mod = ( instr[1] & 0b11000000 ) >> 6;
    unsigned char reg = ( instr[1] & 0b00111000 ) >> 3;
    unsigned char r_m = ( instr[1] & 0b00000111 );

    switch ( mod )
    {
    case 0b11:
        {
            printf( "%s %s, %s\n", instr_name, reg_name( d ? reg : r_m, w ), reg_name( d ? r_m : reg, w ) );

            if ( execute && w )
            {
                register_file[ d ? reg : r_m ] = register_file[ d ? r_m : reg ];
            }

            return 2;
        }

    case 0b00:
        {
            if ( r_m == 0b110 )
            {
                if ( d )
                    printf( "%s %s, [%d]\n", instr_name, reg_name( reg, w ), value_cast( &instr[2], 1 ) );
                else
                    printf( "%s [%d], %s\n", instr_name, value_cast( &instr[2], 1 ), reg_name( reg, w ) );

                return 4;
            }

            if ( d )
                printf( "%s %s, [%s]\n", instr_name, reg_name( reg, w ), regMemTable[ r_m ] );
            else
                printf( "%s [%s], %s\n", instr_name, regMemTable[ r_m ], reg_name( reg, w ) );

            return 2;
        }

    case 0b01:
        {
            if ( d )
                printf( "%s %s, [%s + %d]\n", instr_name, reg_name( reg, w ), regMemTable[ r_m ], instr[2] );
            else
                printf( "%s [%s + %d], %s\n", instr_name, regMemTable[ r_m ], instr[2], reg_name( reg, w ) );

            return 3;
        }

    case 0b10:
        {
            if ( d )
                printf( "%s %s, [%s + %d]\n", instr_name, reg_name( reg, w ), regMemTable[ r_m ], value_cast( &instr[2], 1 ) );
            else
                printf( "%s [%s + %d], %s\n", instr_name, regMemTable[ r_m ], value_cast( &instr[2], 1 ), reg_name( reg, w ) );

            return 4;
        }

    default:
        break;
    }

    printf( "ERROR: Incorrect instruction!\n" );
    return -1;
}

int instr_imm_to_r_m( const char* instr_name, const unsigned char* instr, char s )
{
    unsigned char w = instr[0] & 0b00000001;
    unsigned char mod = ( instr[1] & 0b11000000 ) >> 6;
    unsigned char r_m = ( instr[1] & 0b00000111 );

    char wide = w && !s;

    switch ( mod )
    {
    case 0b11:
        {
            printf( "%s %s, %d\n", instr_name, reg_name( r_m, w ), value_cast( &instr[2], wide ) );
            return wide ? 4 : 3;
        }

    case 0b00:
        {
            if ( r_m == 0b110 )
            {
                printf( "%s [%d], %d\n", instr_name, value_cast( &instr[2], 1 ), value_cast( &instr[4], wide ) );
                return wide ? 6 : 5;
            }

            printf( "%s [%s], %d\n", instr_name, regMemTable[ r_m ], value_cast( &instr[2], wide ) );
            return wide ? 4 : 3;
        }

    case 0b01:
        {
            printf( "%s [%s + %d], %d\n", instr_name, regMemTable[ r_m ], instr[2], value_cast( &instr[3], wide ) );
            return wide ? 5 : 4;
        }

    case 0b10:
        {
            printf( "%s [%s + %d], %d\n", instr_name, regMemTable[ r_m ], value_cast( &instr[2], 1 ), value_cast( &instr[4], wide ) );
            return wide ? 6 : 5;
        }

    default:
        break;
    }

    printf( "ERROR: Incorrect instruction!\n" );
    return -1;
}

int mov_imm_to_reg( const unsigned char* instr, bool execute )
{
    char reg = instr[0] & 0b00000111;
    char w = instr[0] & 0b00001000;

    printf( "mov %s, %d\n", reg_name( reg, w ), value_cast( &instr[1], w ) );

    if ( execute && w )
    {
        register_file[reg] = *(unsigned short*)(&instr[1]);
    }

    return w ? 3 : 2;
}

int instr_imm_to_accum( const char* instr_name, const unsigned char* instr )
{
    char w = instr[0] & 0b00000001;
    printf( "%s %s, %d\n", instr_name, w ? "AX" : "AL", value_cast( &instr[1], w ) );

    return w ? 3 : 2;
}

int decode_reg_mem( const unsigned char* instr_stream, bool execute )
{
    switch ( instr_stream[0] >> 2 )
    {
    case 0b100010:
        return instr_reg_r_m( "mov", instr_stream, execute );

    case 0b000000:
        return instr_reg_r_m( "add", instr_stream, 0 );

    case 0b001010:
        return instr_reg_r_m( "sub", instr_stream, 0 );

    case 0b001110:
        return instr_reg_r_m( "cmp", instr_stream, 0 );

    case 0b000001:
        return instr_imm_to_accum( "add", instr_stream );

    case 0b001011:
        return instr_imm_to_accum( "sub", instr_stream );

    case 0b001111:
        return instr_imm_to_accum( "cmp", instr_stream );

    case 0b100000:
        {
            char s = instr_stream[0] & 0b00000010;
            if ( 0 == ((instr_stream[1] >> 3) & 0b00111) )
            {
                return instr_imm_to_r_m( "add", instr_stream, s );
            }
            else if ( 0b101 == ((instr_stream[1] >> 3) & 0b00111) )
            {
                return instr_imm_to_r_m( "sub", instr_stream, s );
            }
            else if ( 0b111 == ((instr_stream[1] >> 3) & 0b00111) )
            {
                return instr_imm_to_r_m( "cmp", instr_stream, s );
            }
        }

    default:
        break;
    }

    if ( 0b1100011 == (instr_stream[0] >> 1) )
    {
        return instr_imm_to_r_m( "mov", instr_stream, 0 );
    }
    else if ( 0b1011 == (instr_stream[0] >> 4) )
    {
        return mov_imm_to_reg( instr_stream, execute );
    }

    return -1;
}

int decode_jmp( const unsigned char* instr_stream )
{
    switch ( instr_stream[0] )
    {
    case 0b01110100:
        printf( "je %d\n", instr_stream[1] );
        break;

    case 0b01111100:
        printf( "jl %d\n", instr_stream[1] );
        break;

    case 0b01111110:
        printf( "jle %d\n", instr_stream[1] );
        break;

    case 0b01110010:
        printf( "jb %d\n", instr_stream[1] );
        break;

    case 0b01110110:
        printf( "jbe %d\n", instr_stream[1] );
        break;

    case 0b01111010:
        printf( "jp %d\n", instr_stream[1] );
        break;

    case 0b01110000:
        printf( "jo %d\n", instr_stream[1] );
        break;

    case 0b01111000:
        printf( "js %d\n", instr_stream[1] );
        break;

    case 0b01110101:
        printf( "jne %d\n", instr_stream[1] );
        break;

    case 0b01111101:
        printf( "jnl %d\n", instr_stream[1] );
        break;

    case 0b01111111:
        printf( "jnle %d\n", instr_stream[1] );
        break;

    case 0b01110011:
        printf( "jnb %d\n", instr_stream[1] );
        break;

    case 0b01110111:
        printf( "jnbe %d\n", instr_stream[1] );
        break;

    case 0b01111011:
        printf( "jnp %d\n", instr_stream[1] );
        break;

    case 0b01110001:
        printf( "jno %d\n", instr_stream[1] );
        break;

    case 0b01111001:
        printf( "jns %d\n", instr_stream[1] );
        break;

    case 0b11100010:
        printf( "loop %d\n", instr_stream[1] );
        break;

    case 0b11100001:
        printf( "loopz %d\n", instr_stream[1] );
        break;

    case 0b11100000:
        printf( "loopnz %d\n", instr_stream[1] );
        break;

    case 0b11100011:
        printf( "jcxz %d\n", instr_stream[1] );
        break;

    default:
        return 0;
    }

    return 2;
}

void disassemble( const unsigned char* instr_stream, int size, bool execute )
{
    while ( size )
    {
        int instr_size = decode_reg_mem( instr_stream, execute );
        if ( instr_size == -1 )
        {
            instr_size = decode_jmp( instr_stream );
        }

        if ( instr_size < 0 )
        {
            printf( "ERROR: Unknown instruction!\n" );
            return;
        }

        instr_stream += instr_size;
        size -= instr_size;
    }
}

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        return -1;
    }

    bool execute = (bool)(strncmp(argv[1], "-exec", 5) == 0);
    if ( execute && argc != 3 )
    {
        return -1;
    }

    for ( int i = 0; i < sizeof(register_file) / sizeof(register_file[0]); i++ )
    {
        register_file[i] = 0;
    }

    const char* file_name = execute ? argv[2] : argv[1];

    FILE* input_file = fopen( file_name, "rb" );
    fseek( input_file, 0L, SEEK_END );
    int file_size = ftell(input_file);
    fseek( input_file, 0L, SEEK_SET );

    unsigned char* buf = malloc( file_size );
    fread( buf, file_size, 1, input_file );

    disassemble( buf, file_size, execute );

    free( buf );

    for ( int i = 0; i < sizeof(register_file) / sizeof(register_file[0]); i++ )
    {
        printf( "%s: 0x%#04x\n", regTableX[i], register_file[i] );
    }

    return fclose( input_file );
}