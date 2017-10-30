// my_predictor.h
// simple direct-mapped branch target buffer for indirect branch prediction.
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
    
    perceptron_node *front,*rear; //
    
    perceptron_list(): front(NULL), rear(NULL){}
    
    bool no_entries()
    {
        if(rear == NULL)
        {
            return true;
        }
        return false;
    }
    
    
    perceptron_node* add_to_head(unsigned int vpca)
    {
        
        perceptron_node* page = new perceptron_node(vpca);
        if(!front && !rear)
        {
            front = rear = page;
        }
        else
        {
            page->next = front;
            page->prev = NULL;
            front->prev = page;
            front = page;
        }
        return page;
    }
    
    void move_page_to_head(perceptron_node* page)
    {
        if(page==front)
        {
            return;
        }
        if(page==rear)
        {
            rear = rear->prev;
            rear->next = NULL;
        }
        else
        {
            page->prev->next = page->next;
            page->next->prev = page->prev;
        }
    
        page->next = front;
        page->prev = NULL;
        front->prev = page;
        front = page;
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
class perceptron_table
{
    public:
    unsigned  size, capacity;
    perceptron_list *p_list;
    std::map<unsigned int,perceptron_node*> p_map;
    
    perceptron_table(unsigned int cap)
    {
        this->capacity = cap;
        size=0;
        p_list = new perceptron_list();
        p_map = map<unsigned int, perceptron_node*>();
    }
    
    perceptron_node* get(unsigned int vpca)
    {
        if(p_map.find(vpca) == p_map.end()){
            return NULL;
        }
        //unsigned int* p_vector = p_map[vpca]->perceptron;
        p_list->move_page_to_head(p_map[vpca]);
        return p_map[vpca];
    }
    
    void put(unsigned int vpca, int* vec)
    {
        if(vec == NULL)
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
            p_map[vpca] = perp;
            return;
        }
        else if(p_map.find(vpca) != p_map.end())
        {
            for(int i=0;i<PERCEPTRON_SIZE;i++)
            {
                p_map[vpca]->perceptron[i]=vec[i];
            }
            p_list->move_page_to_head(p_map[vpca]);
            return;
        }
        else{
            assert(false);
        }
        
    }

};

void update_bias(perceptron_node* p_node,int taken)
{
    int new_bias = p_node->bias + taken;
    if(abs(new_bias) < 128 )
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
        if(abs(new_weight) < 128)
        {
            p_node->perceptron[i] = new_weight;
        }
    }
}
class perceptron_predictor
{
    public:
    perceptron_table* p_table;
    static const int theta=40;
    unsigned int size_of_table;
    perceptron_predictor(unsigned int size)
    {
        this->size_of_table = size;
        p_table = new perceptron_table(size);
    }
    int compute(unsigned int vpca, unsigned int vghr)
    {
        perceptron_node* p_node = p_table->get(vpca);
        int y;
        if(p_node == NULL)
        {
            p_table->put(vpca,NULL);
            p_node = p_table->get(vpca);
        }
        y = p_node->bias;
        for(int i = 0;i < PERCEPTRON_SIZE;i++)
        {
            y+=(p_node->perceptron[i] * get_history_bit(vghr,i)); 
        }
        return y;
    }
    
