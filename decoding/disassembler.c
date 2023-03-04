#include <stdio.h>

const char* regTableW0[] = {
    "AL",
    "CL",
    "DL",
    "BL",
    "AH",
    "CH",
    "DH",
    "BH"
};

const char* regTableW1[] = {
    "AX",
    "CX",
    "DX",
    "BX",
    "SP",
    "BP",
    "SI",
    "DI"
};

void disassemble( char instr[2] )
{
    char reg1ID = ( instr[1] & 0b00111000 ) >> 3;
    char reg2ID = ( instr[1] & 0b00000111 );
    char regDestID = ( instr[0] & 0b00000010 ) ? reg1ID : reg2ID;
    char regSrcID = ( instr[0] & 0b00000010 ) ? reg2ID : reg1ID;
    const char* regDest = ( instr[0] & 0b00000001 ) ? regTableW1[ regDestID ] : regTableW0[ regDestID ];
    const char* regSrc = ( instr[0] & 0b00000001 ) ? regTableW1[ regSrcID ] : regTableW0[ regSrcID ];

    printf( "mov %s, %s\n", regDest, regSrc );
}

int main( int argc, char** argv )
{
    if ( argc != 2 )
    {
        return -1;
    }

    FILE* inputFile = fopen( argv[1], "rb" );
    char buf[2];
    while ( fread( buf, 2, 1, inputFile ) != 0 )
    {
        disassemble( buf );
    }

    return fclose( inputFile );
}