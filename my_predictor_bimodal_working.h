// my_predictor.h
// This file contains a sample my_predictor class.
// It has a simple 32,768-entry gshare with a history length of 15 and a
// simple direct-mapped branch target buffer for indirect branch prediction.
#include <map>
#include <assert.h>
#include <cmath>
using namespace std;
class my_update : public branch_update {
public:
	unsigned int index;
	unsigned int vpca;
	unsigned int vghr;
	int pred_iter;
};

class my_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	20	
#define TABLE_BITS	26
#define MAX_ITER 12
#define SIZE_OF_BTB 1024	
	my_update u;
	branch_info bi;
	unsigned int history;
	unsigned char tab_taken[1<<(TABLE_BITS)];
	unsigned char tab_not_taken[1<<(TABLE_BITS)];
	unsigned char tab_choice[1<<(TABLE_BITS)];
	unsigned int targets[1<<TABLE_BITS];
	unsigned int hashvals[20];
	
		
	my_predictor (void) : history(0), hashvals {1392,4695,2615,5174,2648,2810,7302,5492,6865,3519,6155,3489,5263,6911,5501,2558} 
	{ 
		memset (tab_taken, 0, sizeof (tab_taken));
		memset (tab_not_taken, 0, sizeof (tab_not_taken));
		memset (tab_choice, 0, sizeof (tab_choice));
		memset (targets, 0, sizeof (targets));
	}
	unsigned int hash(unsigned int a)
	{
		a = (a ^ 61) ^ (a >> 16);
   		a = a + (a << 3);
    		a = a ^ (a >> 4);
    		a = a * 0x27d4eb2d;
   		a = a ^ (a >> 15);
    		return a;
	}
	
	unsigned int hash_func(unsigned int address, unsigned int hashvalue) //simple hash function which XORs the branch address with a random value chosen from a hardcoded table of values
	{
		return (address ^ hashvalue);
	}

	bool access_conditional_bp(unsigned int address, unsigned int ghr)
	{
			unsigned int choice_index = ((address) & ((1<<TABLE_BITS)-1));
			bool choice = (tab_choice[choice_index] >> 1);
			unsigned int common_index = (address & ((1<<TABLE_BITS)-1)) ^ (ghr << (TABLE_BITS - HISTORY_LENGTH));
			
			if(choice)
			{
				return (tab_taken[common_index] >> 1);
			}
			else
			{
				return (tab_not_taken[common_index] >> 1);
			}
	}
	void update_conditional_bp(unsigned int address,unsigned int ghr ,bool taken,int prediction)
	{
			unsigned int choice = ((address) & ((1<<TABLE_BITS)-1));
			unsigned char *c,*d; 	
			unsigned int tab_index = ((address) ^ (ghr  << (TABLE_BITS - HISTORY_LENGTH))) & ((1<<TABLE_BITS) -1);
			if(tab_choice[choice] >> 1)
			{
				c = &tab_taken[tab_index];
			}
			else
			{
				c = &tab_not_taken[tab_index];
			}

			if(taken){
				if (*c < 3) (*c)++;
			}else{
				if (*c > 0) (*c)--;
			}	
			
			d = &tab_choice[choice];
			if(!(choice != taken && prediction == taken))
			{
				if(taken){
					if(*d<3) (*d)++;
				}else{
				if(*d>0) (*d)--;
				}
			
			}

	}	
	branch_update *predict (branch_info & b) 
	{
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) 
		{
	//		u.index = (history << (TABLE_BITS - HISTORY_LENGTH)) ^ (b.address & ((1<<TABLE_BITS)-1));
			u.direction_prediction(access_conditional_bp(b.address,history));
		} 
		else 
		{
			u.direction_prediction (true);
		}
		if (b.br_flags & BR_INDIRECT) 
		{
			/* 	
	   	Algorithm 1 of VPC
		*/
		
			int i=0;		//iterator
			u.vghr = history;	//virtual history i.e history of branch
			u.vpca = b.address;	//initally vpca is the PC itself
			bool done = false;
			unsigned int pred_target;
			bool direction;		
			while(!done)
			{
			u.index = u.vpca & ((1<<TABLE_BITS)-1);
			pred_target = targets[u.index];		//get the BTB prediction
			direction = access_conditional_bp(u.vpca,u.vghr);	//will the branch be taken?
			
	if(pred_target && (direction == true))
				{
					u.direction_prediction(true);
					u.target_prediction(pred_target);	//found target!
					u.pred_iter = i;					
					done = true;
				}
				else if((pred_target == 0) || (i >= (MAX_ITER - 1)))	//no hit in BTB or VPC has run too many times
				{
					u.direction_prediction(false);
					u.target_prediction(0);
					done = true;
				}
				u.vpca = hash_func(b.address, hashvals[i]);		//get new vpca
				u.vghr = u.vghr << 1;					//shift virtual history
				u.vghr &= ((1<<HISTORY_LENGTH)-1);
				i++;	 
			}
			
			
			//u.target_prediction (targets[b.address & ((1<<TABLE_BITS)-1)]);
		}
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target) {
		if (bi.br_flags & BR_CONDITIONAL) {
			/*
			unsigned char *c = &tab[((my_update*)u)->index];
			if (taken) {
				if (*c < 3) (*c)++;
			} else {
				if (*c > 0) (*c)--;
			}
			*/
			update_conditional_bp(bi.address,history,taken,u->direction_prediction());
			history <<= 1;
			history |= taken;
			history &= (1<<HISTORY_LENGTH)-1;

		}
		if (bi.br_flags & BR_INDIRECT) {
			//targets[bi.address &  ((1<<TABLE_BITS)-1)] = target;
					
			if(u->target_prediction() == target) 			//did we make the right prediction?
			{
			/* Algorithm 2*/
				int i=0;
				unsigned int vpca = bi.address;
				unsigned int vghr = history; 	
				bool dir;
				do
				{
					if(i == ((my_update*)u)->pred_iter)			
					{
						dir = access_conditional_bp(vpca,vghr);
						targets[vpca &  ((1<<TABLE_BITS)-1)] = target;
						update_conditional_bp(vpca,vghr,1,dir);	//update the perceptron
					}
					else
					{
						dir = access_conditional_bp(vpca,vghr);
						update_conditional_bp(vpca,vghr,0,dir);	//update the perceptron with not-taken
					}
					vpca = hash_func(bi.address, hashvals[i]);
					vghr = vghr << 1;
					vghr &= ((1<<HISTORY_LENGTH)-1);
					i++;
				}while(i<((my_update*)u)->pred_iter);
			}
 			else
			{
			/* Algorithm 3 */
				int i=0;
				unsigned int vpca = bi.address;
				unsigned int vghr = history;
				bool found_target = false;		//check for the right target 
				while((i<=MAX_ITER-1) && (!found_target))
				{	
					unsigned int pred_target = targets[vpca &  ((1<<TABLE_BITS)-1)];
					if(pred_target == target) 	//found the target
					{
						bool dir = access_conditional_bp(vpca,vghr);
						update_conditional_bp(vpca,vghr,1,dir); //this branch was taken
			//			btb_1->put(vpca,target);
						targets[vpca &  ((1<<TABLE_BITS)-1)] = target;	
						found_target = true;
					}
					else if(pred_target != 0 )
					{
						bool dir = access_conditional_bp(vpca,vghr);
						update_conditional_bp(vpca,vghr,0,dir); //this branch was not taken
					}
					vpca = hash_func(bi.address, hashvals[i]);
					vghr = vghr << 1;
					vghr &= ((1<<HISTORY_LENGTH)-1); 
						i++;
				}
				if(!found_target)    				//if target is still not found
				{
			//		btb_1->put(vpca,target);
					targets[((my_update*)u)->index] = target;	
					update_conditional_bp(((my_update*)u)->vpca,((my_update*)u)->vghr,1,u->direction_prediction());
				}
			
			}

		}
	}
};