    void update(unsigned int vpca, int result, int taken, unsigned int vghr)
    {
        perceptron_node* p_node = p_table->get(vpca);
       	if(p_node == NULL)
	{
		p_table->put(vpca,NULL);
		p_node = p_table->get(vpca);
	}
	 
        if(sign(result) != taken || abs(result) <= theta)
        {
            update_bias(p_node,taken);
            update_weights(p_node,taken,vghr);
        }
    }
    
};
class btb_node
{
    public:
    unsigned int vpca;
    unsigned int target;
    btb_node* prev,*next;
    btb_node(unsigned int vpca, unsigned int address ): vpca(vpca), target(address),prev(NULL), next(NULL){}  
};
class btb_list
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
        
        btb_node* page = new btb_node(vpca,address);
        if(!front && !rear)
        {
            front = rear = page;
        }
        else
        {
            page->next = front;
            page->prev = NULL;
            front->prev = page;
            front = page;
        }
        return page;
    }
    
    void move_page_to_head(btb_node* page)
    {
        if(page==front)
        {
            return;
        }
        if(page==rear)
        {
            rear = rear->prev;
            rear->next = NULL;
        }
        else
        {
            page->prev->next = page->next;
            page->next->prev = page->prev;
        }
    
        page->next = front;
        page->prev = NULL;
        front->prev = page;
        front = page;
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
class btb
{
    public:
    unsigned  size, capacity;
    btb_list *b_list;
    map<unsigned int,btb_node*> b_map;
    
    btb(unsigned int cap)
    {
        this->capacity = cap;
        size=0;
        b_list = new btb_list();
        b_map = map<unsigned int, btb_node*>();
    }
    
    unsigned int get(unsigned int vpca)
    {
        if(b_map.find(vpca) == b_map.end()){
            return -1;
        }
        //unsigned int* p_vector = p_map[vpca]->perceptron;
        b_list->move_page_to_head(b_map[vpca]);
        return b_map[vpca]->target;
    }
    
    void put(unsigned int vpca, unsigned int address)
    {
        if(b_map.find(vpca) != b_map.end())
        {
            b_map[vpca]->target = address;
            b_list->move_page_to_head(b_map[vpca]);
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
#define HISTORY_LENGTH	20
#define NO_OF_PERCEPTRONS 64
#define SIZE_OF_BTB 64
#define MAX_ITER 16
	my_update u;
	branch_info bi;
	unsigned int ghr;	
	unsigned int hashvals[MAX_ITER];
	int result_of_perceptron[MAX_ITER];
	unsigned int pred_target;
	int pred_iter;
	perceptron_predictor* p_pred;
	btb* btb_1;

	my_predictor (void) : ghr(0),hashvals {1392,4695,2615,5174,2648,2810,7302,5494,6865,3519,6155,3489,5263,6911,5501,2558}
	{ 
		p_pred = new perceptron_predictor(NO_OF_PERCEPTRONS);
		btb_1 = new btb(SIZE_OF_BTB);
	}

	branch_update *predict (branch_info & b) 
	{
	/* 	1)Create two objects of type Perceptron_Tab and BTB
	   	2)Algorithm 1 of VPC
	*/
		bi = b;
		if(b.br_flags & BR_INDIRECT)
		{
			int i=0;
			unsigned int vghr = ghr;
			unsigned int vpca = b.address;
			bool done = false;
			
			do
			{
				pred_target = btb_1->get(vpca);
				result_of_perceptron[i] = p_pred->compute(vpca,vghr);
				bool direction = result_of_perceptron[i]>=0 ? true:false;
				if(pred_target && (direction == true))
				{
					u.target_prediction(pred_target);
					done = true;
					i = i-1;
				}
				else if((!pred_target) || (i >= (MAX_ITER - 1)))
				{
					u.target_prediction(0);
					done = true;
					i=i-1;
				}
			vpca = hash_func(b.address, hashvals[i]);
			vghr = vghr << 1;
			i++;	 
			}while(!done);
			
			pred_iter = i; 
		}
		if(b.br_flags & BR_CONDITIONAL)
		{
			u.direction_prediction(true);
		}
		return &u;
		
	}

	void update (branch_update *u, bool taken, unsigned int target)
	{
		bool correct_prediction = false;
		if(bi.br_flags & BR_INDIRECT)
		{
			if(u->target_prediction() == target)
			{
				correct_prediction = true;
			}
			if(correct_prediction)
			{
				int i=0;
				unsigned int vpca = bi.address;
				unsigned int vghr = ghr; 	
				do
				{
					if(i == pred_iter)
					{
						btb_1->put(vpca,target);
						p_pred->update(vpca,result_of_perceptron[i],1,vghr);
					}
					else
					{
						p_pred->update(vpca,result_of_perceptron[i],-1,vghr);
					}
					vpca = hash_func(bi.address, hashvals[i]);
					vghr = vghr << 1;
					i++;
				}while(i<pred_iter);
			}
 			else
			{
				int i=0;
				unsigned int vpca = bi.address;
				unsigned int vghr = ghr;
				bool found_target = false;
				do
				{	
					pred_target = btb_1->get(vpca);
					//printf("%u",pred_target);
					if(pred_target == target)
					{
						p_pred->update(vpca,result_of_perceptron[i],+1,vghr);
						btb_1->put(vpca,target);
						found_target = true;
					}
					else if(pred_target)
					{
						p_pred->update(vpca,result_of_perceptron[i],-1,vghr);
					}
					vpca = hash_func(bi.address, hashvals[i]);
					vghr = vghr << 1; 										 i++;
				}while((i<=MAX_ITER-1) && (!found_target));
				if(!found_target)
				{
					int take;
					if(taken)
					{
						take = 1;
					}
					else
					{
						take = -1;
					}
					unsigned int vpca = bi.address;
					unsigned int vghr = ghr;
					btb_1->put(vpca,target);
					p_pred->update(vpca,0,take,vghr);
				}
			
			}
		}
		if(bi.br_flags & BR_CONDITIONAL)
		{
			ghr <<= 1;
			ghr |= taken;
			ghr &= (1<<HISTORY_LENGTH)-1;
		}	
	}
};

