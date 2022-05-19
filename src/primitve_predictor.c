//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <math.h>

//
// TODO:Student Information
//
const char *studentName = "Shiting Ding";
const char *studentID   = "A59005480";
const char *email       = "s1ding@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;
int result, result_l, result_g;
int global_len, local_len, pc_len;
int global_predictor[8192];
int local_history_table[8192];
int local_predictor[8192];
int selector[8192];  // The best method to handle the non-variable length issue
int choice[8192]; // A selector used to determine the outcome of adapted gshare
int global_history = 0; //Very tricky here
int pcmask,lmask,gmask;
int de_pcmask_high,de_pc_index;
int change;
int pc_index;
int bhr;
int addr_g,addr_l,addr_s;
int local_history;
int de_addr_l,de_result_l;
int de_local_history_table[8192];
int de_local_predictor[8192];
int pc1pc2;
int BIT_RESERVE = 5;
int TICKET_MAX = 16;
int ticket_l,ticket_g,ticket_d;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//



//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  if (verbose != 0) {
      printf ("%s\n", "----------init predicton----------");
  }
  switch(bpType) {
	case GSHARE:
		global_len = 1<<ghistoryBits;
		gmask = ~(~0<<ghistoryBits);
		for(int i=0;i<global_len;i++){global_predictor[i] = WN;}
		
		break;
	case TOURNAMENT:		
		pc_len = 1<<pcIndexBits;
		local_len = 1<<lhistoryBits;
		global_len = 1<<ghistoryBits;
		pcmask = ~(~0<<pcIndexBits);
		lmask = ~(~0<<lhistoryBits);
		gmask = ~(~0<<ghistoryBits);
		if (verbose!=0){printf("ghistoryBits:%d,gmask:%d\n",ghistoryBits,gmask);}

		for(int i=0;i<pc_len;i++){local_history_table[i] = 0;}//As tricky as ghistory
		for(int i=0;i<local_len;i++){local_predictor[i] = WN;}
		for(int i=0;i<global_len;i++){global_predictor[i] = WN;}
		for(int i=0;i<global_len;i++){selector[i] = WN;}
		break;
	case CUSTOM:
		pc_len = 1<<pcIndexBits;
		local_len = 1<<lhistoryBits;
		global_len = 1<<ghistoryBits;
		pcmask = ~(~0<<pcIndexBits);
		de_pcmask_high = ((1<<(pcIndexBits-BIT_RESERVE))-1)<<pcIndexBits;
		if (verbose!=0) {printf("pcmask:%d, de_pcmask_high:%d\n",pcmask,de_pcmask_high);}
		lmask = ~(~0<<lhistoryBits);
		gmask = ~(~0<<ghistoryBits);
		ticket_l = ticket_g =ticket_d = TICKET_MAX/2;
		for(int i=0;i<pc_len;i++){local_history_table[i] = 0;}//As tricky as ghistory
		for(int i=0;i<local_len;i++){local_predictor[i] = WN;}
		for(int i=0;i<pc_len;i++){de_local_history_table[i] = 0;}//As tricky as ghistory
		for(int i=0;i<local_len;i++){de_local_predictor[i] = WN;}
		for(int i=0;i<global_len;i++){global_predictor[i] = WN;}
		// for(int i=0;i<global_len;i++){selector[i] = WN;}
		int init_state = CHOICE_MAX/2-1;
		for(int i=0;i<global_len;i++){choice[i] = init_state;} //Must check before transfer
		break;
	default:
		break;
		
  }

}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
    if (verbose != 0) {
      printf ("%s\n", "---------make predicton----------");
    }
	switch(bpType){
		case GSHARE:
			// First we construct a Branch History Shift Register (BHR)
			bhr = global_history & gmask;
			pc_index = pc & gmask;
			// XOR the address
			addr_g = pc_index ^ bhr;
			//Make predictions according to the predictor
			if ((global_predictor[addr_g]==SN) || (global_predictor[addr_g]==WN)) {
				result = NOTTAKEN;
			}
			else{
				result = TAKEN;
			}
			if (verbose!=0){
				printf("addr_g:%d, g_pred:%d\n",addr_g, global_predictor[addr_g]);
			}
			break;
		case TOURNAMENT:  //GSHARE + LOCAL
			pc_len = 1<<pcIndexBits;
			local_len = 1<<lhistoryBits;
			global_len = 1<<ghistoryBits;
			pcmask = ~(~0<<pcIndexBits);
			lmask = ~(~0<<lhistoryBits);
			gmask = ~(~0<<ghistoryBits);
			// Calculate BHR		
			bhr = global_history & gmask;
			pc_index = pc & pcmask; //fetch pc
			addr_l = local_history_table[pc_index] & lmask; // fetch l history for this pc
			addr_g  = pc_index ^ bhr;
			addr_s = pc & gmask;
			
			if ((local_predictor[addr_l]==SN) || (local_predictor[addr_l]==WN)) {
				result_l = NOTTAKEN;
			}
			else{
				result_l = TAKEN;
			}
			if ((global_predictor[addr_g]==SN) || (global_predictor[addr_g]==WN)) {
				result_g = NOTTAKEN;
			}
			else{
				result_g = TAKEN;
			}
			if (verbose!=0){
				printf("addr_g:%d, g_pred:%d\n",addr_g, global_predictor[addr_g]);
			}
			if (verbose!=0){
				printf("l_pred:%d,g_pred:%d\n",local_predictor[addr_l],global_predictor[addr_g]);
			}
			if (verbose!=0){
				printf("With selector = %d\n",selector[addr_s]);
				if(selector[addr_s]<=WN){printf("using global result\n");}
				else {printf("addr_l = %d, using local result\n",addr_l);}
			}
			if (verbose != 0) {
			printf("-------make prediction--------\n");
			printf ("ghistory:%d,gPredict:%d\n",bhr,result_g);
			printf ("pcIndex:%d,lPredict:%d\n",pc_index,result_l);
			printf ("choice:%d\n", selector[addr_s]);
			}
			if ((selector[addr_s] == SN) || (selector[addr_s] ==WN)){ //pc2 has its advantages
				result = result_g;
			}
			else{ //pc1 has its advantages
				result = result_l;
			}
			break;
		case CUSTOM:  //Adapted Gshare + LOCAL
			// Calculate BHR
			bhr = global_history & gmask;
			pc_index = pc & pcmask; //fetch pc
			de_pc_index = ((pc &de_pcmask_high)>>pcIndexBits-BIT_RESERVE)+(pc&((1<<BIT_RESERVE)-1));
			if (verbose!=0) {printf("pc_index:%d, de_pc_index:%d\n",pc_index,de_pc_index);}
			// addr_l = local_history_table[pc_index] & lmask; // fetch l history for this pc
			//Must check, Very tricky out here
			addr_l = local_history_table[pc_index] & lmask;
			de_addr_l = de_local_history_table[de_pc_index] & lmask;
			// addr_l = pc_index & lmask;
			// de_addr_l = pc_index & lmask;

			addr_g  = pc_index ^ bhr;
			addr_s = pc & gmask;
			//LOCAL is split into 2 parts
			// if (verbose!=0){ //Must check
				// printf("choice = %d\n",choice[pc_index]);
				// if(choice[pc_index]==0){printf("returning the 1st bit of the counter\n");}
				// else {printf("returning the 2nd bit of the counter\n");}
			// }

			if ((local_predictor[addr_l]==SN) || (local_predictor[addr_l]==WN)) {
				result_l = NOTTAKEN;
			}
			else{
				result_l = TAKEN;
			}
			if ((de_local_predictor[de_addr_l]==SN) || (de_local_predictor[de_addr_l]==WN)) {
				de_result_l = NOTTAKEN;
			}
			else{
				de_result_l = TAKEN;
			}
			
			if ((global_predictor[addr_g]==SN) || (global_predictor[addr_g]==WN)) {
				result_g = NOTTAKEN;
			}
			else{
				result_g = TAKEN;
			}
			if (verbose!=0){
				printf("de_l_pred:%d,l_pred:%d,g_pred:%d\n",de_local_predictor[de_addr_l],local_predictor[addr_l],global_predictor[addr_g]);
			}
			// if (verbose!=0){ 
				// printf("With selector = %d\n",selector[addr_s]);
				// if(selector[addr_s]<=WN){printf("using global result\n");}
				// else {printf("using local result\n");}
			// }
			// if ((selector[addr_s] == SN) || (selector[addr_s] ==WN)){ //pc2 has its advantages
				// result = result_g;

			// }
			// else{ //pc1 has its advantages
				// result = result_l;
			// }
			//Must check
			// if (choice[pc_index] < CHOICE_MAX/2){ //1st bit selected 
				// result = result_g;
			// }
			// else{ //2nd bit selected
				// result = result_l;
			// }
			//Must Check
			 if ((global_predictor[addr_g]*2-3)+(local_predictor[addr_l]*2-3)+de_local_predictor[de_addr_l]*2-3>0){
			//if ((2*result_g-1)*ticket_g+(2*result_l-1)*ticket_l+(2*de_result_l-1)*ticket_d>0){	
				result = TAKEN;
			}
			else{
				result = NOTTAKEN;
			}
			break;  
	}
 
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
	  return result;	  
    case TOURNAMENT:
	  return result;
    case CUSTOM:
	  return result;
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  // Update the predictor
	pc_len = 1<<pcIndexBits;
	local_len = 1<<lhistoryBits;
	global_len = 1<<ghistoryBits;
	pcmask = ~(~0<<pcIndexBits);
	lmask = ~(~0<<lhistoryBits);
	gmask = ~(~0<<ghistoryBits);
  if (verbose != 0) {
	  printf ("\n%s\n", "---------train predictor---------");
	  printf ("outcome: %d\n", outcome);
  }
  if ((bpType==GSHARE) || (bpType==TOURNAMENT)||(bpType==CUSTOM)){
	bhr = global_history & gmask;
	if (bpType==GSHARE){pc_index = pc & gmask;}
	else {pc_index = pc & pcmask;}
	if (bpType==CUSTOM){de_pc_index = ((pc &de_pcmask_high)>>pcIndexBits-BIT_RESERVE)+(pc&((1<<BIT_RESERVE)-1));}
	addr_g = (pc_index ^ bhr) & gmask;
	change = outcome * 2 - 1;
	addr_s = pc & gmask;
	if (verbose!=0) {printf("pcmask:%d, de_pcmask_high:%d\n",pcmask,de_pcmask_high);}
	
	if ((bpType==TOURNAMENT)||(bpType==CUSTOM)){
		if (bpType == TOURNAMENT){
			addr_l = local_history_table[pc_index] & lmask;
		}
		else { //CUSTOM predictor
			//Must check, Very tricky out here
			addr_l = local_history_table[pc_index] & lmask;
			de_addr_l = de_local_history_table[de_pc_index] & lmask;
			// addr_l = pc_index & lmask;
			// de_addr_l = pc_index & lmask;
			if (result_l == outcome){ticket_l = (ticket_l > TICKET_MAX/2)? MIN((ticket_l>>1)|ticket_l,TICKET_MAX) :(ticket_l<<1);} else{ticket_l = MAX(ticket_l>>1,1);}
			if (result_g == outcome){ticket_g = (ticket_g > TICKET_MAX/2)? MIN((ticket_g>>1)|ticket_g,TICKET_MAX) :(ticket_g<<1);} else{ticket_g = MAX(ticket_g>>1,1);}
			if (de_result_l == outcome){ticket_d = (ticket_d > TICKET_MAX/2)? MIN((ticket_d>>1)|ticket_d,TICKET_MAX) :(ticket_d<<1);}else{ticket_d = MAX(ticket_d>>1,1);}
		if (verbose!=0){printf("Tickets for g: %d l: %d d: %d",ticket_g,ticket_l,ticket_d);}
		}
		pc1pc2 = (outcome==result_l) - (outcome==result_g);//local_predictor as P1， global as P2
		selector[addr_s] = MAX(MIN(selector[addr_s] + pc1pc2, ST),SN);	
		if (verbose != 0) {
			printf("selector_after:%d\n",selector[addr_s]);
		}

	}
	
	if (verbose != 0) {
	  printf("gmask: %d\n", gmask);
	  printf("bhr: %d\n", bhr);
	  printf("pc_index: %d\n", pc_index);
	}
	// Update g_predictor, l_predictor at first
	// if (bpType!=CUSTOM){//Must check
		// // default rules
		// if (change == 1){
		// global_predictor[addr_g] = MIN(global_predictor[addr_g] + change,ST);
		// }
		// else {
		// global_predictor[addr_g] = MAX(global_predictor[addr_g] + change,SN);
		// }
		
	// } 
	// if (verbose!=0){printf("Updating global_predictor[%d]\n",addr_g);}
	if (change == 1){
		global_predictor[addr_g] = MIN(global_predictor[addr_g] + change,ST);
	}
	else {
		global_predictor[addr_g] = MAX(global_predictor[addr_g] + change,SN);
	}

	
	
	if (bpType==TOURNAMENT){
		if (change == 1){
		local_predictor[addr_l] = MIN(local_predictor[addr_l] + change,ST);

		}
		else {
		local_predictor[addr_l] = MAX(local_predictor[addr_l] + change,SN);

		}		
	}
   if (bpType==CUSTOM){ //Must check
		if (change == 1){
		local_predictor[addr_l] = MIN(local_predictor[addr_l] + change,ST);
		de_local_predictor[de_addr_l] = MIN(de_local_predictor[de_addr_l] + change,ST);
		}
		else {
		local_predictor[addr_l] = MAX(local_predictor[addr_l] + change,SN);
		de_local_predictor[de_addr_l] = MAX(de_local_predictor[de_addr_l] + change,SN);
		}	
  }

	
	if (verbose != 0) {
		printf("local_predictor_after: %d\n",local_predictor[addr_l]);
	}
	// Update the global history register
	global_history = (global_history << 1) & gmask;
	global_history = global_history + outcome;
		// if (verbose != 0){
		// printf ("global_history after: %d\n",global_history & gmask);
		// }
	// Update local history table
	if ((bpType==TOURNAMENT)||(bpType==CUSTOM)){
		if (verbose != 0) {
		printf("LHT[%d] before changing: %d\n",pc_index,local_history_table[pc_index]);
		}
		local_history_table[pc_index] = (local_history_table[pc_index]<<1) + outcome;
		de_local_history_table[de_pc_index] = (de_local_history_table[de_pc_index]<<1) + outcome;
		if (verbose != 0) {
		printf("LHT[%d] after changing: %d\n",pc_index,local_history_table[pc_index]);
		}
	}
	// Always update choice if custom
	if(bpType==CUSTOM){
		choice[pc_index] = MAX(MIN(choice[pc_index] + pc1pc2, CHOICE_MAX-1),0);	
		// if (verbose!=0){printf("choice now changed to %d\n",choice[pc_index]);}
	}

  }
  if (verbose != 0) {
	  printf ("global_history after the shift: %d\n",global_history & gmask);
  }

}
