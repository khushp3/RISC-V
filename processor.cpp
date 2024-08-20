/* ****************************************************************
   RISC-V Instruction Set Simulator
   Computer Architecture, Semester 1, 2024

   Class members for processor

**************************************************************** */

#include <iostream>
#include <iomanip>
#include <stdlib.h>

#include "memory.h"
#include "processor.h"

using namespace std;

processor::processor(memory *main_memory, bool verbose, bool stage2)
{
   Main_Memory = main_memory;
   isVerbose = verbose;
   isStage2 = stage2;
   breakpoint = 0xffffffffffffffff;
   prv = 3;
   pc = 0;
   instruction_count = 0;
   for (int i = 0; i < 32; i++)
   {
      registers[i] = 0;
   }
   csr_register[0xF11] = 0x0000000000000000; 
   csr_register[0xF12] = 0x0000000000000000; 
   csr_register[0xF13] = 0x2024020000000000; 
   csr_register[0xF14] = 0x0000000000000000; 
   csr_register[0x300] = 0x0000000200000000; 
   csr_register[0x301] = 0x8000000000100100; 
   csr_register[0x304] = 0x0000000000000000; 
   csr_register[0x305] = 0x0000000000000000; 
   csr_register[0x340] = 0x0000000000000000; 
   csr_register[0x341] = 0x0000000000000000; 
   csr_register[0x342] = 0x0000000000000000; 
   csr_register[0x343] = 0x0000000000000000; 
   csr_register[0x344] = 0x0000000000000000;
}

bool processor::csr_check(uint16_t csr_num)
{
   return csr_register.find(csr_num) != csr_register.end();
}

void processor::show_pc()
{
   cout << setw(16) << setfill('0') << hex << pc << endl;
}

void processor::set_pc(uint64_t new_pc)
{
   pc = new_pc;
}

void processor::show_reg(unsigned int reg_num)
{
   cout << setw(16) << setfill('0') << hex << registers[reg_num] << endl;
}

void processor::set_reg(unsigned int reg_num, uint64_t new_value)
{
   if (reg_num != 0)
   {
      registers[reg_num] = new_value;
   }
}

void processor::clear_breakpoint()
{
   breakpoint = 0xffffffffffffffff;
}

void processor::set_breakpoint(uint64_t address)
{
   breakpoint = address;
}

void processor::show_prv()
{
   switch (prv)
   {
   case 0:
      cout << "0 (user)" << endl;
      break;
   case 3:
      cout << "3 (machine)" << endl;
      break;
   }
}

void processor::set_prv(unsigned int prv_num)
{
   if (prv_num == 0 || prv_num == 3)
   {
      prv = prv_num;
   }
}

void processor::show_csr(unsigned int csr_num)
{
   if (csr_check(csr_num))
   {
      cout << setw(16) << setfill('0') << hex << csr_register[csr_num] << endl;
   }
   else
   {
      cout << "Illegal CSR number" << endl;
   }
}

void processor::set_csr(unsigned int csr_num, uint64_t new_value)
{
   if (csr_check(csr_num))
   {
      if (csr_num == 0xF11 || csr_num == 0xF12 || csr_num == 0xF13 || csr_num == 0xF14)
      {
         cout << "Illegal write to read-only CSR" << endl;
         return;
      }
      switch (csr_num)
      {
      case 0x300:
         new_value &= 0x0000000000001888;
         new_value |= 0x0000000200000000;
         break;
      case 0x301:
         new_value = 0x8000000000100100;
         break;
      case 0x304:
         new_value &= 0x0000000000000999;
         break;
      case 0x305:
         if (new_value & 0x1)
         {
            new_value &= 0xffffffffffffff01;
         }
         else
         {
            new_value &= 0xfffffffffffffffc;
         }
         break;
      case 0x340:
         break;
      case 0x341:
         new_value &= 0xfffffffffffffffc;
         break;
      case 0x342:
         new_value &= 0x800000000000000f;
         break;
      case 0x343:
         break;
      case 0x344:
         new_value &= 0x0000000000000999;
         break;
      }
      csr_register[csr_num] = new_value;
   }
   else
   {
      cout << "Illegal CSR number" << endl;
   }
   return;
}

