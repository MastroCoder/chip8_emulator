#include <chip8.hpp>

constexpr unsigned short kMemSize = 4096;
constexpr unsigned short kScreenWidth = 64;
constexpr unsigned short kScreenHeight = 32;
constexpr unsigned short kRegisterQty = 16;
constexpr unsigned short kKeypadStatesQty = 16;
constexpr unsigned short kStackSize = 16;

constexpr unsigned char kFontset[80] = { 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8(){
  srand(time(NULL));
  // Resize calls (not only allocates, but initializes with base values)
  memory.resize(kMemSize);
  cpu_registers.resize(kRegisterQty);
  keypad_states.resize(kKeypadStatesQty);
  screen_pixels.resize(kScreenHeight * kScreenWidth);
  stack.resize(kStackSize);

  program_counter = 0x200;
  cur_opcode = 0x0;
  address_register = 0x0;
  stack_pointer = 0;

  for (unsigned char i = 0; i < 80; ++i){
    memory[i] = kFontset[i];
  }
}

void Chip8::ReadOpcode(){
  cur_opcode = memory[program_counter] << 8 | memory[program_counter + 1];
}

// Implements opcode table (see https://en.wikipedia.org/wiki/CHIP-8#Opcode_table)
void Chip8::InterpretOpcode(){
  switch (cur_opcode & 0xF000){
    case 0x1000: // 0x1NNN - Jumps to NNN address. 
      program_counter = cur_opcode & 0x0FFF;
      break;
    case 0x2000: // 0x2NNN - Calls subroutine at NNN
      stack[stack_pointer++] = program_counter;
      program_counter = cur_opcode & 0x0FFF;
      break;
    case 0xB000:
      program_counter = cur_opcode & 0x0FFF + cpu_registers[0];
      break;
    default:
      InterpretIncrementingOpcodes();
      break;
  }
}

// Only for opcodes that invariably need program counter increment
void Chip8::InterpretIncrementingOpcodes(){
  switch (cur_opcode & 0xF000){
    case 0x0000:
      Interpret0BasedOpcodes();
      break;
    case 0x3000: // 0x3XNN - Skips to next instruction if Vx == NN
      if (cpu_registers[(cur_opcode & 0x0F00) >> 8] == cur_opcode & 0x00FF) 
        program_counter += 2;
      break; 
    case 0x4000: // 0x4XNN - Skips to next instruction if Vx != NN
      if (cpu_registers[(cur_opcode & 0x0F00) >> 8] != cur_opcode & 0x00FF) 
        program_counter += 2;
      break;
    case 0x5000: // 0x5XY0 - Skips to next instruction if Vx == Vy
      if (cpu_registers[(cur_opcode & 0x0F00) >> 8] == cpu_registers[(cur_opcode & 0x00F0) >> 4]) 
        program_counter += 2;
      break;
    case 0x6000: // 0x6XNN - Puts NN in Vx
      cpu_registers[(cur_opcode & 0x0F00) >> 8] = cur_opcode & 0x00FF;
      break;
    case 0x7000: // 0x7XNN - Increments Vx with NN
      cpu_registers[(cur_opcode & 0x0F00) >> 8] += cur_opcode & 0x00FF;
      break;
    case 0x8000: // 0x8000 - Separated in another function
      Interpret8BasedOpcodes();
      break;
    case 0x9000:
      if (cpu_registers[(cur_opcode & 0x0F00) >> 8] != cpu_registers[(cur_opcode & 0x00F0) >> 4])
        program_counter += 2;
      break;
    case 0xA000:
      address_register = cur_opcode & 0x0FFF;
      break;
    case 0xC000:
      cpu_registers[(cur_opcode & 0x0F00) >> 8] = (rand() % 256) & (cur_opcode & 0x00FF);
      break;
    case 0xD000:
      DrawSprite();
    case 0xE000:
      InterpretEBasedOpcodes();
    case 0xF000:
      InterpretFBasedOpcodes();
    default:
      cerr << "Unknown opcode call: " << cur_opcode << "\n";
  }
  program_counter += 2;
}

void Chip8::Interpret0BasedOpcodes(){
  switch (cur_opcode & 0x0FFF){
    case 0x00E0: // clear screen
      ClearScreen();
      break;
    case 0x00EE: // return from a subroutine
      program_counter = stack[--stack_pointer]; // decreases stack pointer, puts stack return address in program counter
      break;
  }
}

void Chip8::Interpret8BasedOpcodes(){
  switch (cur_opcode & 0x000F){
    case 0x0000: // assigns Vx to Vy
      cpu_registers[(cur_opcode & 0x0F00) >> 8] = (cur_opcode & 0x00F0) >> 4;
      break;
    case 0x0001: // bitwise or (Vx | Vy)
      cpu_registers[(cur_opcode & 0x0F00) >> 8] |= (cur_opcode & 0x00F0) >> 4;
      break;
    case 0x0002: // bitwise and (Vx & Vy)
      cpu_registers[(cur_opcode & 0x0F00) >> 8] &= (cur_opcode & 0x00F0) >> 4;
      break;
    case 0x0003: // bitwise xor (Vx ^ Vy)
      cpu_registers[(cur_opcode & 0x0F00) >> 8] ^= (cur_opcode & 0x00F0) >> 4;
      break;
    case 0x0004: // Sum (Vx += Vy)
      break;
    case 0x0005: // Subtraction (Vx -= Vy)
      break;
    case 0x0006: // Shifts Vx >>= 1, LSB in VF
      break;
    case 0x0007: // Subtraction (Vx = Vy - Vx)
      break;
    case 0x000E: // Shifts Vx <<= 1, MSB in VF
      break;
    default:
      cerr << "Unknown opcode call: " << cur_opcode << "\n";
  }
}

void Chip8::UpdateTimers(){
  if (delay_timer) --delay_timer;
  if (sound_timer){
    if (sound_timer == 1) cout << "BEEP\n";
    --sound_timer;
  }
}