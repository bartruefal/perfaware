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

int mov_reg( unsigned char* instr )
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
            printf( "mov %s, %s\n", reg_name( d ? reg : r_m, w ), reg_name( d ? r_m : reg, w ) );
            return 2;
        }

    case 0b00:
        {
            if ( r_m == 0b110 )
            {
                if ( d )
                    printf( "mov %s, [%d]\n", reg_name( reg, w ), *(unsigned short*)(&instr[2]) );
                else
                    printf( "mov [%d], %s\n", *(unsigned short*)(&instr[2]), reg_name( reg, w ) );

                return 4;
            }

            if ( d )
                printf( "mov %s, [%s]\n", reg_name( reg, w ), regMemTable[ r_m ] );
            else
                printf( "mov [%s], %s\n", regMemTable[ r_m ], reg_name( reg, w ) );

            return 2;
        }

    case 0b01:
        {
            if ( d )
                printf( "mov %s, [%s + %d]\n", reg_name( reg, w ), regMemTable[ r_m ], instr[2] );
            else
                printf( "mov [%s + %d], %s\n", regMemTable[ r_m ], instr[2], reg_name( reg, w ) );

            return 3;
        }

    case 0b10:
        {
            if ( d )
                printf( "mov %s, [%s + %d]\n", reg_name( reg, w ), regMemTable[ r_m ], *(unsigned short*)(&instr[2]) );
            else
                printf( "mov [%s + %d], %s\n", regMemTable[ r_m ], *(unsigned short*)(&instr[2]), reg_name( reg, w ) );

            return 4;
        }

    default:
        break;
    }

    printf( "ERROR: Incorrect mov instruction!\n" );
    return -1;
}

int mov_imm_to_reg( unsigned char* instr )
{
    char reg = instr[0] & 0b00000111;
    char w = instr[0] & 0b00001000;
    if ( w )
    {
        printf( "mov %s, %d\n", regTableX[ reg ], *(unsigned short*)(&instr[1]) );
        return 3;
    }

    printf( "mov %s, %d\n", regTableL[ reg ], instr[1] );
    return 2;
}

void disassemble( unsigned char* instr_stream, int size )
{
    while ( size )
    {
        int instr_size;
        if ( 0b100010 == (instr_stream[0] >> 2) )
        {
            instr_size = mov_reg( instr_stream );
        }
        else if ( 0b1011 == (instr_stream[0] >> 4) )
        {
            instr_size = mov_imm_to_reg( instr_stream );
        }
        else
        {
            printf( "ERROR: Unknown instruction!\n" );
            return;
        }

        if ( instr_size < 0 )
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