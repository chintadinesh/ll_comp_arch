#include <stdio.h>
#include <stdlib.h>

#define CORE 1
int * InFIFO = (int *) 0x1234;
int * OutFIFO = (int *) 0x2345;
int * OutStatus = (int *) 0x3456;

// subroutine declaration for operating on InFIFO and OutFIFO
void enqueue(int* f, int data);
int dequeue(int* f);

#define GET_SRC(msg)\
    (msg.word1 & 0xFFFF)

#define GET_MSGLEN(msg) \
    (msg.word2)

typedef struct msg {
    int word1;
    int word2;
    int word3;
    int word4;
    int word5;
} msg_t;

typedef struct ll_node{
    struct ll_node* next;
    msg_t* msg;
} ll_node_t;

typedef struct msg_bank msg_bank_t;

struct msg_bank {
    int src;
    struct msg_bank* next;
    ll_node_t* head; 

};

ll_node_t* _get_msg(msg_bank_t* bank){
    if(bank->head == NULL){
        // TODO: remove the node from the bank
        return NULL;
    }
    else{
        ll_node_t* ret_node = bank->head;
        bank->head = bank->head->next;    
        return ret_node; 
    }
}

static msg_bank_t* global_msg_bank;

ll_node_t* search_msg_bank(int src){
    msg_bank_t* m_iter = global_msg_bank;

    while(m_iter != NULL){
        if(m_iter->src == src)
            return _get_msg(m_iter);
        else
            m_iter = m_iter->next;
    }
    
    return NULL;
}

void _add_to_tail(msg_bank_t* m_iter, msg_t m){
    ll_node_t* n = m_iter->head;
    ll_node_t* prev = NULL;

    while(n != NULL){
        prev = n;
        n = n->next;
    }

    ll_node_t* new_ll = (ll_node_t*) malloc(sizeof(ll_node_t));
    new_ll->msg = &m;
    prev->next = new_ll;
}

void add_to_bank(msg_t m){

    msg_bank_t* m_iter = global_msg_bank;

    while(m_iter != NULL){
        if(m_iter->src == GET_SRC(m)){
            _add_to_tail(m_iter, m);
        }
        else
            m_iter = m_iter->next;
    }

    // no node is found in the bank for the src node
    // create and add to the bank
    msg_bank_t* new_bank_node = (msg_bank_t *) malloc(sizeof(msg_bank_t));
    new_bank_node->next = global_msg_bank;
    global_msg_bank = new_bank_node;

    ll_node_t* new_ll_node = (ll_node_t *)malloc(sizeof(ll_node_t));
    new_ll_node->next = NULL;
    new_ll_node->msg = &m;

    return;
}

msg_t __recieve_one_msg(){
    msg_t m;

    // here we can sleep if there is no correct input
    // and wake up on a signal from OS when a new message arrives
	do{
        int word1 = dequeue(InFIFO); 
        if (word1 & 0x80000000){
            // we assume that we are polling for the input to be valid
            m.word1 = word1;
            m.word2 =  dequeue(InFIFO); 
            m.word3 =  dequeue(InFIFO); 
            m.word4 =  dequeue(InFIFO); 
            m.word5 =  dequeue(InFIFO); 
            return m;
        }
	} while(1);
}

//receives one word from the source node “src”)
int receiveMsg(int src) {
    // first check if a message exists in message_bank
    ll_node_t*ll = search_msg_bank(src);
    static msg_t msg;
    static int current_read = 0;

    if (current_read == GET_MSGLEN(msg)){ // need to read a new msg
        if(ll == NULL){ // no pre existing messages in the bank
            do {
                msg  = __recieve_one_msg(); // extract one msg from InFIFO
                add_to_bank(msg); // add it to the bank
            } while(GET_SRC(msg) !=  src); // search until a msg is rcvd 
            
            ll = search_msg_bank(src); // remove the msg from the bank
            msg = *(ll->msg);
        }
        else{ // pre existing msg in the bank
            msg = *(ll->msg);
        }

        current_read = 0; // reset the # words read
    }

    int ret_val = (current_read == 0) ? msg.word3
                    : (current_read == 1) ? msg.word4
                    : msg.word5;

    current_read++;
    return ret_val;
}

extern int* TIMER;

#define RESET_MSG(dest,data)\
    pre_dest = dest; word3 = data; SET_TIMER()

// microseconds
#define TIME_CONSTANT 1
#define SET_TIMER()\
    *TIMER = TIME_CONSTANT

static int word1, word3, word4, word5;
static int curr_num_words = 0, pre_dest = 0;

int _send_curr_msg() {
    // check if the msg can be sent
    if(! *OutStatus) return 0;

    word1 = 0x80000000 | (pre_dest << 16) | CORE;
    enqueue(OutFIFO, word1); enqueue(OutFIFO, curr_num_words);enqueue(OutFIFO, word3);
    enqueue(OutFIFO, word4); enqueue(OutFIFO, word5);

    curr_num_words = 0;
    return 1; // return successful msg sent
}

void timer_interrupt_handler(){
    _send_curr_msg();
}

int sendMsg(int dest, int data) {
    if((curr_num_words > 0) && (curr_num_words < 3)){ // previous words available
            if(pre_dest == dest){ // adding to previous words
                curr_num_words++;
                switch(curr_num_words){
                    case 1: // save the word for later
                        word3 = data; SET_TIMER();
                        return 1;
                    case 2: //save the word for later
                        word4 = data; SET_TIMER();
                        return 1;
                    case 3: // send the msg
                        word5 = data;
                        return _send_curr_msg();

                    default: // never taken
                        return 1;
                }
            }
            else{ // curr dest different from prev msgs
                // send the previous message
                _send_curr_msg();
                RESET_MSG(dest,data);// form a new message now
                return 1;
            }
    }
    else{ // curr_num_words ==  0
        RESET_MSG(dest,data);// form a new message now
        return 1;
    }
}

