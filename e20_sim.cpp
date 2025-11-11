#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <regex>
#include <cstdlib>

using namespace std;

// Some helpful constant values to use
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;


/*
    Loads an E20 machine code file into the list
    provided by mem. We assume that mem is
    large enough to hold the values in the machine
    code file.

    @param f Open file to read from
    @param mem Array represetnting memory into which to read program
*/
void load_machine_code(ifstream &f, uint16_t mem[]) {
    regex machine_code_re("^ram\\[(\\d+)\\] = 16'b(\\d+);.*$");
    size_t expectedaddr = 0;
    string line;
    while (getline(f, line)) {
        smatch sm;
        if (!regex_match(line, sm, machine_code_re)) {
            cerr << "Can't parse line: " << line << endl;
            exit(1);
        }
        size_t addr = stoi(sm[1], nullptr, 10);
        unsigned instr = stoi(sm[2], nullptr, 2);
        if (addr != expectedaddr) {
            cerr << "Memory addresses encountered out of sequence: " << addr << endl;
            exit(1);
        }
        if (addr >= MEM_SIZE) {
            cerr << "Program too big for memory" << endl;
            exit(1);
        }
        expectedaddr ++;
        mem[addr] = instr;
    }
}

/*
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/
void print_state(uint16_t pc, uint16_t regs[], uint16_t memory[], size_t memquantity) {
    cout << setfill(' ');
    cout << "Final state:" << endl;
    cout << "\tpc=" <<setw(5)<< pc << endl;

    for (size_t reg=0; reg<NUM_REGS; reg++)
        cout << "\t$" << reg << "="<<setw(5)<<regs[reg]<<endl;

    cout << setfill('0');
    bool cr = false;
    for (size_t count=0; count<memquantity; count++) {
        cout << hex << setw(4) << memory[count] << " ";
        cr = true;
        if (count % 8 == 7) {
            cout << endl;
            cr = false;
        }
    }
    if (cr)
        cout << endl;
}

void sign_extend7_func(uint16_t& imm7)
{
    if ((imm7 >> 6) == 1) //msb of the imm7 is set, aka is 1, so imm7 < 0; Sign extend
    {
        imm7 = imm7 | 65408;
    }
}

/**
    Main function
    Takes command-line args as documented below
*/
int main(int argc, char *argv[]) {
    /*
        Parse the command-line arguments
    */
    char* filename = nullptr;
    bool do_help = false;
    bool arg_error = false;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else
                arg_error = true;
        } else {
            if (filename == nullptr)
                filename = argv[i];
            else
                arg_error = true;
        }
    }
    /* Display error message if appropriate */
    if (arg_error || do_help || filename == nullptr) {
        cerr << "usage " << argv[0] << " [-h] filename" << endl << endl;
        cerr << "Simulate E20 machine" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        return 1;
    }

    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }


    // Load f and parse using load_machine_code
    uint16_t pc = 0; //initializes pc to 0
    uint16_t regs_arr[NUM_REGS]; //create an array called regs_arr of size 8 (index 0 to index 7), each index can hold a unsigned 16-bit integer
    for (size_t i = 0; i < NUM_REGS; i++) //loop over each element in the array
    {
        regs_arr[i] = 0; //initializes all registers to 0
    }
    uint16_t memory_arr[MEM_SIZE]; //create an array called memory_arr of size 8192 (index 0 to index 8191), each index can hold a unsigned 16-bit integer
    for (size_t i = 0; i < MEM_SIZE; i++) //loop over each element in the array
    {
        memory_arr[i] = 0; //initializes all memory to 0
    }


    // Do simulation.
    load_machine_code(f, memory_arr);
    bool halt = false;

    while (!halt)
    {
        uint16_t index = pc % MEM_SIZE; // pc is 16-bit unsigned integer, MEM_SIZE is 13-bit; this always makes sure pc < MEM_SIZE. If PC > MEM_SIZE, modulus forces pc to wrap around to 0
        uint16_t instruction = memory_arr[index]; //indexes memory_arr at index; will never be out of range due to line above, and will always loop over and over without a problem


        //Extract all possible combinations
        uint16_t opcode = instruction >> 13;
        uint16_t bits10_12 = (instruction >> 10) & 7;
        uint16_t bits7_9 = (instruction >> 7) & 7;
        uint16_t bits4_6 = (instruction >> 4) & 7;
        uint16_t bits0_3 = instruction & 15;
        uint16_t bits0_6 = instruction & 127;
        uint16_t bits0_12 = instruction & 8191;
        sign_extend7_func(bits0_6);

        if (opcode == 0) //add, sub, or, and, slt, jr, 
        {
            if (bits0_3 == 0) //add
            {
                regs_arr[bits4_6] = regs_arr[bits10_12] + regs_arr[bits7_9];
                pc+=1;
            }

            else if (bits0_3 == 1) //sub
            {
                regs_arr[bits4_6] = regs_arr[bits10_12] - regs_arr[bits7_9];
                pc+=1;
            }
            
            else if (bits0_3 == 2) //or
            {
                regs_arr[bits4_6] = regs_arr[bits10_12] | regs_arr[bits7_9];
                pc+=1;
            }

            else if (bits0_3 == 3) //and
            {
                regs_arr[bits4_6] = regs_arr[bits10_12] & regs_arr[bits7_9];
                pc+=1;
            }

            else if (bits0_3 == 4) //slt
            {
                if (regs_arr[bits10_12] < regs_arr[bits7_9])
                {
                    regs_arr[bits4_6] = 1;
                }
                else
                {
                    regs_arr[bits4_6] = 0;
                }
                pc+=1;
            }

            else if (bits0_3 == 8) //jr
            {
                pc = regs_arr[bits10_12];
            }

            regs_arr[0] = 0; //ensures that the zero register is always 0
        }

        else if (opcode == 1) //addi
        {
            regs_arr[bits7_9] = regs_arr[bits10_12] + bits0_6;
            regs_arr[0] = 0; //ensures that the zero register is always 0
            pc+=1;
        }

        else if (opcode == 2) //j
        {
            if (pc == bits0_12) //if pc will jump to itself
            {
                halt = true; //set halt to be true to stop loop
            }
            pc = bits0_12;
        }

        else if (opcode == 3) //jal
        {
            regs_arr[7] = pc + 1;
            pc = bits0_12;
        }

        else if (opcode == 4) //lw
        {
            uint16_t address = (regs_arr[bits10_12] + bits0_6) % MEM_SIZE;
            regs_arr[bits7_9] = memory_arr[address];
            regs_arr[0] = 0; //ensures that the zero register is always 0
            pc+=1;
        }

        else if (opcode == 5) //sw
        {
            uint16_t address = (regs_arr[bits10_12] + bits0_6) % MEM_SIZE;
            memory_arr[address] = regs_arr[bits7_9];
            pc+=1;
        }

        else if (opcode == 6) //jeq
        {
            if (regs_arr[bits10_12] == regs_arr[bits7_9])
            {
                pc = pc + 1 + bits0_6;
            }
            else
            {
                pc+=1;
            }
        }

        else if (opcode == 7) //slti
        {
            if (regs_arr[bits10_12] < bits0_6)
            {
                regs_arr[bits7_9] = 1;
            }
            else
            {
                regs_arr[bits7_9] = 0;
            }
            regs_arr[0] = 0;
            pc+=1;
        }
    }
    
    // print the final state of the simulator before ending, using print_state
    print_state(pc, regs_arr, memory_arr, 128);

    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9
