/***my_predictor.h*****/

#include <map>
#include <assert.h>
#include <cmath>
#define PERCEPTRON_SIZE 20  //size of each perceptron. This is the number of weights within each perceptron
using namespace std;

int sign(int y) //function to find the sign of an integer. Returns 1 or -1 based on sign
{
    int sign;
    if(y >= 0)
    {
        sign = 1;
    }
    else
    {
        sign = -1;
    }
    return sign;
}

unsigned int hash_func(unsigned int address, unsigned int hashvalue) //simple hash function which XORs the branch address with a random value chosen from a hardcoded table of values
{
	return (address ^ hashvalue);
}

int get_history_bit(unsigned int history,unsigned int position) //function to return a particular bit of the history register. The output is bipolar, i.e the function returns 1 if the bit is 1, or -1 if the bit is 0. 
{
    int y = history;
    y = y & (1<<position);
    y = y >> position;
    y &= 1;
    if(y == 0)
    {
        y = -1;
    }
    return y;
}

class perceptron_node //The fundamental data unit of the perceptron branch predictor.
{
    public:
    unsigned int vpca; //The address associated with each perceptron
    int perceptron[PERCEPTRON_SIZE]; //The array of weights, i.e the correlations between branch history and current behavior
    int bias; //w_0 or the initial assumption/prediction that the perceptron makes about the branch 
    perceptron_node* next; //pointer to the next node 
    perceptron_node* prev; //pointer to the previous node
    perceptron_node(int k): vpca(k), perceptron {0}, next(NULL), prev (NULL) //constructor
    {
        
        bias = 0; //initially the bias is towards predicting as taken
    }
};
class perceptron_list //Doubly linked list to hold perceptrons
{
    
    public:
    
    perceptron_node *front,*rear; // The first and last nodes of the  linked list
    
    perceptron_list(): front(NULL), rear(NULL){} //Constructor
    
    bool no_entries() //Check if there are no entries in the linked list
    {
        if(rear == NULL)
        {
            return true;
        }
        return false;
    }
    
    
    perceptron_node* add_to_head(unsigned int vpca) //adds a perceptron node to the head of the linked list
    {
        
        perceptron_node* node = new perceptron_node(vpca); //get a new node
        if(!front && !rear)
        {
            front = rear = node; //if there are no entries in the linked list
        }
        else
        {
            node->next = front;
            node->prev = NULL;
            front->prev = node;
            front = node;
        }
        return node;
    }
    
    void move_node_to_head(perceptron_node* node) //a key operation required for implementing an LRU policy 
    {
        if(node==front)
        {
            return;
        }
        if(node==rear)
        {
            rear = rear->prev;
            rear->next = NULL;
        }
        else
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
    
        node->next = front;
        node->prev = NULL;
        front->prev = node;
        front = node;
    }
    
    void remove_rear() //remove the last node
    {
        if(no_entries())
        {
            return;
        }
        if(front == rear)
        {
            delete rear;
            front = NULL;
            rear = NULL;
        }
        else
        {
            perceptron_node* temp = rear;
            rear = rear->prev;
            rear->next = NULL;
            delete temp;
        }
    }
    perceptron_node* get_rear()
    {
        return rear;
    }
    perceptron_node* get_front()
    {
        return front;
    }
};
class perceptron_table //the map which keeps track of the various nodes and their mapping to various branch addresses
{
    public:
    unsigned  size, capacity; //current size and maximum capacity
    perceptron_list *p_list; //pointer to linked list  of perceptron nodes
    std::map<unsigned int,perceptron_node*> p_map; // unordered map to keep track of addresse and perceptron nodes 
    
    perceptron_table(unsigned int cap) // constructor
    {
        this->capacity = cap;
        size=0;
        p_list = new perceptron_list();
        p_map = map<unsigned int, perceptron_node*>();
    }
    
    perceptron_node* get(unsigned int vpca) //returns the perceptron node associated with a particular vpca 
    {
        if(p_map.find(vpca) == p_map.end()){
            return NULL;		//return NULL if node doesn't exist
        }
        
        p_list->move_node_to_head(p_map[vpca]); // this node just became the most used
        return p_map[vpca];
    }
    
    void put(unsigned int vpca, int* vec)
    {
        if(vec == NULL)				//this means that the node hasn't been created yet, so we get a new one 
        {
            if(size == capacity)
            {
                unsigned int v = p_list->get_rear()->vpca;
                p_map.erase(v);
                p_list->remove_rear();
                size--;
            }
            perceptron_node* perp = p_list->add_to_head(vpca);
            size++;
            p_map[vpca] = perp; //map the new node to the address
            return;
        }
        else if(p_map.find(vpca) != p_map.end()) //the node already exists, enter new values into existing perceptron
        {
            for(int i=0;i<PERCEPTRON_SIZE;i++)
            {
                p_map[vpca]->perceptron[i]=vec[i];
            }
            p_list->move_node_to_head(p_map[vpca]);
            return;
        }
        else{
            assert(false);
        }
        
    }

};

