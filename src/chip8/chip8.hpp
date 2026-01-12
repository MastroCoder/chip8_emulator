#include <vector>
#include <functional>
#include <iostream>
#include <cstdlib> // C rand()
#include <ctime>
#include <map>

using std::vector, std::function, std::cout, std::cerr, std::map;

using Opcode = unsigned short;

class Chip8{
  public:
    Chip8();
    void ReadOpcode();
    void InterpretOpcode();
    void InterpretIncrementingOpcodes();
    void UpdateTimers();
    void Interpret0BasedOpcodes();
    void Interpret8BasedOpcodes();
    void InterpretEBasedOpcodes();
    void InterpretFBasedOpcodes();
    void LoadGame();
    void Emulate();
    void DrawSprite();
    void ClearScreen();
    void SetKeys();

  private:
    unsigned short program_counter;
    
    // 60 Hz counters
    unsigned char delay_timer; // Game/program event timing 
    unsigned char sound_timer; // Sound timing (sound when 0)
    
    vector<unsigned char> memory; // each Opcode takes two bytes. Memory represents Opcodes as memory[i] >> 8 | memory[i+1].
    vector<unsigned char> cpu_registers; // V0 to VF
    vector<unsigned char> keypad_states;
    vector<bool> screen_pixels;

    vector<unsigned short> stack; // levels of instruction calls
    unsigned short stack_pointer; // Points to current stack call

    unsigned short address_register; // I

    Opcode cur_opcode;
};