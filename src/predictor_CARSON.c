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
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
uint8_t *gBHT;
uint8_t *lBHT;
uint8_t *chooser;
uint32_t *PHT;
uint32_t ghistory;
uint32_t gMask;
uint32_t lMask;
uint32_t pcMask;

unsigned int perceptron_size;
unsigned int perceptron_pt_idx_max;
unsigned int perceptron_threshold;
unsigned int perceptron_weight_count;
int8_t weight_max;
int8_t weight_min;
int8_t** weight_table;
int total_weight;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//
int
power(int base, int times) {
    if (times == 0) return 1;
    return base * power(base, times - 1);
}
// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
    if (bpType == CUSTOM) {
        ghistoryBits = 12;
        pcIndexBits = 6;
    }

    gMask = 0;
    
    for (int i = 0; i < ghistoryBits; i++) {
        gMask = (gMask << 1) + 1;
    }

    lMask = 0;
    
    for (int i = 0; i < lhistoryBits; i++) {
        lMask = (lMask << 1) + 1;
    }

    pcMask = 0;
    
    for (int i = 0; i < pcIndexBits; i++) {
        pcMask = (pcMask << 1) + 1;
    }

    switch (bpType) {
        case STATIC:

        case GSHARE:
            ghistory = 0;
            int size = (int)power(2, ghistoryBits);
            gBHT = (uint8_t*)malloc(size * sizeof(uint8_t));
            for(int i = 0; i < size; i++) {
                gBHT[i] = WN;
            }
            break;          

        case TOURNAMENT:
            ghistory = 0;
            int gSize = (int)power(2, ghistoryBits);
            int lSize = (int)power(2, lhistoryBits);
            int pcSize = (int)power(2, pcIndexBits);
            
            gBHT = (uint8_t*)malloc(gSize * sizeof(uint8_t));
            lBHT = (uint8_t*)malloc(lSize * sizeof(uint8_t));
            chooser = (uint8_t*)malloc(gSize * sizeof(uint8_t));
            PHT = (uint32_t*)malloc(pcSize * sizeof(uint32_t));

            for(int i = 0; i < gSize; i++) {
                gBHT[i] = WN;
            }

            for(int i = 0; i < lSize; i++) {
                lBHT[i] = WN;
            }

            for(int i = 0; i < gSize; i++) {
                chooser[i] = WN;
            }

            for(int i = 0; i < pcSize; i++) {
                PHT[i] = 0;
            }           
            break;  

        case CUSTOM: {
            ghistory = 0;


            perceptron_size = 224;
            perceptron_weight_count = 28;
            perceptron_pt_idx_max = 224;
            perceptron_threshold = 68;
            weight_max = 127;
            weight_min = -128;

            // perceptron_size = 256;
            // perceptron_weight_count = 24;
            // perceptron_pt_idx_max = 256;
            // perceptron_threshold = 60;
            // weight_max = 127;
            // weight_min = -128;

            weight_table = (int8_t **)malloc(perceptron_size * sizeof(int8_t *));
            for (int i = 0; i < perceptron_size; ++i) {
                weight_table[i] = (int8_t *)malloc(perceptron_weight_count * sizeof(int8_t));
                for (int j = 0; j < perceptron_weight_count; ++j) {
                    weight_table[i][j] = 0;
                }
            }

        }
        default:
            break;
    }
}

