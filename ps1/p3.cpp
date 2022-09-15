// HW access variables and constants
#define CORE 1
int * InFIFO = (int *) 0x1234;
int * OutFIFO = (int *) 0x2345;
int * OutStatus = (int *) 0x3456; 

// subroutine declaration for operating on InFIFO and OutFIFO
void enqueue(int* f, int data);
int dequeue(int* f);

int sendMsg(int dest, int data)
{
    // check if the msg can be sent
    if(! *OutStatus) return 0;

    enqueue(OutFIFO, 0x80000000 | (dest << 16) | CORE);
    enqueue(OutFIFO, data);

    enqueue(OutFIFO, -1); enqueue(OutFIFO, -1); enqueue(OutFIFO, -1); // dummy words

    return 1; // return successful msg sent
}

int resendMsg(int dest, int data, int src)
{
    // check if the msg can be sent
    if(! *OutStatus) return 0;

    enqueue(OutFIFO, 0x80000000 | (dest << 16) | src);
    enqueue(OutFIFO, data);

    enqueue(OutFIFO, -1); enqueue(OutFIFO, -1); enqueue(OutFIFO, -1); // dummy words

    return 1; // return successful msg sent
}

//receives one word from the source node “src”)
int receiveMsg(int src)  
{
	do{
        int word1 = dequeue(InFIFO); 
        if (word1 & 0x80000000){ // check if the msg is valid

            // extract the rest of the message
            int word2 = dequeue(InFIFO);
            dequeue(InFIFO); dequeue(InFIFO); dequeue(InFIFO); // discard

            // check if the dst matches
            if((word1 & 0xFFFF) != src){
                return word2;
            }
            else{
                while(!resendMsg(word1 & 0xFFFF, word2, word1 & 0x7FFF)); // push to the back of the InFifo
            }
        }
	} while(1);
}


