#include <stdio.h>
#include <stdlib.h>

//#define SYSTEM_BIG_ENDIAN /* UNCOMMENT IF YOUR SYSTEM USES BIG ENDIAN */

// CPU defines
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;

struct Mem
{
	static constexpr u32 MAX_MEM = 1024 * 64;
	u8 data[MAX_MEM];

	void Initialise() 
	{
		for (u32 i = 0; i < MAX_MEM; i++) 
			data[i] = 0;
	}

	// read 1 byte
	u8 operator[](u32 address) const 
	{
		// assert if addr > MAX_MEM
		if (address >= MAX_MEM) {
			printf("Error: Tried to read memory at invalid location: %x", address);
			return data[0]; // return first entry in data, to not crash
		}
		return data[address];
	}

	// write 1 byte
	u8& operator[](u32 address)
	{
		// assert if addr > MAX_MEM
		if (address >= MAX_MEM) {
			printf("Error: Tried to write memory at invalid location: %x", address);
			return data[0]; // return first entry in data, to not crash
		}
		return data[address];
	}

	// write 2 bytes
	void WriteU16(u32& cycles, u16 val, u32 address) {
		data[address] = val & 0xFF;
		cycles--;

		data[address + 1] = (val >> 8);
		cycles--;
	}

	void ReadU16(u32& cycles, u16& val, u32 address) {
		val = (((short)data[address - 1]) << 8) | data[address - 2];
		cycles -= 2;
	}

	void PrintMemory() {
		printf("Printing CPU Memory\n");
		for (int i = 0; i < MAX_MEM; i++) {
			printf("[%x] ", data[i]);
			if (i % 16 == 0 && i != 0)
				printf("\n");
		}
	}
};

struct CPU
{
	// opcodes
	static constexpr u8
		INS_LDA_IM = 0xA9, // 2 cycles
		INS_LDA_ZP = 0xA5, // 3 cycles
		INS_LDA_ZPX = 0xB5, // 4 cycles
		INS_JSR = 0x20, // 6 cycles
		INS_RTS = 0x60, // 6 cycles
		INS_CMP_IM = 0xC9, // 2 cycles
		INS_CPX_IM = 0xE0, // 2 cycles
		INS_CPY_IM = 0xC0, // 2 cycles
		INS_NOP = 0xEA, // 2 cycles
		INS_CLC = 0x18, // 2 cycles
		INS_CLD = 0xD8, // 2 cycles
		INS_CLI = 0x58, // 2 cycles
		INS_CLV = 0xB8, // 2 cycles
		INS_BCC = 0x90, // 4 cycles
		INS_BCS = 0xB0, // 4 cycles
		INS_BEQ = 0xF0; // 4 cycles

	void LDASetStatus()
	{
		Z = (A == 0);
		N = (A & 0b10000000) > 0;
	}

	u16 PC; // the instruction pointer
	u16 SP; // stack pointer

	u8 A, X, Y; // registers

	// status flags
	u8 C : 1; // carry
	u8 Z : 1; // zero
	u8 I : 1; // interrupt disable
	u8 D : 1; // decimal mode
	u8 B : 1; // break
	u8 V : 1; // overflow
	u8 N : 1; // negative

	void PrintStatus() {
		printf("Printing CPU info...\n");
		printf("------------ Registers ------------\n");
		printf("        PC:		0x%x\n", PC);
		printf("        SP:		0x%x\n", SP);
		printf("        A:		0x%x\n", A);
		printf("        X:		0x%x\n", X);
		printf("        Y:		0x%x\n", Y);
		printf("\n\n\n");

		printf("------------   Flags   ------------\n");
		printf("        C:		0x%x\n", C);
		printf("        Z:		0x%x\n", Z);
		printf("        I:		0x%x\n", I);
		printf("        D:		0x%x\n", D);
		printf("        B:		0x%x\n", B);
		printf("        V:		0x%x\n", V);
		printf("        N:		0x%x\n", N);
		printf("\n\n\n");
	}

	// not 100% accurate
	void Reset(Mem& memory)
	{
		printf("Booting CPU...\n");
		PC = 0xFFFC;
		SP = 0x0100;
		C = Z = I = D = B = V = N = 0;
		A = X = Y = 0;
		memory.Initialise();
	}

	// CPU is executing code
	u8 FetchU8(u32& cycles, Mem& memory)
	{
		u8 data = memory[PC];
		PC++;
		cycles--;
		return data;
	}