class perceptron_predictor //the predictor class
{
    public:
    perceptron_table* p_table; // perceptron table
    static const int theta=40; // learning threshold
    unsigned int size_of_table; //the number of perceptrons
    perceptron_predictor(unsigned int size)
    {
        this->size_of_table = size;
        p_table = new perceptron_table(size);
    }
    int compute(unsigned int vpca, unsigned int vghr) // the computation function, returns an integer result
    {							// y = dotproduct(w, history) + bias
        perceptron_node* p_node = p_table->get(vpca); //get the relevant perceptron node
        int y;
        if(p_node == NULL)			     //if node doesn't exist
        {
            p_table->put(vpca,NULL);
            p_node = p_table->get(vpca);
        }
        y = p_node->bias;			//add bias(w0)
        for(int i = 0;i < PERCEPTRON_SIZE;i++)
        {
            y+=(p_node->perceptron[i] * get_history_bit(vghr,i)); 
        }
        return y; //result of computation
    }
    
    void update(unsigned int vpca, int result, int taken, unsigned int vghr)
    {
        perceptron_node* p_node = p_table->get(vpca); //get the required node
       	if(p_node == NULL)			//if it doesn't exist
	{
		p_table->put(vpca,NULL);
		p_node = p_table->get(vpca);
	}
	 
        if(sign(result) != taken || abs(result) <= theta) //if the result of computation and the actual outcome don't match or if the result is lesser than the learning threshold 
        {
            update_bias(p_node,taken);		
            update_weights(p_node,taken,vghr);
        }
    }
    void update_bias(perceptron_node* p_node,int taken)
    {
    	int new_bias = p_node->bias + taken;
    	if(new_bias < 7 && new_bias > 0 )  //each weight is a saturating three-bit counter
    	{
       		p_node->bias = new_bias;
    	}
    }
    void update_weights(perceptron_node* p_node,int taken, unsigned int vghr)
    {
    	int new_weight;
    	for(int i=0;i<PERCEPTRON_SIZE;i++)
    	{
        	new_weight = p_node->perceptron[i] + (taken * get_history_bit(vghr, i)); 
        	if(new_weight < 7 && new_weight > 0)
        	{
            		p_node->perceptron[i] = new_weight;
        	}
    	}
    }
    
};
class btb_node					//basic node of Branch Target Buffer
{
    public:
    unsigned int vpca;				//branch address
    unsigned int target;			//predicted target
    btb_node* prev,*next;
    btb_node(unsigned int vpca, unsigned int address ): vpca(vpca), target(address),prev(NULL), next(NULL){}  
};
class btb_list					//Doubly linked list of BTB nodes
{
    
    public:
    btb_node *front,*rear;
    btb_list(): front(NULL), rear(NULL){}
    
    bool no_entries()
    {
        if(rear == NULL)
        {
            return true;
        }
        return false;
    }
    
    
    btb_node* add_to_head(unsigned int vpca, unsigned int address)
    {
        
        btb_node* node = new btb_node(vpca,address);
        if(!front && !rear)
        {
            front = rear = node;
        }
        else
        {
            node->next = front;
            node->prev = NULL;
            front->prev = node;
            front = node;
        }
        return node;
    }
    
    void move_node_to_head(btb_node* node)
    {
        if(node==front)
        {
            return;
        }
        if(node==rear)
        {
            rear = rear->prev;
            rear->next = NULL;
        }
        else
        {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
    
        node->next = front;
        node->prev = NULL;
        front->prev = node;
        front = node;
    }
    
    void remove_rear()
    {
        if(no_entries())
        {
            return;
        }
        if(front == rear)
        {
            delete rear;
            front = NULL;
            rear = NULL;
        }
        else
        {
            btb_node* temp = rear;
            rear = rear->prev;
            rear->next = NULL;
            delete temp;
        }
    }
    btb_node* get_rear()
    {
        return rear;
    }
    btb_node* get_front()
    {
        return front;
    }
};
class btb //BTB class. Uses unordered map and linked list to implement an LRU cache.
{
    public:
    unsigned  size, capacity;
    btb_list *b_list; //linked list
    map<unsigned int,btb_node*> b_map; //the unordered map
    
    btb(unsigned int cap)
    {
        this->capacity = cap; //set capacity of BTB ( no. of entries)
        size=0;
        b_list = new btb_list();
        b_map = map<unsigned int, btb_node*>();
    }
    
    unsigned int get(unsigned int vpca)   //get the target address
    {
        if(b_map.find(vpca) == b_map.end()){
            return 0;
        }
        b_list->move_node_to_head(b_map[vpca]);
        return b_map[vpca]->target;
    }
    