void processor::exception_handling(uint32_t cause, uint64_t instruction)
{
   set_csr(0x341, pc);
   set_csr(0x342, cause);
   uint64_t PC = pc;
   set_pc((csr_register[0x305] & 0xfffffffffffffffc) - 4);

   uint64_t mie = (csr_register[0x300] >> 3) & 1;
   set_csr(0x300, csr_register[0x300] & 0xFFFFFFFFFFFFE777);
   set_csr(0x300, csr_register[0x300] | (prv << 11 | mie << 7));
   instruction_count--;

   switch (cause)
   {
   case 0:
   {
      set_csr(0x343, PC);
      instruction_count++;
      pc += 4;
   }
   break;
   case 2:
   {
      set_csr(0x343, instruction);
   }
   break;
   case 4:
   {
      uint64_t rs1 = (instruction & 0x000F8000) >> 15;
      int64_t imm = ((int64_t)(int32_t)instruction) >> 20;
      set_csr(0x343, registers[rs1] + imm);
   }
   break;
   case 6:
   {

      uint64_t rs1 = (instruction & 0x000F8000) >> 15;
      uint64_t imm2 = (instruction & 0x00000F80) >> 7;
      uint64_t imm = ((int64_t)(int32_t)instruction) >> 25;
      imm = (imm << 5) + imm2;
      set_csr(0x343, registers[rs1] + imm);
   }
   break;
   case 8:
   case 11:
   {
      prv = 3;
      set_csr(0x343, 0x0000000000000000);
   }
   break;
   }
}

void processor::interrupt(uint32_t cause)
{
   set_csr(0x300, csr_register[0x300] | 0x0000000000000080);
   set_csr(0x341, pc);
   set_csr(0x342, 0x8000000000000000 + cause);
   if (csr_register[0x305] & 0x0000000000000001)
   {
      set_pc((csr_register[0x305] & 0xfffffffffffffffc) + (4 * cause));
   }
   else
   {
      set_pc(csr_register[0x305] & 0xfffffffffffffffc);
   }
   uint64_t mstatus = csr_register[0x300];
   if (prv == 0)
   {
      mstatus &= 0xffffffffffffe7ff;
      if (!(mstatus & 0x0000000000000008))
      {
         mstatus &= 0xffffffffffffff7f;
      }
      prv = 3;
   }
   else if (prv == 3)
   {
      mstatus |= 0x0000000000001800;
   }
   set_csr(0x300, mstatus & 0xfffffffffffffff7);
}

void processor::execute(unsigned int num, bool breakpoint_check)
{
   for (unsigned int i = 0; i < num; i++)
   {
      if (breakpoint_check && (pc == breakpoint))
      {
         cout << "Breakpoint reached at ";
         cout << setw(16) << setfill('0') << hex << breakpoint << endl;
         break;
      }
      if ((csr_register[0x300] & 0x8) || (prv == 0))
      {
         if ((csr_register[0x304] & 0x800) && (csr_register[0x344] & 0x800))
         {
            interrupt(11);
         }
         else if ((csr_register[0x304] & 0x8) && (csr_register[0x344] & 0x8))
         {
            interrupt(3);
         }
         else if ((csr_register[0x304] & 0x80) && (csr_register[0x344] & 0x80))
         {
            interrupt(7);
         }
         else if ((csr_register[0x304] & 0x100) && (csr_register[0x344] & 0x100))
         {
            interrupt(8);
         }
         else if ((csr_register[0x304] & 0x1) && (csr_register[0x344] & 0x1))
         {
            interrupt(0);
         }
         else if ((csr_register[0x304] & 0x10) && (csr_register[0x344] & 0x10))
         {
            interrupt(4);
         }
      }
      if (pc % 4 != 0)
      {
         exception_handling(0, 0);
         continue;
      }
      uint32_t instruction;
      uint64_t buffer = Main_Memory->read_doubleword(pc);
      curr_inst = buffer;
      if (pc % 8 == 4)
      {
         buffer = buffer >> 32;
      }
      instruction = buffer;
      execute_instruction(instruction);
      instruction_count++;
      pc += 4;
   }
}

