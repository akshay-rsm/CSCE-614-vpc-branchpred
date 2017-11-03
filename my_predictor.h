// my_predictor.h
// This file contains an implementation of the Virtual Program Counter(VPC) algorithm for indirect branch prediction 
// It also contains an implementation of a bi-mode predictor,as well as a simple direct-mapped Branch Target Buffer containing 2^20 entries. Cheers!

using namespace std;
class my_update : public branch_update {
public:
	unsigned int index;	// BTB index 
	unsigned int vpca;	// The virtual program counter address, passed after making a prediction
	unsigned int vghr;	// The 'virtual' history, for the current indirect branch
	int pred_iter;		// The iteration of VPC at which the prediction was made
};

class my_predictor : public branch_predictor {
public:
#define HISTORY_LENGTH	20	//How far into the history are we looking?
#define TABLE_BITS	20	//Size of tables
#define MAX_ITER 12		//Maximum number of times VPC will run
#define SIZE_OF_BTB 1024	
	my_update u;		//struct to store info about prediction 
	branch_info bi;		//contains information about the branch, such as type, address
	unsigned int history;	//The history register
	unsigned char tab_taken[1<<(TABLE_BITS)];	//The taken predictor
	unsigned char tab_not_taken[1<<(TABLE_BITS)];	//The not-taken predictor
	unsigned char tab_choice[1<<(TABLE_BITS)];	//The preditor which chooses the predictor from which we make our final prediction
	unsigned int targets[1<<TABLE_BITS];		//Branch Target Buffer
	unsigned int hashvals[20];			//Array of randomized hash values
	
		
	my_predictor (void) : history(0), hashvals {1392,4695,2615,5174,2648,2810,7302,5492,6865,3519,6155,3489,5263,6911,5501,2558}	//Constructor for the class 
	{ 
		memset (tab_taken, 0, sizeof (tab_taken));
		memset (tab_not_taken, 0, sizeof (tab_not_taken));
		memset (tab_choice, 0, sizeof (tab_choice));
		memset (targets, 0, sizeof (targets));
	}
	
	unsigned int hash_func(unsigned int address, unsigned int hashvalue) //simple hash function which XORs the branch address with a random value chosen from a hardcoded table of values
	{
		return (address ^ hashvalue);
	}

	bool access_conditional_bp(unsigned int address, unsigned int ghr)	//conditional branch prediction 
	{
			unsigned int choice_index = ((address) & ((1<<TABLE_BITS)-1));	//index into the choice table
			bool choice = (tab_choice[choice_index] >> 1);	//the choice predictor's prediction
			unsigned int common_index = ((address) & ((1<<TABLE_BITS)-1)) ^ ((ghr << (TABLE_BITS - HISTORY_LENGTH)));	//index into the predictor table
			
			if(choice)	
			{
				return (tab_taken[common_index] >> 1);	//choose taken table
			}
			else
			{
				return (tab_not_taken[common_index] >> 1);	//choose not-taken
			}
	}
	void update_conditional_bp(unsigned int address,unsigned int ghr ,bool taken,int prediction)	//update the conditional branch predictor
	{
			unsigned int choice_index = ((address) & ((1<<TABLE_BITS)-1));	//into the table 
			unsigned char *c,*d;	//used to increment/decrement the counters
			bool choice = (tab_choice[choice_index]>>1); 	//the choice predictor's prediction
			unsigned int tab_index = ((address) & ((1<<TABLE_BITS)-1)) ^ ((ghr << (TABLE_BITS - HISTORY_LENGTH)));	//the index for the preditor table
			if(tab_choice[choice_index] >> 1)
			{
				c = &tab_taken[tab_index];	//choose taken table
			}
			else
			{
				c = &tab_not_taken[tab_index];	//choose not-taken table
			}

			if(taken)
			{
				if (*c < 3) (*c)++;		//if the counter hasn't saturated, increment
			}else
			{
				if (*c > 0) (*c)--;		//if the counter hasn't saturated, decrement
			}	
			
			d = &tab_choice[choice_index];		//the choice predictor entry
			if(!(choice != taken && prediction == taken))		//the case when the choice predictor chose wrongly, but the counter in the prediction table gave the right prediction, we don't update the counter
			{
				if(taken)			
				{
					if(*d<3) (*d)++;
				}else
				{
					if(*d>0) (*d)--;
				}
			
			}

	}	
	branch_update *predict (branch_info & b) 		//the predict algorithm
	{
		bi = b;
		if (b.br_flags & BR_CONDITIONAL) 
		{
			u.direction_prediction(access_conditional_bp(b.address,history));		//make prediction if conditional branch
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
			bool done = false;	//if prediction is done
			unsigned int pred_target;	//the btb's prediction
			bool direction;			//the cbp's prediction
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
					u.target_prediction(0);				//give no prediction
					done = true;
				}
				u.vpca = hash_func(b.address, hashvals[i]);		//get new vpca
				u.vghr = u.vghr << 1;					//shift virtual history
				u.vghr &= ((1<<HISTORY_LENGTH)-1);
				i++;	 
			}
	
		}
		return &u;
	}

	void update (branch_update *u, bool taken, unsigned int target)
	{
		if (bi.br_flags & BR_CONDITIONAL)
		{
			update_conditional_bp(bi.address,history,taken,u->direction_prediction());
			history <<= 1;
			history |= taken;
			history &= (1<<HISTORY_LENGTH)-1;

		}
		if (bi.br_flags & BR_INDIRECT)
		{
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
						dir = access_conditional_bp(vpca,vghr);	//access the conditional branch predictor again
						targets[vpca &  ((1<<TABLE_BITS)-1)] = target;	//update the btb
						update_conditional_bp(vpca,vghr,1,dir);	//update the conditional branch predictor as taken
					}
					else
					{
						dir = access_conditional_bp(vpca,vghr); 
						update_conditional_bp(vpca,vghr,0,dir);	//update the cbp as not-taken
					}
					vpca = hash_func(bi.address, hashvals[i]);
					vghr = vghr << 1;
					vghr &= ((1<<HISTORY_LENGTH)-1);
					i++;
				}while(i<((my_update*)u)->pred_iter);	//iterate until we reach the iteration at which prediction is made
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
						targets[vpca &  ((1<<TABLE_BITS)-1)] = target;	//update btb
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
					targets[((my_update*)u)->index] = target;	//update btb at the stored index
					update_conditional_bp(((my_update*)u)->vpca,((my_update*)u)->vghr,1,u->direction_prediction());
				}
			
			}

		}
	}
};