    void put(unsigned int vpca, unsigned int address) //insert value into BTB
    {
        if(b_map.find(vpca) != b_map.end())
        {
            b_map[vpca]->target = address;
            b_list->move_node_to_head(b_map[vpca]);
            return;
        }
        if(size == capacity)
        {
            unsigned int v = b_list->get_rear()->vpca;
            b_map.erase(v);
            b_list->remove_rear();
            size--;
        }
        btb_node* btb_temp = b_list->add_to_head(vpca,address);
        size++;
        b_map[vpca] = btb_temp;
        return;
        
    }

};

class my_update : public branch_update
{
public:
	unsigned int index;
};

class my_predictor : public branch_predictor 
{
public:
#define HISTORY_LENGTH	20 //how far back into history are we looking?
#define NO_OF_PERCEPTRONS 1024 //the number of perceptrons
#define SIZE_OF_BTB 1024 //size of the BTB
#define MAX_ITER 8  //the number of iterations of VPC
	my_update u;
	branch_info bi;
	perceptron_predictor* p_pred; //predictor object
	btb* btb_1; //BTB object
	unsigned int ghr;	//global history register
	unsigned int hashvals[MAX_ITER];
	int result_of_perceptron[MAX_ITER];
	unsigned int pred_target; //the predicted target of BTB
	int pred_iter; // the iteration at which prediction is made
	
	my_predictor (void) : ghr(0),hashvals {1392,4695,2615,5174,2648,2810,7302,5494}//,6865,3519,6155,3489,5263,6911,5501,2558}
	{ 
		p_pred = new perceptron_predictor(NO_OF_PERCEPTRONS);
		btb_1 = new btb(SIZE_OF_BTB);
	}

	branch_update *predict (branch_info & b) 
	{
	/* 	
	   	Algorithm 1 of VPC
	*/
		bi = b;
		if(b.br_flags & BR_INDIRECT)   //for indirect branches only
		{
			int i=0;		//iterator
			unsigned int vghr = ghr;	//virtual history i.e history of branch
			unsigned int vpca = b.address;	//initally vpca is the PC itself
			bool done = false;		
			
			do
			{
				pred_target = btb_1->get(vpca);		//get the BTB prediction
				result_of_perceptron[i] = p_pred->compute(vpca,vghr);	//compute the result and store in an array
				bool direction = result_of_perceptron[i]>=0 ? true:false; //will the branch be taken?
				if(pred_target && (direction == true))
				{
					u.target_prediction(pred_target);	//found target!
					done = true;
					i = i-1;
				}
				else if((!pred_target) || (i >= (MAX_ITER - 1)))	//no hit in BTB or VPC has run too many times
				{
					u.target_prediction(0);
					done = true;
					i=i-1;
				}
			vpca = hash_func(b.address, hashvals[i]);		//get new vpca
			vghr = vghr << 1;					//shift virtual history
			i++;	 
			}while(!done);
			
			pred_iter = i; 						//get the iteration at which prediction was made
		}
		if(b.br_flags & BR_CONDITIONAL)
		{
			u.direction_prediction(true);
		}
		return &u;
		
	}

	void update (branch_update *u, bool taken, unsigned int target)
	{
	/* algorithm 2 & 3 */
		bool correct_prediction = false;
		if(bi.br_flags & BR_INDIRECT)
		{
			if(u->target_prediction() == target) 			//did we make the right prediction?
			{
				correct_prediction = true;
			}
			if(correct_prediction)
			{
			/* Algorithm 2*/
				int i=0;
				unsigned int vpca = bi.address;
				unsigned int vghr = ghr; 	
				do
				{
					if(i == pred_iter)			
					{
						btb_1->put(vpca,target);		//update the BTB
						p_pred->update(vpca,result_of_perceptron[i],1,vghr);	//update the perceptron
					}
					else
					{
						p_pred->update(vpca,result_of_perceptron[i],-1,vghr);	//update the perceptron with not-taken
					}
					vpca = hash_func(bi.address, hashvals[i]);
					vghr = vghr << 1;
					i++;
				}while(i<pred_iter);
			}
 			else
			{
			/* Algorithm 3 */
				int i=0;
				unsigned int vpca = bi.address;
				unsigned int vghr = ghr;
				bool found_target = false;		//check for the right target 
				do
				{	
					pred_target = btb_1->get(vpca);
					if(pred_target == target) 	//found the target
					{
						p_pred->update(vpca,result_of_perceptron[i],+1,vghr); //this branch was taken
						btb_1->put(vpca,target);
						found_target = true;
					}
					else if(pred_target)
					{
						p_pred->update(vpca,result_of_perceptron[i],-1,vghr); //this branch was not taken
					}
					vpca = hash_func(bi.address, hashvals[i]);
					vghr = vghr << 1; 									
					i++;
				}while((i<=MAX_ITER-1) && (!found_target));
				if(!found_target)    				//if target is still not found
				{
					//int take = taken? 1:-1;							
					unsigned int vpca = bi.address;
					unsigned int vghr = ghr;
					btb_1->put(vpca,target);
					p_pred->update(vpca,0,+1,vghr);
				}
			
			}
		}
		if(bi.br_flags & BR_CONDITIONAL)
		{
			ghr <<= 1; //shift history
			ghr |= taken;
			ghr &= (1<<HISTORY_LENGTH)-1;
		}	
	}
};

