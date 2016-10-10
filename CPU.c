/**************************************************************/
/* CS/COE 1541
 just compile with gcc -o CPU CPU.c
 and execute using
 ./CPU  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0   0
 ***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h"
#include "HashTable.h"

int main(int argc, char **argv) {
    struct trace_item *tr_entry;
    size_t size;
    char *trace_file_name;
    int trace_view_on = 0;
    
    //variables that weren't in the original-----
    int prediction_method = 0; //stores the third argument
    
    struct trace_item *pipe[8]; //size 8 array for a size 8 pipeline
    int i;
    struct trace_item nop_item; //insert nop_item wherever you need a no-op
    nop_item.type = ti_NOP;
    for (i = 0; i < 8; i++) { //initializing pipeline with no-ops (well, references to one no-op. same effect.)
        pipe[i] = &nop_item;
    }
    int end_program_counter = 7; //# cycles needed to flush the pipeline before exiting
    
    int hazard_detected = 0; //equals 1 during a cycle if a hazard is detected
    int squash_counter = 0; //starts at 4 and counts down if control hazard detected
    struct trace_item squash_item; //insert squash_item wherever you need a squash
    squash_item.type = ti_NOP;
    int print_squash[8]; //NOPs that should instead be printed SQUASHED get a 1
    int t_squashed = 0;
    
    struct DataItem *prediction; //pointer to entry in branch prediction table
    int branch_taken; //equals 1 if branch was taken this cycle
    //-----
    
    unsigned char t_type = 0;
    unsigned char t_sReg_a= 0;
    unsigned char t_sReg_b= 0;
    unsigned char t_dReg= 0;
    unsigned int t_PC = 0;
    unsigned int t_Addr = 0;
    
    unsigned int cycle_number = 0;
    
    if (argc == 1) {
        fprintf(stdout, "\nUSAGE: CPU <trace_file> <switch - 0 or 1> <method - 0, 1, or 2>\n");
        fprintf(stdout, "\n(switch) to turn on or off individual item view.\n");
        fprintf(stdout, "\n(method) to choose prediction method.\n\n");
        exit(0);
    }
    
    trace_file_name = argv[1];
    if (argc >= 3) trace_view_on = atoi(argv[2]) ;
    if (argc == 4) prediction_method = atoi(argv[3]) ;
    
    fprintf(stdout, "\n ** opening file %s\n", trace_file_name);
    
    trace_fd = fopen(trace_file_name, "rb");
    
    if (!trace_fd) {
        fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
        exit(0);
    }
    
    trace_init();
    
    while(1) {
        if (squash_counter > 0) { //Pipeline is "not-stalled" for the fake squashes
            // Insert squash at EX2
            //actually EX1
            pipe[3] = &squash_item;
            print_squash[3] = 1;
            squash_counter--;
        }
        else if (hazard_detected == 1) { //Pipeline stalled for structural or data hazard
            //Insert noop at EX1
            pipe[3] = &nop_item;
            print_squash[3] = 0;
        }
        else { //New instruction enters IF stage (pipe[0])
            size = trace_get_item(&pipe[0]);
        }
        
        // Simulation is done when pipeline is flushed
        if (end_program_counter == 0) {
            printf("+ Simulation terminates at cycle : %u\n", cycle_number);
            break;
        }
        else if (squash_counter > 0) {
            // The cycle when inserting squashes
            cycle_number++;
            
            // Get the values of the instruction that's about to finish executing
            t_type = pipe[7]->type;
            t_sReg_a = pipe[7]->sReg_a;
            t_sReg_b = pipe[7]->sReg_b;
            t_dReg = pipe[7]->dReg;
            t_PC = pipe[7]->PC;
            t_Addr = pipe[7]->Addr;
            t_squashed = print_squash[7];
            
            //EX1 and on move to the next stage
            for (i = 7; i > 3; i--) {
                pipe[i] = pipe[i-1];
                print_squash[i] = print_squash[i-1];
            }
        }
        else {
            // The normal cycle
            // If trace_get_item isn't returning trace items, we flush the pipeline
            if (!size) {
                pipe[0] = &nop_item;
                print_squash[0] = 0;
                end_program_counter--;
            }
            
            cycle_number++;
            
            //get the values of the instruction that's about to finish executing
            t_type = pipe[7]->type;
            t_sReg_a = pipe[7]->sReg_a;
            t_sReg_b = pipe[7]->sReg_b;
            t_dReg = pipe[7]->dReg;
            t_PC = pipe[7]->PC;
            t_Addr = pipe[7]->Addr;
            t_squashed = print_squash[7];
            
            
            hazard_detected = 0;
            // If a branch instruction is in stage EX2 (pipe[4])
            //But, since this simulation isn't like real circuits, the check can happen in EX1 (pipe[3])
            //In fact, this is necessary, since it prevents erroneous noops from messing up the check
            if (pipe[3]->type == ti_BRANCH) {
                switch(prediction_method) {
                    case 0:
                        // No prediction
                        if (pipe[2]->type != ti_NOP) {
                            hazard_detected = (pipe[3]->Addr == pipe[2]->PC);
                        }
                        break;
                    case 1:
                        // 1 bit prediction
                        if (pipe[2]->type != ti_NOP) {
                            //look branch up in the table
                            prediction = search(pipe[3]->PC);
                            
                            //if branch isn't in table, predict not taken, then add it with result of branch condition
                            if (prediction == NULL) {
                                hazard_detected = (pipe[3]->Addr == pipe[2]->PC);
                                insert(pipe[3]->PC, hazard_detected,0);
                            }
                            //if it is there…
                            else {
                                //hazard not detected yet. but this result equals 1 if the branch is taken
                                branch_taken = (pipe[3]->Addr == pipe[2]->PC);
                                
                                //if the result doesn't match the prediction, the hazard is confirmed
                                hazard_detected = (branch_taken != prediction->takenOne);
                                
                                //update the table.
                                prediction->takenOne = branch_taken;
                            }
                        }
                        break;
                    case 2:
                        // 2 bit prediction
                        if (pipe[2]->type != ti_NOP) {
                            //look branch up in the table
                            prediction = search(pipe[3]->PC);
                            
                            //if branch isn't in table, predict not taken, then add it with result of branch condition
                            if (prediction == NULL) {
                                hazard_detected = (pipe[3]->Addr == pipe[2]->PC);
                                insert(pipe[3]->PC, hazard_detected,0);
                            }
                            //if it is there…
                            else {
                                //hazard not detected yet. but this result equals 1 if the branch is taken
                                branch_taken = (pipe[3]->Addr == pipe[2]->PC);
                                
                                //confirm hazard and update table
                                if (branch_taken != prediction->takenTwo) {
                                    hazard_detected = 1;
                                    
                                    if (prediction->takenOne == prediction->takenTwo) {
                                        prediction->takenOne = branch_taken;
                                    }
                                    else {
                                        prediction->takenTwo = branch_taken;
                                    }
                                }
                                else {
                                    if (prediction->takenOne != prediction->takenTwo) {
                                        prediction->takenOne = branch_taken;
                                    }
                                    else {
                                        //do nothing
                                    }
                                }
                            }
                        }
                        break;
                }
            }
            // If any prediction method detects a hazard, insert the squashes!
            if (hazard_detected == 1) {
                squash_counter = 4;
            }
            
            
            //it works out that we should insert a noop for one hazard per cycle
            
            if (pipe[2]->type != ti_NOP) {
                
                // If the instruction in WB is trying to write to the register while ID is reading
                // Stall ID and subsequent instructions for one cycle
                //if WB's write reg is valid
                if ((hazard_detected == 0) && (pipe[7]->type != ti_NOP) && (pipe[7]->dReg >= 0) && (pipe[7]->dReg <= 31)) {
                    //if any of ID's read regs are valid
                    if (((pipe[2]->sReg_a >= 0) && (pipe[2]->sReg_a <= 31)) || ((pipe[2]->sReg_b >= 0) && (pipe[2]->sReg_b <= 31))) {
                        hazard_detected = 1;
                    }
                }
                
                // If EX1 is going to write to reg X while ID is reading from reg X
                // Stall ID and subsequent instructions for one cycle
                // Inject no-op in EX1/EX2
                //if EX1's write reg is valid
                if ((hazard_detected == 0) && (pipe[3]->type != ti_NOP) && (pipe[3]->dReg >= 0) && (pipe[3]->dReg <= 31)) {
                    //if any of ID's read regs are valid
                    if (((pipe[2]->sReg_a >= 0) && (pipe[2]->sReg_a <= 31)) || ((pipe[2]->sReg_b >= 0) && (pipe[2]->sReg_b <= 31))) {
                        //if any of ID's regs read from the same reg that EX1 writes
                        if ((pipe[2]->sReg_a == pipe[3]->dReg) || (pipe[2]->sReg_b == pipe[3]->dReg)) {
                            hazard_detected = 1;
                        }
                    }
                }
                
                // If EX2 is a load type which is going to write to reg X while ID is reading from reg X
                // Stall ID and subsequent instructions for one cycle
                // Inject no-op in EX1/EX2
                //if EX2's write reg is valid
                if ((hazard_detected == 0) && (pipe[4]->type != ti_NOP) && (pipe[4]->dReg >= 0) && (pipe[4]->dReg <= 31)) {
                    //if EX2 is load type
                    if (pipe[4]->type == ti_LOAD) {
                        //if any of ID's read regs are valid
                        if (((pipe[2]->sReg_a >= 0) && (pipe[2]->sReg_a <= 31)) || ((pipe[2]->sReg_b >= 0) && (pipe[2]->sReg_b <= 31))) {
                            //if any of ID's regs read from the same reg that EX2 writes
                            if ((pipe[2]->sReg_a == pipe[4]->dReg) || (pipe[2]->sReg_b == pipe[4]->dReg)) {
                                hazard_detected = 1;
                            }
                        }
                    }
                }
                
                // If MEM1 is a load type which is going to write to reg X while ID is reading from reg X
                // Stall ID and subsequent instructions for one cycle
                // Inject no-op in EX1/EX2
                //if MEM1's write reg is valid
                if ((hazard_detected == 0) && (pipe[5]->type != ti_NOP) && (pipe[5]->dReg >= 0) && (pipe[5]->dReg <= 31)) {
                    //if MEM1 is load type
                    if (pipe[5]->type == ti_LOAD) {
                        //if any of ID's read regs are valid
                        if (((pipe[2]->sReg_a >= 0) && (pipe[2]->sReg_a <= 31)) || ((pipe[2]->sReg_b >= 0) && (pipe[2]->sReg_b <= 31))) {
                            //if any of ID's regs read from the same reg that MEM1 writes
                            if ((pipe[2]->sReg_a == pipe[5]->dReg) || (pipe[2]->sReg_b == pipe[5]->dReg)) {
                                hazard_detected = 1;
                            }
                        }
                    }
                }
                
                
            }//closing bracket for "if (pipe[2]->type != ti_NOP) {"
            
            
            // Pipe doesn't move normally when there's a hazard
            if (hazard_detected == 0) {
                // Move all instructions to next stage
                for (i = 7; i > 0; i--) {
                    pipe[i] = pipe[i-1];
                    print_squash[i] = print_squash[i-1];
                }
            }
            else {
                //EX1 and on move to the next stage
                for (i = 7; i > 3; i--) {
                    pipe[i] = pipe[i-1];
                    print_squash[i] = print_squash[i-1];
                }
            }
        }
        
        // Print the executed instruction if trace_view_on=1
        if (trace_view_on) {
            switch(t_type) {
                case ti_NOP:
                    if (t_squashed == 1) {
                        printf("[cycle %d] SQUASHED: \n",cycle_number) ;
                    } else {
                        printf("[cycle %d] NOP: \n",cycle_number) ;
                    }
                    break;
                case ti_RTYPE:
                    printf("[cycle %d] RTYPE:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", t_PC, t_sReg_a, t_sReg_b, t_dReg);
                    break;
                case ti_ITYPE:
                    printf("[cycle %d] ITYPE:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", t_PC, t_sReg_a, t_dReg, t_Addr);
                    break;
                case ti_LOAD:
                    printf("[cycle %d] LOAD:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", t_PC, t_sReg_a, t_dReg, t_Addr);
                    break;
                case ti_STORE:
                    printf("[cycle %d] STORE:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", t_PC, t_sReg_a, t_sReg_b, t_Addr);
                    break;
                case ti_BRANCH:
                    printf("[cycle %d] BRANCH:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", t_PC, t_sReg_a, t_sReg_b, t_Addr);
                    break;
                case ti_JTYPE:
                    printf("[cycle %d] JTYPE:",cycle_number) ;
                    printf(" (PC: %x)(addr: %x)\n", t_PC,t_Addr);
                    break;
                case ti_SPECIAL:
                    printf("[cycle %d] SPECIAL:\n",cycle_number) ;
                    break;
                case ti_JRTYPE:
                    printf("[cycle %d] JRTYPE:",cycle_number) ;
                    printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", t_PC, t_sReg_a, t_Addr);
                    break;
            }
        }
    }
    
    trace_uninit();
    
    exit(0);
}
