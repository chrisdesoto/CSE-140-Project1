#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */

unsigned int endianSwap(unsigned int);

void PrintInfo (int changedReg, int changedMem);
unsigned int Fetch (int);
void Decode (unsigned int, DecodedInstr*, RegVals*);
int Execute (DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int);
void PrintInstruction (DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer (FILE* filein, int printingRegisters, int printingMemory,
  int debugging, int interactive) {
	int k;
	unsigned int instr;

	/* Initialize registers and memory */

	for (k=0; k<32; k++) {
		mips.registers[k] = 0;
	}
	
	/* stack pointer - Initialize to highest address of data segment */
	mips.registers[29] = 0x00400000 + (MAXNUMINSTRS+MAXNUMDATA)*4;

	for (k=0; k<MAXNUMINSTRS+MAXNUMDATA; k++) {
		mips.memory[k] = 0;
	}

	k = 0;
	while (fread(&instr, 4, 1, filein)) {
	/*swap to big endian, convert to host byte order. Ignore this.*/
		mips.memory[k] = ntohl(endianSwap(instr));
		k++;
		if (k>MAXNUMINSTRS) {
			fprintf (stderr, "Program too big.\n");
			exit (1);
		}
	}

	mips.printingRegisters = printingRegisters;
	mips.printingMemory = printingMemory;
	mips.interactive = interactive;
	mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
	return (i>>24)|(i>>8&0x0000ff00)|(i<<8&0x00ff0000)|(i<<24);
}

/*
 *  Run the simulation.
 */
void Simulate () {
	char s[40];  /* used for handling interactive input */
	unsigned int instr;
	int changedReg=-1, changedMem=-1, val;
	DecodedInstr d;
	
	/* Initialize the PC to the start of the code section */
	mips.pc = 0x00400000;
	while (1) {
		if (mips.interactive) {
			printf ("> ");
			fgets (s,sizeof(s),stdin);
			if (s[0] == 'q') {
				return;
			}
		}

	/* Fetch instr at mips.pc, returning it in instr */
	instr = Fetch (mips.pc);

	printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

	/* 
	 * Decode instr, putting decoded instr in d
	 * Note that we reuse the d struct for each instruction.
	 */
	Decode (instr, &d, &rVals);

	/*Print decoded instruction*/
	PrintInstruction(&d);

	/* 
	 * Perform computation needed to execute d, returning computed value 
	 * in val 
	 */
	val = Execute(&d, &rVals);

	UpdatePC(&d,val);

	/* 
 	 * Perform memory load or store. Place the
 	 * address of any updated memory in *changedMem, 
 	 * otherwise put -1 in *changedMem. 
 	 * Return any memory value that is read, otherwise return -1.
	 */
	val = Mem(&d, val, &changedMem);

	/* 
 	 * Write back to register. If the instruction modified a register--
 	 * (including jal, which modifies $ra) --
	 * put the index of the modified register in *changedReg,
	 * otherwise put -1 in *changedReg.
	 */
	RegWrite(&d, val, &changedReg);

	PrintInfo (changedReg, changedMem);
	
	}
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo ( int changedReg, int changedMem) {
	int k, addr;
	printf ("New pc = %8.8x\n", mips.pc);
	if (!mips.printingRegisters && changedReg == -1) {
		printf ("No register was updated.\n");
	} else if (!mips.printingRegisters) {
		printf ("Updated r%2.2d to %8.8x\n",
		changedReg, mips.registers[changedReg]);
	} else {
		for (k=0; k<32; k++) {
			printf ("r%2.2d: %8.8x  ", k, mips.registers[k]);
			if ((k+1)%4 == 0) {
				printf ("\n");
			}
		}
	} 
	if (!mips.printingMemory && changedMem == -1) {
		printf ("No memory location was updated.\n");
	} else if (!mips.printingMemory) {
		printf ("Updated memory at address %8.8x to %8.8x\n",
		changedMem, Fetch (changedMem));
	} else {
		printf ("Nonzero memory\n");
		printf ("ADDR	  CONTENTS\n");
		for (addr = 0x00400000+4*MAXNUMINSTRS;
			 addr < 0x00400000+4*(MAXNUMINSTRS+MAXNUMDATA);
			 addr = addr+4) {
			if (Fetch (addr) != 0) {
				printf ("%8.8x  %8.8x\n", addr, Fetch (addr));
			}
		}
	}
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch(int addr) {
    return mips.memory[(addr-0x00400000)/4];
}

/*
 *  Stores the given value in memory at the given address.
 */
void Store(int addr, int val) {
	mips.memory[(addr-0x00400000)/4] = val;
}

int sign_extend_16_bit(unsigned int val) {
	if((val & 0x00008000) > 0)
		return val | 0xffff0000;
	return val;
}

int instr_type_check(int op) {
	switch(op) {
		case 0:		return 0;
		case 4:
		case 5:
		case 9:
		case 12:
		case 13:
		case 15:
		case 35:
		case 43:	return 1;
		case 2:	
		case 3: 	return 2;
		default:	return -1;
	}
}

void decode_r_instr(unsigned int instr, DecodedInstr* d, RegVals* rVals) {
	d->type = R;
	d->regs.r.funct = instr & 0x3f;
	instr = instr >> 6;
	d->regs.r.shamt = instr & 0x1f;
	instr = instr >> 5;
	d->regs.r.rd = instr & 0x1f;
	instr = instr >> 5;
	d->regs.r.rt = instr & 0x1f;
	instr = instr >> 5;
	d->regs.r.rs = instr & 0x1f;
}

void decode_i_instr(unsigned int instr, DecodedInstr* d, RegVals* rVals) {
	d->type = I;	
	d->regs.i.addr_or_immed = sign_extend_16_bit(instr & 0xffff);
	instr = instr >> 16;
	d->regs.i.rt = instr & 0x1f;
	instr = instr >> 5;
	d->regs.i.rs = instr & 0x1f;
}

void decode_j_instr(unsigned int instr, DecodedInstr* d, RegVals* rVals) {
	d->type = J;
	d->regs.j.target = (((instr & 0x3ffffff) << 2) + (mips.pc & 0xf0000000));
}

void fill_reg_vals(int instrType, DecodedInstr* d, RegVals* rVals) {
	if(instrType) {
		rVals->R_rs = mips.registers[d->regs.i.rs];
		rVals->R_rt = mips.registers[d->regs.i.rt];
	} else {
		rVals->R_rs = mips.registers[d->regs.r.rs];
		rVals->R_rt = mips.registers[d->regs.r.rt];
		rVals->R_rd = mips.registers[d->regs.r.rd];
	}
}

/* Decode instr, returning decoded instruction. */
void Decode (unsigned int instr, DecodedInstr* d, RegVals* rVals) {
	if(instr == 0) 
		exit(0);
	/* Your code goes here */
	d->op = instr >> 26;
	switch(instr_type_check(d->op)) {
		case 0:		decode_r_instr(instr, d, rVals);
					fill_reg_vals(d->type, d, rVals);
					break;
		case 1:		decode_i_instr(instr, d, rVals);
					fill_reg_vals(d->type, d, rVals);
					break;
		case 2:		decode_j_instr(instr, d, rVals);
					break;
		default:	exit(0);
	}
}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction ( DecodedInstr* d) {
	/* Your code goes here */
	char *instr = "     ";
	switch(d->op) {
		case 0:		switch(d->regs.r.funct) {
						// SLL
						case 0:		instr = "sll";
									break;
						// SRL
						case 2:		instr = "srl";
									break;
						// JR
						case 8:		instr = "jr";
									break;
						// ADDU
						case 33: 	instr = "addu";
									break;
						// SUBU
						case 35: 	instr = "subu";
									break;
						// AND
						case 36:	instr = "and";
									break;
						// OR
						case 37:	instr = "or";
									break;
						// SLT
						case 42:	instr = "slt";
					}
					break;
		// J
		case 2:		instr = "j";
					break;
		// JAL
		case 3:		instr = "jal";
					break;
		// BEQ
		case 4:		instr = "beq";
					break;
		// BNE
		case 5:		instr = "bne";
					break;
		// ADDIU
		case 9:		instr = "addiu";
					break;
		// ANDI
		case 12:	instr = "andi";
					break;
		// ORI
		case 13:	instr = "ori";
					break;
		// LUI
		case 15:	instr = "lui";
					break;
		// LW
		case 35:	instr = "lw";
					break;
		// SW
		case 43:	instr = "sw";
	}

	switch(d->type) {
		case R:		switch(d->regs.r.funct) {
						case 0:
						case 2: 	printf("%s\t$%d, $%d, %d\n", instr, d->regs.r.rd, d->regs.r.rs, d->regs.r.shamt);
									break;
						case 8:		printf("%s\t$%d\n", instr, d->regs.r.rs);
									break;
						default:	printf("%s\t$%d, $%d, $%d\n", instr, d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
					}
					break;
		case I:		switch(d->op) {
						case 4:
						case 5:		printf("%s\t$%d, $%d, 0x%08x\n", instr, d->regs.i.rs, d->regs.i.rt, (mips.pc + 4 + (d->regs.i.addr_or_immed << 2)));
									break;
						case 9:		printf("%s\t$%d, $%d, %d\n", instr, d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
									break;
						case 12:
						case 13:	printf("%s\t$%d, $%d, 0x%x\n", instr, d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
									break;
						case 15:	printf("%s\t$%d, 0x%x\n", instr, d->regs.i.rt, d->regs.i.addr_or_immed);
									break;
						case 35:	
						case 43:	printf("%s\t$%d, %d($%d)\n", instr, d->regs.i.rt, d->regs.i.addr_or_immed, d->regs.i.rs);
									break;
					}
					break;
		case J:		printf("%s\t0x%08x\n", instr, d->regs.j.target);
					break;
	}
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
	switch(d->op) {
		case 0:		switch(d->regs.r.funct) {
						// SLL
						case 0:		return rVals->R_rt << d->regs.r.shamt;
						// SRL
						case 2:		return rVals->R_rt >> d->regs.r.shamt;
						// JR
						case 8:		return rVals->R_rs;
						// ADDU
						case 33: 	return rVals->R_rs + rVals->R_rt;
						// SUBU
						case 35: 	return rVals->R_rs - rVals->R_rt;
						// AND
						case 36:	return rVals->R_rs & rVals->R_rt;
						// OR
						case 37:	return rVals->R_rs | rVals->R_rt;
						// SLT
						case 42:	return (rVals->R_rs - rVals->R_rt) < 0;
					}
		// J
		case 2:		break;
		// JAL
		case 3:		return mips.pc + 4;
					break;
		// BEQ
		case 4:		return (rVals->R_rs - rVals->R_rt) == 0 ? (d->regs.i.addr_or_immed << 2) : 0;
		// BNE
		case 5:		return (rVals->R_rs - rVals->R_rt) == 0 ? 0 : (d->regs.i.addr_or_immed << 2);
		// ADDIU
		case 9:		return rVals->R_rs + d->regs.i.addr_or_immed;
		// ANDI
		case 12:	return rVals->R_rs & d->regs.i.addr_or_immed;
		// ORI
		case 13:	return rVals->R_rs | d->regs.i.addr_or_immed;
		// LUI
		case 15:	return d->regs.i.addr_or_immed << 16;
		// LW
		case 35:	return rVals->R_rs + d->regs.i.addr_or_immed;
		// SW
		case 43:	return rVals->R_rs + d->regs.i.addr_or_immed;
	}
	return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int val) {
	mips.pc += 4;
	/* Your code goes here */
	switch(d->op) {
		case 0:		if(d->regs.r.funct == 8)
						mips.pc = val;
					break;
		case 2:
		case 3:		mips.pc = d->regs.j.target;
					break;
		case 4:
		case 5:		mips.pc += val;
					break;
	}
}

/**
 *	Checks for word alignment on 32-bit address
 *	using a 2-bit mask. Returns 0 if not word aligned.
 */
int is_word_aligned(int addr) {
	return (addr & 0x3) ? 0 : 1;
}

/**
 *	Checks if the memory address to be accessed is
 *	valid, that is, within range of data memory and 
 *	word aligned. 
 */
void check_valid_mem_addr(int addr) {
	if(addr < 0x00401000 || addr > 0x00403fff || !is_word_aligned(addr)){
		printf("Memory Access Exception at 0x%08x: address 0x%08x\n", mips.pc-4, addr);
		exit(0);
	}
}

/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 *
 */
int Mem( DecodedInstr* d, int val, int *changedMem) {
	/* Your code goes here */
	*changedMem = -1;
	if(d->op == 35) {
		check_valid_mem_addr(val);
		return Fetch(val);
	} else if(d->op == 43) {
		check_valid_mem_addr(val);
		Store(val, mips.registers[d->regs.i.rt]);
		*changedMem = val;
	}
	return val;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
	/* Your code goes here */
	*changedReg = -1;
	switch(d->op) {
		case 0:		if(d->regs.r.funct == 8 || d->regs.r.rd == 0)
						break;
					mips.registers[d->regs.r.rd] = val;
					*changedReg = d->regs.r.rd;
					break;
		case 3:		mips.registers[31] = val;
					*changedReg = 31;
					break;
		case 9:
		case 12:
		case 13:
		case 15:
		case 35:	if(d->regs.i.rt == 0)
						break;
					mips.registers[d->regs.i.rt] = val;
					*changedReg = d->regs.i.rt;
	}
}
