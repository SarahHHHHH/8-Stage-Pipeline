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
      // Pipeline is "not-stalled" for the fake squashes
      // New instruction enters IF stage (pipe[0])
      if (squash_counter > 0) {
         // Insert squash at EX2
         pipe[4] = &squash_item;
         print_squash++;
         squash_counter--;
      } else {
         size = trace_get_item(&pipe[0]);
      }

      // Simulation is done when pipeline is flushed
      if (end_program_counter == 0) {
         printf("+ Simulation terminates at cycle : %u\n", cycle_number);
         break;
      } else if (squash_counter > 0) {
         // The cycle when inserting squashes
         cycle_number++;

         // Get the values of the instruction that's about to finish executing
         t_type = pipe[7]->type;
         t_sReg_a = pipe[7]->sReg_a;
         t_sReg_b = pipe[7]->sReg_b;
         t_dReg = pipe[7]->dReg;
         t_PC = pipe[7]->PC;
         t_Addr = pipe[7]->Addr;

         // EX2 and on move up the pipeline
         for (i = 7; i > 4; i--) {
            pipe[i] = pipe[i-1];
         }
      } else {
         // The normal cycle
         // If trace_get_item isn't returning trace items, we flush the pipeline
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
         // If a branch instruction is in stage EX2 (pipe[4])
         if (pipe[4]->type == ti_BRANCH) {
            switch(prediction_method) {
               case 0:
               // No prediction
               if (pipe[3]->type != ti_NOP) {
                  hazard_detected = (pipe[4]->Addr == pipe[3]->PC);
               }
               break;
               case 1:
               // 1 bit prediction
               break;
               case 2:
               // 2 bit prediction
               break;
            }
         }

         // If any prediction method detects a hazard, insert the squashes!
         if (hazard_detected == 1) {
            squash_counter = 4;
         }


         // In this case, correcting one hazard precludes the others
         if (hazard_detected == 0) {
            //STRUCTURAL AND DATA HAZARDS HERE

            //-----
         }

         // If the instruction in WB is trying to right to the register
         // Stall ID and subsequent instructions
         if (pipe[7]->type == ti_RTYPE || pipe[7]->type == ti_ITYPE || pipe[7]->type == ti_LOAD) {
            hazard_detected = 1;
            for (i = 7; i > 3; i--) {
               pipe[i] = pipe[i-1];
            }
            pipe[3] = pipe[i] = &nop_item;
         }

         // If EX1 is going to need to write to reg X while ID is reading from reg X
         // Stall ID and subsequent instructions
         // Forward result from EX1/MEM1 to ID/EX1 (following cycle)
         // Inject no-op in EX1/EX2
         if (pipe[3]->type == ti_RTYPE || pipe[3]->type == ti_ITYPE || pipe[3]->type == ti_LOAD) {
            // This could be one if statement, but I think this is easier to read
            if (pipe[2]->sReg_a == pipe[3]->dReg || pipe[2]->sReg_b == pipe[3]->dReg) {
               hazard_detected = 1;
               for (i = 7; i > 3; i--) {
                  pipe[i] = pipe[i-1];
               }
               // Im not really sure how we forward data, do we have any?
               pipe[3] = pipe[i] = &nop_item;
            }
         }

         // If EX2 is load which is going to need to write into reg X while ID is reading from reg X
         // Stall ID and subsequent instructions
         // Inject no-op in EX1/EX2
         if (pipe[4]->type == ti_LOAD) {
            if (pipe[2]->sReg_a == pipe[4]->dReg || pipe[2]->sReg_b == pipe[4]->dReg) {
               hazard_detected = 1;
               for (i = 7; i > 3; i--) {
                  pipe[i] = pipe[i-1];
               }
               pipe[3] = pipe[i] = &nop_item;
            }
         }

         // If MEM1 is load is going to need to write into reg X while ID is reading from reg X
         // Stall ID and subsequent instructions
         // Forward result from MEM2/WB to ID/EX1 (following cycle)
         // Inject no-op in EX1/EX2
         if (pipe[5]->type == ti_LOAD) {
            if (pipe[2]->sReg_a == pipe[5]->dReg || pipe[2]->sReg_b == pipe[5]->dReg) {
               hazard_detected = 1;
               for (i = 7; i > 3; i--) {
                  pipe[i] = pipe[i-1];
               }
               // Again not sure about forwarding
               pipe[3] = pipe[i] = &nop_item;
            }
         }

         // Pipe doesn't move normally when there's a hazard
         if (hazard_detected == 0) {
            // Move all instructions to next stage
            for (i = 7; i > 0; i--) {
               pipe[i] = pipe[i-1];
            }
         } else if (squash_counter > 0) {
            //EX2 and on move up the pipeline
            for (i = 7; i > 4; i--) {
               pipe[i] = pipe[i-1];
            }
         }
      }

      // Print the executed instruction if trace_view_on=1
      if (trace_view_on) {
         switch(t_type) {
            case ti_NOP:
            if (print_squash > 0) {
               printf("[cycle %d] SQUASHED: \n",cycle_number) ;
               print_squash--;
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
