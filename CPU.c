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

int main(int argc, char **argv)
{
    struct trace_item *tr_entry;
    size_t size;
    char *trace_file_name;
    int trace_view_on = 0;
    
    //variables that weren't in the original-----
    int prediction_method = 0; //stores the third argument
    
    struct trace_item *pipe[8]; //size 8 array for a size 8 pipeline
    int i;
    struct trace_item nop_item; //insert nop_item wherever you need a noop
    nop_item.type = ti_NOP;
    for (i = 0; i < 8; i++) { //initializing pipeline with noops (well, references to one noop. same effect.)
        pipe[i] = &nop_item;
    }
    int end_program_counter = 7; //# cycles needed to flush the pipeline before exiting
    
    int hazard_detected = 0; //equals 1 during a cycle if a hazard is detected
    int squash_counter = 0; //starts at 4 and counts down if control hazard detected
    struct trace_item squash_item; //insert squash_item wherever you need a squash
    squash_item.type = ti_NOP;
    int print_squash = 0; //number of NOPs that should instead be printed SQUASHED
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
        if (squash_counter > 0) { //pipeline is "not-stalled" for the fake squashes
            //insert squash at EX2
            pipe[4] = &squash_item;
            print_squash++;
            squash_counter--;
        }
        else { //new instruction inters IF stage (pipe[0])
            size = trace_get_item(&pipe[0]);
        }
        
        
        if (end_program_counter == 0) {//simulation is done when pipeline is flushed
            printf("+ Simulation terminates at cycle : %u\n", cycle_number);
            break;
        }
        else if (squash_counter > 0) {//the cycle when inserting squashes
            cycle_number++;
            
            //get the values of the instruction that's about to finish executing
            t_type = pipe[7]->type;
            t_sReg_a = pipe[7]->sReg_a;
            t_sReg_b = pipe[7]->sReg_b;
            t_dReg = pipe[7]->dReg;
            t_PC = pipe[7]->PC;
            t_Addr = pipe[7]->Addr;
            
            //EX2 and on move up the pipeline
            for (i = 7; i > 4; i--) {
                pipe[i] = pipe[i-1];
            }
        }
        else{//the normal cycle
            
            //if trace_get_item isn't returning trace items, we flush the pipeline
            if (!size) {
                pipe[0] = &nop_item;
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
            
            
            hazard_detected = 0;
            //if a branch instruction is in stage EX2 (pipe[4])
            if(pipe[4]->type == ti_BRANCH) {
                switch(prediction_method) {
                    case 0:
                        //no prediction
                        if (pipe[3]->type != ti_NOP) {
                            hazard_detected = (pipe[4]->Addr == pipe[3]->PC);
                        }
                        break;
                    case 1:
                        //1 bit prediction
                        break;
                    case 2:
                        //2 bit prediction
                        break;
                }
            }
            if (hazard_detected == 1) { //if any prediction method detects a hazard, insert the squashes!
                squash_counter = 4;
            }
            
            
            if (hazard_detected == 0) { //in this case, correcting one hazard precludes the others
                //STRUCTURAL AND DATA HAZARDS HERE
                
                //-----
            }
            
            
            if (hazard_detected == 0) {//pipe doesn't move normally when there's a hazard
                //move all instructions to next stage
                for (i = 7; i > 0; i--) {
                    pipe[i] = pipe[i-1];
                }
            }
            else if (squash_counter > 0) {
                //EX2 and on move up the pipeline
                for (i = 7; i > 4; i--) {
                    pipe[i] = pipe[i-1];
                }
            }
        }
        
        if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
            switch(t_type) {
                case ti_NOP:
                    if (print_squash > 0) {
                        printf("[cycle %d] SQUASHED: \n",cycle_number) ;
                        print_squash--;
                    }
                    else {
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
                    printf("[cycle %d] SPECIAL:",cycle_number) ;      	
                    break;
                case ti_JRTYPE:
                    printf("[cycle %d] JRTYPE:",cycle_number) ;
                    printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", t_PC, t_dReg, t_Addr);
                    break;
            }
        }
    }
    
    trace_uninit();
    
    exit(0);
}

