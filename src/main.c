//========================================================//
//  CSE 240a Branch Lab                                   //
//                                                        //
//  Students need to implement various Branch Predictors  //
//========================================================//

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "predictor.h"

FILE *stream;
char *buf = NULL;
size_t len = 0;

// Print out the Usage information to stderr
//
void
usage()
{
  fprintf(stderr,"Usage: predictor <options> [<trace>]\n");
  fprintf(stderr,"       bunzip -kc trace.bz2 | predictor <options>\n");
  fprintf(stderr," Options:\n");
  fprintf(stderr," --help       Print this message\n");
  fprintf(stderr," --verbose    Print predictions on stdout\n");
  fprintf(stderr," --<type>     Branch prediction scheme:\n");
  fprintf(stderr,"    static\n"
                 "    gshare:<# ghistory>\n"
                 "    tournament:<# ghistory>:<# lhistory>:<# index>\n"
                 "    custom\n");
}

// Process an option and update the predictor
// configuration variables accordingly
//
// Returns True if Successful
//
int
handle_option(char *arg)
{
  if (!strcmp(arg,"--static")) {
    bpType = STATIC;
  } else if (!strncmp(arg,"--gshare:",9)) {
    bpType = GSHARE;
    sscanf(arg+9,"%d", &ghistoryBits);
  } else if (!strncmp(arg,"--tournament:",13)) {
    bpType = TOURNAMENT;
    sscanf(arg+13,"%d:%d:%d", &ghistoryBits, &lhistoryBits, &pcIndexBits);
  } else if (!strncmp(arg,"--custom:",9)) {
    bpType = CUSTOM;
	sscanf(arg+9,"%d:%d:%d", &ghistoryBits, &lhistoryBits, &pcIndexBits);
  } else if (!strcmp(arg,"--verbose")) {
    verbose = 1;
  } else {
    return 0;
  }

  return 1;
}

// Reads a line from the input stream and extracts the
// PC and Outcome of a branch
//
// Returns True if Successful 
//
int
read_branch(uint32_t *pc, uint8_t *outcome)
{
  if (getline(&buf, &len, stream) == -1) {
    return 0;
  }

  uint32_t tmp;
  sscanf(buf,"0x%x %d\n",pc,&tmp);
  *outcome = tmp;

  return 1;
}

int
main(int argc, char *argv[])
{
  // Set defaults
  stream = stdin;
  bpType = STATIC;
  verbose = 0;

  // Process cmdline Arguments
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i],"--help")) {
      usage();
      exit(0);
    } else if (!strncmp(argv[i],"--",2)) {
      if (!handle_option(argv[i])) {
        printf("Unrecognized option %s\n", argv[i]);
        usage();
        exit(1);
      }
    } else {
      // Use as input file
      stream = fopen(argv[i], "r");
    }
  }

  // Initialize the predictor

  init_predictor();

  uint32_t num_branches = 0;
  uint32_t num_branches_g = 0;
  uint32_t num_branches_l = 0;
  uint32_t mispredictions = 0;
  uint32_t mispred_g = 0;
  uint32_t mispred_l = 0;
  uint32_t sel_state = 0;
  uint32_t pc = 0;
  uint8_t outcome = NOTTAKEN;

  // Reach each branch from the trace
  while (read_branch(&pc, &outcome)) {
    num_branches++;
	sel_state = selector[addr_s];
	if ((sel_state== SN) || (sel_state ==WN)){ //pc2 has its advantages
	num_branches_g++;

	}
	else{ //pc1 has its advantages
	num_branches_l++;
	}

    // Make a prediction and compare with actual outcome
    uint8_t prediction = make_prediction(pc);
    if (prediction != outcome) {
      mispredictions++;
	// if (verbose != 0) {
      // printf ("Outcome!\n");
    // }
	  if ((sel_state == SN) || (sel_state ==WN)){ //pc2 has its advantages
	  mispred_g++;
	  }
	  else{ //pc1 has its advantages
	  mispred_l++;
	  }
    }
    if (verbose != 0) {
      printf ("prediction: %d\n", prediction);
	  printf ("outcome: %d\n", outcome);
    }


    // Train the predictor
    train_predictor(pc, outcome);
  }

  // Print out the mispredict statistics
  printf("--Branches:        %10d\n", num_branches);
  printf("--Incorrect:       %10d\n", mispredictions);
  float mispredict_rate = 100*((float)mispredictions / (float)num_branches);
  printf("--Misprediction Rate: %7.3f\n", mispredict_rate);
  if ((bpType==TOURNAMENT)||(bpType == CUSTOM)){
  float GMR = 100*((float)mispred_g / (float)num_branches_g);
  float Gtakesup = 100*((float)num_branches_g / (float)num_branches);
  printf("Global prediction takes up: %7.3f\n", Gtakesup);
  printf("Global misprediction Rate: %7.3f\n", GMR);
  float LMR = 100*((float)mispred_l / (float)num_branches_l);
  float Ltakesup = 100*((float)num_branches_l / (float)num_branches);
  printf("Local prediction takes up: %7.3f\n", Ltakesup);
  printf("Local misprediction Rate: %7.3f\n", LMR);
  int non_0_history=0;
  int non_WN=0;
  for(int i=0; i<1024;i++){if (local_history_table[i]!=0){non_0_history+=1;}}
  printf("non_0 in BHT:%d\n", non_0_history);
  for(int i=0; i<1024;i++){if (local_predictor[i]!=WN){non_WN+=1;}}
  printf("non_WN in BPT:%d\n", non_WN);
  }

  // Cleanup
  fclose(stream);
  free(buf);

  return 0;
}