	u16 FetchU16(u32& cycles, Mem& memory)
	{
		// 6502 is little endian
		u16 data = memory[PC];
		PC++;
		cycles--;

		data |= (memory[PC] << 8);
		PC++;
		cycles--;

#ifdef SYSTEM_BIG_ENDIAN
		// swap bytes to handle big endian systems
		u16 tmp = ((data & 0xff) << 8) | ((data & 0xff00) >> 8);
		data = tmp;
#endif // SYSTEM_BIG_ENDIAN


		return data;
	}

	// CPU is reading from memory
	u8 ReadU8(u32& cycles, u8 address, Mem& memory)
	{
		u8 data = memory[address];
		cycles--;
		return data;
	}

	u32 Execute(u32 cycles, Mem& memory)
	{
		u32 begcycles = cycles;
		while (cycles > 0)
		{
			u8 instruction = FetchU8(cycles, memory);
			if (instruction)
				printf("Executing instruction: %x\n", instruction);

			switch (instruction) {
			case INS_LDA_IM: 
			{
				u8 val = FetchU8(cycles, memory);
				A = val;
				LDASetStatus();
			}
				break;

			case INS_LDA_ZP:
			{
				u8 zeropageaddress = FetchU8(cycles, memory);
				A = ReadU8(cycles, zeropageaddress, memory);
				LDASetStatus();
			}
				break;

			case INS_LDA_ZPX:
			{
				u8 zeropageaddress = FetchU8(cycles, memory);
				zeropageaddress += X;
				cycles--;
				A = ReadU8(cycles, zeropageaddress, memory);
				LDASetStatus();
			}
				break;

			case INS_JSR:
			{
				u16 subaddr = FetchU16(cycles, memory);
				memory.WriteU16(cycles, PC - 1, SP);
				SP+=2;
				PC = subaddr;
				cycles--;
			}
				break;

			case INS_CMP_IM:
			{
				u8 val = FetchU8(cycles, memory);
				u8 res = A - val;
				Z = res == 0;
				C = val <= A;
				N = (res & 0b10000000) > 0;
			}
				break;

			case INS_CPX_IM:
			{
				u8 val = FetchU8(cycles, memory);
				u8 res = X - val;
				Z = res == 0;
				C = val >= X;
				N = (res & 0b10000000) > 0;
			}
				break;

			case INS_CPY_IM:
			{
				u8 val = FetchU8(cycles, memory);
				u8 res = X - val;
				Z = res == 0;
				C = val >= X;
				N = (res & 0b10000000) > 0;
			}
			break;

			case INS_RTS:
			{
				u16 top;
				memory.ReadU16(cycles, top, SP);
				SP-=2;
				PC = top;
				cycles -= 2; // writing 2 bytes here
				PC++;
				cycles--;
			}
				break;

			case INS_NOP:
			{
				cycles--;
			}
				break;

			case INS_CLC:
			{
				C = 0;
				cycles--;
			}
				break;

			case INS_CLD:
			{
				D = 0;
				cycles--;
			}
				break;

			case INS_CLI:
			{
				I = 0;
				cycles--;
			}
			break;

			case INS_CLV:
			{
				V = 0;
				cycles--;
			}
			break;

			case INS_BCC:
			{
				if (C == 0)
				{
					u16 subaddr = FetchU16(cycles, memory);
					PC = subaddr;
					cycles--;
				}
			}
				break;

			case INS_BCS:
			{
				if (C == 1)
				{
					u16 subaddr = FetchU16(cycles, memory);
					PC = subaddr;
					cycles--;
				}
			}
				break;

			case INS_BEQ:
			{
				if (Z == 1)
				{
					u16 subaddr = FetchU16(cycles, memory);
					PC = subaddr;
					cycles--;
				}
			}
				break;
				// unknown opcode, do nothing
			default:
			{
				printf("Error: Unknown instruction: %x aborting...\n", instruction);
				return begcycles - cycles;
			}
				break;
			}
		}
		return begcycles - cycles;
	}
};

int main()
{
	Mem mem;
	CPU cpu;
	cpu.Reset( mem );

	// inline program
	mem[0xFFFC] = CPU::INS_JSR;
	mem[0xFFFD] = 0x42;
	mem[0xFFFE] = 0x00;

	mem[0x0042] = CPU::INS_LDA_IM;
	mem[0x0043] = 0x69;

	mem[0x0044] = CPU::INS_NOP;
	
	mem[0x0045] = CPU::INS_CMP_IM;
	mem[0x0046] = 0x70;

	mem[0x0047] = CPU::INS_RTS;
	// end inline program

	u32 reqcycles = cpu.Execute( 1, mem );
	cpu.PrintStatus();
	printf("Operation took: %d cycles\n", reqcycles);

	//mem.PrintMemory();
	return 0;
}