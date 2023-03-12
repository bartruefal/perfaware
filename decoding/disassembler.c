#include <stdio.h>
#include <malloc.h>

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

const char* reg_name( unsigned char reg, unsigned char w )
{
    return w ? regTableX[ reg ] : regTableL[ reg ];
}

int value_cast( const unsigned char* reg, unsigned char w )
{
    return w ? *(const unsigned short*)reg : *reg;
}

int instr_reg_r_m( const char* instr_name, const unsigned char* instr )
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

    printf( "ERROR: Incorrect mov instruction!\n" );
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

    printf( "ERROR: Incorrect mov instruction!\n" );
    return -1;
}

int mov_imm_to_reg( const unsigned char* instr )
{
    char reg = instr[0] & 0b00000111;
    char w = instr[0] & 0b00001000;

    printf( "mov %s, %d\n", reg_name( reg, w ), value_cast( &instr[1], w ) );

    return w ? 3 : 2;
}

int instr_imm_to_accum( const char* instr_name, const unsigned char* instr )
{
    char w = instr[0] & 0b00000001;
    printf( "%s %s, %d\n", instr_name, w ? "AX" : "AL", value_cast( &instr[1], w ) );

    return w ? 3 : 2;
}

void disassemble( unsigned char* instr_stream, int size )
{
    while ( size )
    {
        int instr_size = 0;

        switch ( instr_stream[0] >> 2 )
        {
        case 0b100010:
            instr_size = instr_reg_r_m( "mov", instr_stream );
            break;
        
        case 0b000000:
            instr_size = instr_reg_r_m( "add", instr_stream );
            break;

        case 0b001010:
            instr_size = instr_reg_r_m( "sub", instr_stream );
            break;

        case 0b001110:
            instr_size = instr_reg_r_m( "cmp", instr_stream );
            break;

        case 0b000001:
            instr_size = instr_imm_to_accum( "add", instr_stream );
            break;

        case 0b001011:
            instr_size = instr_imm_to_accum( "sub", instr_stream );
            break;

        case 0b001111:
            instr_size = instr_imm_to_accum( "cmp", instr_stream );
            break;

        case 0b100000:
            {
                char s = instr_stream[0] & 0b00000010;
                if ( 0 == ((instr_stream[1] >> 3) & 0b00111) )
                {
                    instr_size = instr_imm_to_r_m( "add", instr_stream, s );
                }
                else if ( 0b101 == ((instr_stream[1] >> 3) & 0b00111) )
                {
                    instr_size = instr_imm_to_r_m( "sub", instr_stream, s );
                }
                else if ( 0b111 == ((instr_stream[1] >> 3) & 0b00111) )
                {
                    instr_size = instr_imm_to_r_m( "cmp", instr_stream, s );
                }

                break;
            }
        
        default:
            break;
        }

        if ( 0b1100011 == (instr_stream[0] >> 1) )
        {
            instr_size = instr_imm_to_r_m( "mov", instr_stream, 0 );
        }
        else if ( 0b1011 == (instr_stream[0] >> 4) )
        {
            instr_size = mov_imm_to_reg( instr_stream );
        }

        if ( instr_size == 0 )
        {
            printf( "ERROR: Unknown instruction!\n" );
            return;
        }
        else if ( instr_size < 0 )
        {
            return;
        }

        instr_stream += instr_size;
        size -= instr_size;
    }
}

int main( int argc, char** argv )
{
    if ( argc != 2 )
    {
        return -1;
    }

    FILE* input_file = fopen( argv[1], "rb" );
    fseek( input_file, 0L, SEEK_END );
    int file_size = ftell(input_file);
    fseek( input_file, 0L, SEEK_SET );

    unsigned char* buf = malloc( file_size );
    fread( buf, file_size, 1, input_file );

    disassemble( buf, file_size );

    free( buf );

    return fclose( input_file );
}