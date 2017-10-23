#include <iostream>
#include <map>
#include <assert.h>
using namespace std;
#define PERCEPTRON_SIZE 20

unsigned int sign(unsigned int y)
    {
        int sign =(y>0) - (y<0);
        return sign;
    }

class perceptron_node
{
    public:
    unsigned int vpca;
    unsigned int perceptron[PERCEPTRON_SIZE] ;
    perceptron_node* prev,*next;
    perceptron_node(unsigned int k): vpca(k), next(NULL), prev(NULL)
    {
        perceptron[PERCEPTRON_SIZE] = {0};
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
    
    unsigned int* get(unsigned int vpca)
    {
        if(p_map.find(vpca) == p_map.end()){
            return NULL;
        }
        unsigned int* p_vector = p_map[vpca]->perceptron;
        p_list->move_page_to_head(p_map[vpca]);
        return p_vector;
    }
    
    void put(unsigned int vpca, unsigned int* vec)
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
int main(){
    perceptron_table p(5);
    unsigned int arr[PERCEPTRON_SIZE] = {1,2};
    p.put(21,arr);
    unsigned int* vec = p.get(21);
    for(int i=0;i<PERCEPTRON_SIZE;i++)
    {
        cout<<vec[i]<<" ";
    }
    perceptron_node* hed =p.p_list->get_front();
    cout<<hed->vpca<<"\n";
    vec = p.get(23);
    for(int i=0;i<PERCEPTRON_SIZE;i++)
    {
        cout<<vec[i]<<" ";
    }
    hed =p.p_list->get_front();
    cout<<hed->vpca<<"\n";
    vec = p.get(12);
    for(int i=0;i<PERCEPTRON_SIZE;i++)
    {
        cout<<vec[i]<<" ";
    }
    hed =p.p_list->get_front();
    cout<<hed->vpca<<"\n";
    
    return 0;
}

unsigned int hash(unsigned int address,unsigned int value)
{
    unsigned int result;
    result = (1/2)*(address + value)*(address + value + 1) + value;
    return result;
}

