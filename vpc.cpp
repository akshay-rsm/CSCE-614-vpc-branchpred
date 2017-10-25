#include <iostream>
#include <map>
#include <assert.h>
#include <cmath>
using namespace std;
#define PERCEPTRON_SIZE 20
#define MAX_ITER 16
#define RANDOM_HASH_CONSTANT 2340
unsigned int hashvals[MAX_ITER]={1392,4695,2615,5174,2648,2810,7302,5494,6865,3519,6155,3489,5263,6911,5501,2558};
int sign(int y)
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
unsigned int hash_func(unsigned int a,unsigned int b,unsigned int c)
{
  a=a-b;  a=a-c;  a=a^(c >> 13);
  b=b-c;  b=b-a;  b=b^(a << 8);
  c=c-a;  c=c-b;  c=c^(b >> 13);
  a=a-b;  a=a-c;  a=a^(c >> 12);
  b=b-c;  b=b-a;  b=b^(a << 16);
  c=c-a;  c=c-b;  c=c^(b >> 5);
  a=a-b;  a=a-c;  a=a^(c >> 3);
  b=b-c;  b=b-a;  b=b^(a << 10);
  c=c-a;  c=c-b;  c=c^(b >> 15);
  return c;
}
int get_history_bit(unsigned int history,unsigned int position)
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
class perceptron_node
{
    public:
    unsigned int vpca;
    int perceptron[PERCEPTRON_SIZE];
    int bias;
    perceptron_node* prev,*next;
    perceptron_node(int k): vpca(k), next(NULL), prev(NULL)
    {
        perceptron[PERCEPTRON_SIZE] = {0};
        bias = 0;
    }
};
class perceptron_list
{
    
    public:
    
    perceptron_node *front,*rear;
    
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
    map<unsigned int,perceptron_node*> p_map;
    
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
    btb_node(unsigned int vpca, unsigned int address ): vpca(vpca), target(address),next(NULL), prev(NULL){}  
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
            return 0;
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