// int ghistoryTransform() {
//     int ghr = 0;
//     for (int i = 0; i < ghistoryBits; i++) {
//         ghr = ghr << 1;
//         if (history[i] == 1) {
//             ghr = ghr + 1;
//         }
//     }
//     return ghr;
// }

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //
  // Make a prediction based on the bpType
    switch (bpType) {
        case STATIC:
            return TAKEN;
        case GSHARE: {
            // uint32_t index = (pc) ^ (ghistory & mask);
            uint32_t index = (pc & gMask) ^ (ghistory & gMask);
            return gBHT[index] <= 1 ? NOTTAKEN : TAKEN;         
        }
        case TOURNAMENT: {
            uint8_t gPredict = gBHT[ghistory];

            uint32_t pcIndex = pc & pcMask;
            uint32_t lPredict = lBHT[PHT[pcIndex]];

            uint8_t choice = chooser[ghistory];
			if (verbose != 0) {
			printf("-------make prediction--------\n");
			printf ("bhr:%d,gPredict:%d\n",ghistory,gPredict);
			printf ("pcIndex:%d,lPredict:%d\n",pcIndex,lPredict);
			printf ("choice:%d\n", choice);
			}

            if (choice <= 1) {
                return gPredict <= 1 ? NOTTAKEN : TAKEN;
            } else {
                return lPredict <= 1 ? NOTTAKEN : TAKEN;
            }
        }
        case CUSTOM: {
            unsigned int pt_idx = (pc % perceptron_pt_idx_max); 
            total_weight = 0;
            
            for (int i = 0; i < perceptron_weight_count - 1; i++) {     
                if ((ghistory >> i) & 0x01) {
                    total_weight += weight_table[pt_idx][i];
                } else {
                    total_weight -= weight_table[pt_idx][i];            
                }
            }

            total_weight += weight_table[pt_idx][perceptron_weight_count-1];
            return total_weight >= 0 ? TAKEN : NOTTAKEN;            
        }
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

    switch (bpType) {
        case STATIC:

        case GSHARE: {
            // uint32_t index = (pc) ^ (ghistory & mask);
            uint32_t index = (pc & gMask) ^ (ghistory & gMask);
            
            if (outcome == NOTTAKEN) {
                ghistory = (ghistory << 1) & gMask;
                if (gBHT[index] >= 1) {
                    gBHT[index]--;
                }
            }
            else if (outcome == TAKEN) {
                ghistory = ((ghistory << 1) + 1) & gMask;
                if (gBHT[index] <= 2) {
                    gBHT[index]++;
                }
            }
            break;
        }
        
        case TOURNAMENT: {

            uint8_t gPredict = gBHT[ghistory];

            uint32_t lhistory = PHT[pc & pcMask];
            uint8_t lPredict = lBHT[lhistory];

            if (gPredict / 2 == outcome && lPredict / 2 != outcome) {
                if (chooser[ghistory] >= 1) {
                    chooser[ghistory]--;
                }
            }

            if (gPredict / 2 != outcome && lPredict / 2 == outcome) {
                if (chooser[ghistory] <= 2) {
                    chooser[ghistory]++;
                }
            }   

            if (outcome == NOTTAKEN) {

                if (gPredict >= 1) {
                    gBHT[ghistory]--;
                }

                if (lPredict >= 1) {
                    lBHT[lhistory]--;
                }   

                ghistory = (ghistory << 1) & gMask;
                PHT[pc & pcMask] = (PHT[pc & pcMask] << 1) & lMask;         
            }
            else if (outcome == TAKEN) {
                if (gPredict <= 2) {
                    gBHT[ghistory]++;
                }

                if (lPredict <= 2) {
                    lBHT[lhistory]++;
                }   

                ghistory = ((ghistory << 1) + 1) & gMask;
                PHT[pc & pcMask] = ((PHT[pc & pcMask] << 1) + 1) & lMask;
            }       
            break;
        }

        case CUSTOM: {
            unsigned int pt_idx = (pc % perceptron_pt_idx_max);   
            
            int predict = total_weight >= 0 ? TAKEN : NOTTAKEN;
            if ((predict != outcome) || (abs(total_weight) <= perceptron_threshold)) {
                for (int i = 0; i < perceptron_weight_count - 1; i++) {     
                    if (outcome != ((ghistory >> i) & 0x01)) {
                        if (weight_table[pt_idx][i] > weight_min) { 
                            weight_table[pt_idx][i] -= 1;
                        }
                    } else {
                        if (weight_table[pt_idx][i] < weight_max) {
                            weight_table[pt_idx][i] += 1;
                        }
                    }
                }

                if (outcome) {
                    if (weight_table[pt_idx][perceptron_weight_count-1] < weight_max) {
                        weight_table[pt_idx][perceptron_weight_count-1] += 1;
                    }
                } else {
                    if (weight_table[pt_idx][perceptron_weight_count-1] > weight_min) {
                        weight_table[pt_idx][perceptron_weight_count-1] -= 1;
                    }
                }
            }

            if (outcome) {
                ghistory = (ghistory << 1) | 0x01;
            } else {
                ghistory = (ghistory << 1) | 0x00;
            }           

            break;          

        }
        default:
            break;
    }   
}