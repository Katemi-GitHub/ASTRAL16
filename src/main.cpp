#include <iostream>
#include <sstream>
#include <fstream>
#include <windows.h>
#include <vector>
#include <string>
#include <bitset>

using namespace std;

using Byte = unsigned char;
using Word = unsigned short;

using u32 = unsigned int;

struct Mem {
    static constexpr u32 MAX_MEM = 1024 * 64;
    Byte Data[MAX_MEM];

    void Init() {
        string line;
        string token;
        ifstream kateRom( "src/rom.txt" );
        int i = 0;
        if( kateRom.is_open() ) {
            while( getline( kateRom, line, '\n' ) ) {
                stringstream ss( line );
                string byte;
                while ( getline( ss, byte, ' ' ) ) {
                    Byte value = static_cast< Byte >( stoi( byte, nullptr, 16 ) );
                    Data[i] = value;
                    i++;
                }
            }
        }
        kateRom.close();
    }

    // Read 1 byte
    Byte operator[]( u32 Address ) const {
        return Data[Address];
    }

    // Write 1 byte
    Byte& operator[]( u32 Address ) {
        return Data[Address];
    }
};

struct CPU {
    Word PC; // Program Counter
    Word SP; // Stack Pointer

    Byte A, X, Y; // Registers

    Byte C : 1; // Status Flag - Carry Flag
    Byte Z : 1; // Status Flag - Zero Flag
    Byte I : 1; // Status Flag - IRQ Disable Flag
    Byte D : 1; // Status Flag - Decimal mode Flag
    Byte B : 1; // Status Flag - Break command Flag
    Byte V : 1; // Status Flag - Overflow Flag
    Byte N : 1; // Status Flag - Negative Flag

    void Reset( Mem& memory ) {
        PC = 0x0000;
        SP = 0x0100;
        C = Z = I = D = B = V = N = 0;
        A = X = Y = 0;
        memory.Init();
    }

    Byte add( Byte R1, Byte R2 ) {
        Word Sum = static_cast< Word >( R1 ) + static_cast< Word >( R2 );
        C = static_cast< Byte >( Sum >> 8 );
        Byte Result = static_cast< Byte >( Sum );
        return Result;
    }

    Byte FetchByte( u32& Cycles, Mem& memory ) {
        Byte Data = memory[PC];
        PC++;
        Cycles--;
        return Data;
    }

    Word FetchWord( u32& Cycles, Mem& memory ) {
        Word Data = memory[PC];
        PC++;

        Data |= ( memory[PC] << 8 );
        PC++;
        
        Cycles -= 2;
        return Data;
    }

    // Opcodes
    static constexpr Byte
        REG_A = 0x00,
        REG_X = 0x01,
        REG_Y = 0x02;

    Byte &FetchRegister( Byte RegID ) {
        switch ( RegID ) {
            case REG_A: { return A; } break;
            case REG_X: { return X; } break;
            case REG_Y: { return Y; } break;
            default: { cout << "WRONG REGISTER ID :" << RegID << endl; } break;
        }
    }

    // Opcodes
    static constexpr Byte
        INS_ADD = 0x00,
        INS_SUB = 0x01,
        INS_AND = 0x02,
        INS_OR = 0x03,
        INS_XOR = 0x04,
        INS_NOT = 0x05,
        INS_MOV = 0x06,
        INS_LOAD = 0x07,
        INS_STORE = 0x08,
        INS_INC = 0x09,
        INS_DEC = 0x0A,
        INS_JMP = 0x0B,
        INS_JZ = 0x0C,
        INS_JNZ = 0x0D,
        INS_CMP = 0x0E,
        INS_NOP = 0x0F;

    void Execute( u32 Cycles, Mem& memory ) {
        while ( Cycles > 0 ) {
            Byte Ins = FetchByte( Cycles, memory );
            switch ( Ins ) {
                case INS_ADD: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    Byte R2 = FetchRegister( FetchByte( Cycles, memory ) );
                    R1 = add( R1, R2 );
                } break;
                case INS_SUB: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    Byte R2 = FetchRegister( FetchByte( Cycles, memory ) );
                    R1 = add( R1, ~R2 );
                } break;
                case INS_AND: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    Byte R2 = FetchRegister( FetchByte( Cycles, memory ) );
                    R1 =  R1 & R2;
                } break;
                case INS_OR: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    Byte R2 = FetchRegister( FetchByte( Cycles, memory ) );
                    R1 = R1 | R2;
                } break;
                case INS_XOR: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    Byte R2 = FetchRegister( FetchByte( Cycles, memory ) );
                    R1 = ~(R1 | R2);
                } break;
                case INS_NOT: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    R1 = ~R1;
                } break;
                case INS_MOV: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    Byte R2 = FetchRegister( FetchByte( Cycles, memory ) );
                    R1 = R2;
                } break;
                case INS_LOAD: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    Byte R2 = FetchByte( Cycles, memory );
                    R1 = memory[R2];
                } break;
                case INS_STORE: {
                    Byte R1 = FetchByte( Cycles, memory );
                    Byte R2 = FetchRegister( FetchByte( Cycles, memory ) );
                    memory[R1] = R2;
                } break;
                case INS_INC: {
                    Byte R1 = FetchByte( Cycles, memory );
                    R1 = add( R1, Byte( 1 ) );
                } break;
                case INS_DEC: {
                    Byte R1 = FetchByte( Cycles, memory );
                    R1 = add( R1, ~( Byte( 1 ) ) );
                } break;
                case INS_JMP: {
                    Byte ADDR = FetchByte( Cycles, memory );
                    PC = ADDR;
                } break;
                case INS_JZ: {
                    if (Z) {
                        Byte ADDR = FetchByte( Cycles, memory );
                        PC = ADDR;
                    }
                } break;
                case INS_JNZ: {
                    if (!Z) {
                        Byte ADDR = FetchByte( Cycles, memory );
                        PC = ADDR;
                    }
                } break;
                case INS_CMP: {
                    Byte R1 = FetchRegister( FetchByte( Cycles, memory ) );
                    Byte R2 = FetchRegister( FetchByte( Cycles, memory ) );
                    if ( R1 == R2 ) {
                        Z = 1;
                    } else {
                        Z = 0;
                    }
                } break;
                case INS_NOP: { PC++; } break;
                default: {} break;
            }
        }
    }
};

int main() {
    Mem mem;
    CPU cpu;
    cpu.Reset( mem );
    cpu.Execute( 2, mem );
    return 0;
}