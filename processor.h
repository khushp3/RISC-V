#ifndef PROCESSOR_H
#define PROCESSOR_H

/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class for processor

**************************************************************** */

#include "memory.h"
#include <set>

using namespace std;

class processor
{

private:
   // TODO: Add private members here
   bool isVerbose;
   memory *Main_Memory;
   bool isStage2;
   uint64_t pc;
   uint64_t reg_num;
   uint64_t curr_inst;
   uint64_t prv;
   uint64_t breakpoint;
   uint64_t registers[32];
   int64_t instruction_count;
   unordered_map<uint16_t, uint64_t> csr_register;
   
public:
   // Consructor
   processor(memory *main_memory, bool verbose, bool stage2);

   // Display PC value
   void show_pc();

   bool check_instruction(uint32_t instruction);

   bool csr_check(uint16_t csr_num);

   // Set PC to new value
   void set_pc(uint64_t new_pc);

   // Display register value
   void show_reg(unsigned int reg_num);

   // Set register to new value
   void set_reg(unsigned int reg_num, uint64_t new_value);

   // Execute a number of instructions
   void execute(unsigned int num, bool breakpoint_check);

   // Clear breakpoint
   void clear_breakpoint();

   // Set breakpoint at an address
   void set_breakpoint(uint64_t address);

   void execute_instruction(uint32_t instruction);

   void interrupt(uint32_t cause);

   void exception_handling(uint32_t cause, uint64_t instruction);

   // Show privilege level
   // Empty implementation for stage 1, required for stage 2
   void show_prv();

   // Set privilege level
   // Empty implementation for stage 1, required for stage 2
   void set_prv(unsigned int prv_num);

   // Display CSR value
   // Empty implementation for stage 1, required for stage 2
   void show_csr(unsigned int csr_num);

   // Set CSR to new value
   // Empty implementation for stage 1, required for stage 2
   void set_csr(unsigned int csr_num, uint64_t new_value);

   uint64_t get_instruction_count();

   // Used for Postgraduate assignment. Undergraduate assignment can return 0.
   uint64_t get_cycle_count();
};

#endif