void processor::execute_instruction(uint32_t instruction)
{
   uint32_t opcode = instruction & 0x0000007F;
   uint32_t funct3 = (instruction & 0x00007000) >> 12;
   uint16_t funct7;
   uint8_t rd;
   uint8_t rs1;
   uint8_t rs2;
   int64_t imm;

   switch (opcode)
   {
   case 0b1100111:
   {
      switch (funct3)
      {
      case 0b000:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         uint64_t targetAddress = imm + registers[rs1];
         targetAddress &= ~1;
         set_reg(rd, pc + 4);
         set_pc(targetAddress - 4);
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   }
   case 0b0110111:
   {
      rd = (instruction & 0x00000F80) >> 7;
      registers[rd] = (int64_t)(int32_t)(instruction & 0xFFFFF000);
   }
   break;
   case 0b0010111:
   {
      rd = (instruction & 0x00000F80) >> 7;
      set_reg(rd, (int64_t)(pc) + (int64_t)((int32_t)(instruction & 0xFFFFF000)));
   }
   break;
   case 0b0010011:
      switch (funct3)
      {
      case 0b000:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         set_reg(rd, registers[rs1] + imm);
      }
      break;
      case 0b111:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         set_reg(rd, registers[rs1] & imm);
      }
      break;
      case 0b001:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         uint64_t shamt = (instruction >> 20) & 0x03F;
         set_reg(rd, registers[rs1] << shamt);
      }
      break;
      case 0b110:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         set_reg(rd, registers[rs1] | imm);
      }
      break;
      case 0b010:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         if ((int64_t)registers[rs1] < imm)
         {
            set_reg(rd, 1);
         }
         else
         {
            set_reg(rd, 0);
         }
      }
      break;
      case 0b011:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         if (registers[rs1] < (uint64_t)imm)
         {
            set_reg(rd, 1);
         }
         else
         {
            set_reg(rd, 0);
         }
      }
      break;
      case 0b100:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         set_reg(rd, registers[rs1] ^ imm);
      }
      break;
      case 0b101:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         funct7 = (instruction & 0xFC000000) >> 25;
         int64_t shamt = ((instruction >> 20) & 0x3F);
         switch (funct7)
         {
         case 0b0000000:
         {
            set_reg(rd, registers[rs1] >> shamt);
            break;
         case 0b0100000:
            set_reg(rd, (int64_t)registers[rs1] >> shamt);
            break;
         default:
            exception_handling(2, instruction);
            break;
         }
         break;
         }
         break;
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   case 0b0110011:
      switch (funct3)
      {
      case 0b000:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         funct7 = (instruction & 0xFE000000) >> 25;
         switch (funct7)
         {
         case 0b0000000:
            set_reg(rd, registers[rs1] + registers[rs2]);
            break;
         case 0b0100000:
            set_reg(rd, registers[rs1] - registers[rs2]);
            break;
         default:
            exception_handling(2, instruction);
            break;
         }
      }
      break;
      case 0b111:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         set_reg(rd, registers[rs1] & registers[rs2]);
      }
      break;
      case 0b110:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         set_reg(rd, registers[rs1] | registers[rs2]);
      }
      break;
      case 0b100:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         set_reg(rd, registers[rs1] ^ registers[rs2]);
      }
      break;
      case 0b001:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         set_reg(rd, registers[rs1] << (registers[rs2] & 0x3F));
      }
      break;
      case 0b010:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         if ((int64_t)registers[rs1] < (int64_t)registers[rs2])
         {
            set_reg(rd, 1);
         }
         else
         {
            set_reg(rd, 0);
         }
      }
      break;
      case 0b011:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         if (registers[rs1] < registers[rs2])
         {
            set_reg(rd, 1);
         }
         else
         {
            set_reg(rd, 0);
         }
      }
      break;
      case 0b101:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         funct7 = (instruction & 0xFE000000) >> 25;
         switch (funct7)
         {
         case 0b0000000:
            set_reg(rd, registers[rs1] >> (registers[rs2] & 0x3F));
            break;
         case 0b0100000:
            set_reg(rd, (int64_t)registers[rs1] >> (registers[rs2] & 0x3F));
            break;
         default:
            exception_handling(2, instruction);
            break;
         }
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   case 0b1100011:
      switch (funct3)
      {
      case 0b001:
      {
         rs1 = (instruction >> 15) & 0x1F;
         rs2 = (instruction >> 20) & 0x1F;
         if (registers[rs1] != registers[rs2])
         {
            int32_t imm = ((instruction >> 31) & 0x1) << 12 | ((instruction >> 7) & 0x1) << 11 | ((instruction >> 25) & 0x3F) << 5 | ((instruction >> 8) & 0xF) << 1;
            if (imm & 0x800)
            {
               imm |= 0xFFFFFFFFFFFFF000;
            }
            set_pc(pc + imm - 4);
         }
      }
      break;
      case 0b000:
      {
         rs1 = (instruction >> 15) & 0x1F;
         rs2 = (instruction >> 20) & 0x1F;
         if (registers[rs1] == registers[rs2])
         {
            int32_t imm = ((instruction >> 31) & 0x1) << 12 | ((instruction >> 7) & 0x1) << 11 | ((instruction >> 25) & 0x3F) << 5 | ((instruction >> 8) & 0xF) << 1;
            if (imm & 0x800)
            {
               imm |= 0xFFFFFFFFFFFFF000;
            }
            set_pc(pc + imm - 4);
         }
      }
      case 0b100:
      {

         rs1 = (instruction >> 15) & 0x1F;
         rs2 = (instruction >> 20) & 0x1F;
         if ((int64_t)registers[rs1] < (int64_t)registers[rs2])
         {
            int32_t imm = ((instruction >> 31) & 0x1) << 12 | ((instruction >> 7) & 0x1) << 11 | ((instruction >> 25) & 0x3F) << 5 | ((instruction >> 8) & 0xF) << 1;
            if (imm & 0x800)
            {
               imm |= 0xFFFFFFFFFFFFF000;
            }
            set_pc(pc + imm - 4);
         }
      }
      break;
      case 0b101:
      {
         rs1 = (instruction >> 15) & 0x1F;
         rs2 = (instruction >> 20) & 0x1F;
         if ((int64_t)registers[rs1] >= (int64_t)registers[rs2])
         {
            int32_t imm = ((instruction >> 31) & 0x1) << 12 | ((instruction >> 7) & 0x1) << 11 | ((instruction >> 25) & 0x3F) << 5 | ((instruction >> 8) & 0xF) << 1;
            if (imm & 0x800)
            {
               imm |= 0xFFFFFFFFFFFFF000;
            }
            set_pc(pc + imm - 4);
         }
      }
      break;
      case 0b110:
      {
         rs1 = (instruction >> 15) & 0x1F;
         rs2 = (instruction >> 20) & 0x1F;
         if (registers[rs1] < registers[rs2])
         {
            int32_t imm = ((instruction >> 31) & 0x1) << 12 | ((instruction >> 7) & 0x1) << 11 | ((instruction >> 25) & 0x3F) << 5 | ((instruction >> 8) & 0xF) << 1;
            if (imm & 0x800)
            {
               imm |= 0xFFFFFFFFFFFFF000;
            }
            set_pc(pc + imm - 4);
         }
      }
      break;
      case 0b111:
      {
         rs1 = (instruction >> 15) & 0x1F;
         rs2 = (instruction >> 20) & 0x1F;
         if (registers[rs1] >= registers[rs2])
         {
            int32_t imm = ((instruction >> 31) & 0x1) << 12 | ((instruction >> 7) & 0x1) << 11 | ((instruction >> 25) & 0x3F) << 5 | ((instruction >> 8) & 0xF) << 1;
            if (imm & 0x800)
            {
               imm |= 0xFFFFFFFFFFFFF000;
            }
            set_pc(pc + imm - 4);
         }
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   case 0b0111011:
      switch (funct3)
      {
      case 0b000:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         funct7 = (instruction & 0xFE000000) >> 25;
         switch (funct7)
         {
         case 0b0000000:
            set_reg(rd, (int32_t)registers[rs1] + (int32_t)registers[rs2]);
            break;
         case 0b0100000:
            set_reg(rd, (int32_t)registers[rs1] - (int32_t)registers[rs2]);
            break;
         default:
            exception_handling(2, instruction);
            break;
         }
      }
      break;
      case 0b001:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t result = registers[rs1] << (registers[rs2] & 0x1F);
         result &= 0xFFFFFFFF;
         if (result & 0x80000000)
         {
            result |= 0xFFFFFFFF00000000;
         }
         set_reg(rd, result);
      }
      break;
      case 0b101:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         funct7 = (instruction & 0xFE000000) >> 25;
         uint64_t result;
         switch (funct7)
         {
         case 0b0000000:
            result = (registers[rs1] & 0x00000000ffffffff) >> (registers[rs2] & 0x1F);
            result &= 0xFFFFFFFF;
            if (result & 0x80000000)
            {
               result |= 0xFFFFFFFF00000000;
            }
            set_reg(rd, result);
            break;
         case 0b0100000:
            set_reg(rd, (int32_t)registers[rs1] >> (registers[rs2] & 0x1F));
            break;
         default:
            exception_handling(2, instruction);
            break;
         }
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   case 0b0011011:
      switch (funct3)
      {
      case 0b000:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         int64_t result = (int64_t)((int32_t)registers[rs1] + (int32_t)imm);
         set_reg(rd, result);
      }
      break;
      case 0b001:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         uint64_t shamt = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t result = registers[rs1] << shamt;
         result &= 0xFFFFFFFF;
         if (result & 0x80000000)
         {
            result |= 0xFFFFFFFF00000000;
         }
         set_reg(rd, result);
      }
      break;
      case 0b101:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         uint64_t shamt = (instruction & 0x01F00000) >> 20;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t result;
         funct7 = (instruction & 0xFE000000) >> 25;
         switch (funct7)
         {
         case 0b0000000:
            result = (registers[rs1] & 0x00000000ffffffff) >> shamt;
            if (result & 0x80000000)
            {
               result |= 0xFFFFFFFF00000000;
            }
            set_reg(rd, result);
            break;
         case 0b0100000:
            set_reg(rd, (int32_t)registers[rs1] >> shamt);
            break;
         default:
            exception_handling(2, instruction);
            break;
         }
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   case 0b1101111:
   {
      uint64_t imm2 = (((instruction >> 12) & 0xFF) << 12);
      uint64_t imm3 = (((instruction >> 20) & 0x1) << 11);
      uint64_t imm4 = (((instruction >> 21) & 0x3FF) << 1);
      imm = ((int64_t)(int32_t)instruction) >> 31;
      imm = imm << 20;
      imm = imm + imm2 + imm3 + imm4;
      rd = (instruction & 0x00000F80) >> 7;
      set_reg(rd, pc + 4);
      set_pc(pc + (imm - 4));
   }
   break;
   case 0b0000011:
      switch (funct3)
      {
      case 0b011:
      {
         rd = (instruction & 0x00000F80) >> 7;
         rs1 = (instruction & 0x000F8000) >> 15;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         int64_t targetAddress = registers[rs1] + imm;
         if (targetAddress % 8 != 0)
         {
            exception_handling(4, instruction);
            break;
         }
         uint64_t loadDoubleword = Main_Memory->read_doubleword(targetAddress);
         set_reg(rd, loadDoubleword);
      }
      break;
      case 0b001:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         int64_t targetAddress = registers[rs1] + imm;
         if (targetAddress % 2 != 0)
         {
            exception_handling(4, instruction);
            break;
         }
         uint64_t loadDoubleword = Main_Memory->read_doubleword(targetAddress & ~0x7);
         int64_t offset = targetAddress - (targetAddress & ~0x7);
         int16_t loadHalfword = (int16_t)((loadDoubleword >> (offset * 8)) & 0xFFFF);
         set_reg(rd, loadHalfword);
      }
      break;
      case 0b101:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         int64_t targetAddress = registers[rs1] + imm;
         if (targetAddress % 2 != 0)
         {
            exception_handling(4, instruction);
            break;
         }
         int64_t align = targetAddress & ~0x7;
         uint64_t loadDoubleword = Main_Memory->read_doubleword(align);
         int64_t offset = targetAddress - align;
         uint16_t loadHalfword = (loadDoubleword >> (offset * 8)) & 0xFFFF;
         set_reg(rd, loadHalfword);
      }
      break;
      case 0b010:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         int64_t targetAddress = registers[rs1] + imm;
         if (targetAddress % 4 != 0)
         {
            exception_handling(4, instruction);
            break;
         }
         uint64_t loadDoubleword = Main_Memory->read_doubleword(targetAddress & ~0x7);
         int64_t offset = targetAddress - (targetAddress & ~0x7);
         int32_t loadWord = (int32_t)((loadDoubleword >> (offset * 8)) & 0xFFFFFFFF);
         set_reg(rd, loadWord);
      }
      break;
      case 0b110:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         int64_t targetAddress = registers[rs1] + imm;
         if (targetAddress % 4 != 0)
         {
            exception_handling(4, instruction);
            break;
         }
         int64_t align = targetAddress & ~0x7;
         uint64_t loadDoubleword = Main_Memory->read_doubleword(align);
         int64_t offset = targetAddress - align;
         uint32_t loadWord = (uint32_t)((loadDoubleword >> (offset * 8)) & 0xFFFFFFFF);
         set_reg(rd, loadWord);
      }
      break;
      case 0b000:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         int64_t targetAddress = registers[rs1] + imm;
         int64_t align = targetAddress & ~0x7;
         uint64_t loadDoubleword = Main_Memory->read_doubleword(align);
         int64_t offset = targetAddress - align;
         int8_t loadByte = (int8_t)((loadDoubleword >> (offset * 8)) & 0xFF);
         set_reg(rd, loadByte);
      }
      break;
      case 0b100:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 20;
         int64_t targetAddress = registers[rs1] + imm;
         int64_t align = targetAddress & ~0x7;
         uint64_t loadDoubleword = Main_Memory->read_doubleword(align);
         int64_t offset = targetAddress - align;
         uint8_t loadWord = (uint8_t)((loadDoubleword >> (offset * 8)) & 0xFF);
         set_reg(rd, loadWord);
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   case 0b0100011:
      switch (funct3)
      {
      case 0b011:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         uint32_t imm2 = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 25;
         imm = (imm << 5) + imm2;
         uint64_t targetAddress = registers[rs1] + imm;
         if (targetAddress % 8 != 0)
         {
            exception_handling(6, instruction);
            break;
         }
         Main_Memory->write_doubleword(targetAddress, registers[rs2], 0xFFFFFFFFFFFFFFFF);
      }
      break;
      case 0b000:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         uint32_t imm2 = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 25;
         imm = (imm << 5) + imm2;
         uint64_t targetAddress = registers[rs1] + imm;
         int64_t offset = targetAddress - (targetAddress & ~0x7);
         Main_Memory->write_doubleword(targetAddress, registers[rs2] << (offset * 8), 0xFFULL << (offset * 8));
      }
      break;
      case 0b001:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         uint32_t imm2 = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 25;
         imm = (imm << 5) + imm2;
         uint64_t targetAddress = registers[rs1] + imm;
         if (targetAddress % 2 != 0)
         {
            exception_handling(6, instruction);
            break;
         }
         int64_t offset = targetAddress - (targetAddress & ~0x7);
         Main_Memory->write_doubleword(targetAddress, registers[rs2] << (offset * 8), 0xFFFFULL << (offset * 8));
      }
      break;
      case 0b010:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rs2 = (instruction & 0x01F00000) >> 20;
         uint32_t imm2 = (instruction & 0x00000F80) >> 7;
         imm = ((int64_t)(int32_t)instruction) >> 25;
         imm = (imm << 5) + imm2;
         uint64_t targetAddress = registers[rs1] + imm;
         if (targetAddress % 4 != 0)
         {
            exception_handling(6, instruction);
            break;
         }
         int64_t align = targetAddress & ~0x7;
         int64_t offset = targetAddress - align;
         Main_Memory->write_doubleword(targetAddress, registers[rs2] << (offset * 8), 0xFFFFFFFFULL << (offset * 8));
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   case 0b1110011:
   {
      switch (funct3)
      {
      case 0b000:
      {
         uint32_t funct12 = (instruction & 0xFFF00000) >> 20;
         switch (funct12)
         {
         case 0b000000000000:
         {
            if (prv == 0)
            {
               exception_handling(8, instruction);
               break;
            }
            if (prv == 3)
            {
               exception_handling(11, instruction);
               break;
            }
         }
         break;
         case 0b000000000001:
         {
            set_csr(0x341, pc);
            set_csr(0x342, 3);
            uint64_t base = csr_register[0x305] & 0xfffffffffffffffc;
            uint64_t inter = (csr_register[0x342] & 0x8000000000000000) >> 63;
            set_pc(base + (4 * inter) - 4);
            uint64_t mstatus = csr_register[0x300];
            uint64_t mie = (mstatus >> 3) & 1;
            mstatus &= 0xffffffffffffe777;
            mstatus |= (prv << 11) | (mie << 7);
            set_csr(0x300, mstatus);
            prv = 3;
            instruction_count--;
         }
         break;
         case 0b001100000010:
         {
            if (prv == 0)
            {
               exception_handling(2, instruction);
               break;
            }
            else
            {
               set_pc(csr_register[0x341] - 4);
               uint64_t mstatus = csr_register[0x300];
               prv = (mstatus >> 11) & 0x3;
               uint64_t temp = (mstatus & 0x80) >> 4;
               set_csr(0x300, (mstatus & 0xffffffffffffe777) | (temp | 0x0000000000000080));
            }
         }
         break;
         default:
            exception_handling(2, instruction);
            break;
         }
      }
      break;
      case 0b001:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t csr = (instruction >> 20) & 0xFFF;
         if (prv == 0 || !csr_check(csr) || (csr == 0xF11 && rs1 != 0) || (csr == 0xF12 && rs1 != 0) || (csr == 0xF13 && rs1 != 0) || (csr == 0xF14 && rs1 != 0))
         {
            exception_handling(2, instruction);
            break;
         }
         set_reg(rd, csr_register[csr]);
         uint64_t temp = registers[rs1];
         if (csr == 0x344)
         {
            temp &= 0x111;
         }
         set_csr(csr, temp);
      }
      break;
      case 0b010:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t csr = (instruction >> 20) & 0xFFF;
         if (prv == 0 || !csr_check(csr) || (csr == 0xF11 && rs1 != 0) || (csr == 0xF12 && rs1 != 0) || (csr == 0xF13 && rs1 != 0) || (csr == 0xF14 && rs1 != 0))
         {
            exception_handling(2, instruction);
            break;
         }
         set_reg(rd, csr_register[csr]);
         uint64_t temp = registers[rs1] | csr_register[csr];
         if (csr != 0xF11 && csr != 0xF12 && csr != 0xF13 && csr != 0xF14)
         {
            if (csr == 0x344)
            {
               temp &= 0x111;
            }
            set_csr(csr, temp);
         }
      }
      break;
      case 0b011:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t csr = (instruction >> 20) & 0xFFF;
         if (prv == 0 || !csr_check(csr) || (csr == 0xF11 && rs1 != 0) || (csr == 0xF12 && rs1 != 0) || (csr == 0xF13 && rs1 != 0) || (csr == 0xF14 && rs1 != 0))
         {
            exception_handling(2, instruction);
            break;
         }
         set_reg(rd, csr_register[csr]);
         uint64_t temp = (~registers[rs1]) & csr_register[csr];
         if (csr != 0xF11 && csr != 0xF12 && csr != 0xF13 && csr != 0xF14)
         {
            if (csr == 0x344)
            {
               temp &= 0x111;
            }
            set_csr(csr, temp);
         }
      }
      break;
      case 0b101:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t csr = (instruction >> 20) & 0xFFF;
         if (prv == 0 || !csr_check(csr) || (csr == 0xF11 && rs1 != 0) || (csr == 0xF12 && rs1 != 0) || (csr == 0xF13 && rs1 != 0) || (csr == 0xF14 && rs1 != 0))
         {
            exception_handling(2, instruction);
            break;
         }
         set_reg(rd, csr_register[csr]);
         if (csr == 0x344)
         {
            rs1 &= 0x111;
         }
         set_csr(csr, rs1);
      }
      break;
      case 0b110:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t csr = (instruction >> 20) & 0xFFF;
         if (prv == 0 || !csr_check(csr) || (csr == 0xF11 && rs1 != 0) || (csr == 0xF12 && rs1 != 0) || (csr == 0xF13 && rs1 != 0) || (csr == 0xF14 && rs1 != 0))
         {
            exception_handling(2, instruction);
            break;
         }
         uint64_t temp = csr_register[csr] | rs1;
         set_reg(rd, csr_register[csr]);
         if (csr != 0xF11 && csr != 0xF12 && csr != 0xF13 && csr != 0xF14)
         {
            if (csr == 0x344)
            {
               temp &= 0x111;
            }
            set_csr(csr, temp);
         }
      }
      break;
      case 0b111:
      {
         rs1 = (instruction & 0x000F8000) >> 15;
         rd = (instruction & 0x00000F80) >> 7;
         uint64_t csr = (instruction >> 20) & 0xFFF;
         if (prv == 0 || !csr_check(csr) || (csr == 0xF11 && rs1 != 0) || (csr == 0xF12 && rs1 != 0) || (csr == 0xF13 && rs1 != 0) || (csr == 0xF14 && rs1 != 0))
         {
            exception_handling(2, instruction);
            break;
         }
         uint64_t temp = csr_register[csr] & ~rs1;
         set_reg(rd, csr_register[csr]);
         if (csr != 0xF11 && csr != 0xF12 && csr != 0xF13 && csr != 0xF14)
         {
            if (csr == 0x344)
            {
               temp &= 0x111;
            }
            set_csr(csr, temp);
         }
      }
      break;
      default:
         exception_handling(2, instruction);
         break;
      }
      break;
   }
   break;
   default:
      exception_handling(2, instruction);
      break;
   }
}

uint64_t processor::get_instruction_count()
{
   return instruction_count;
}

uint64_t processor::get_cycle_count()
{
   return 0;
}
