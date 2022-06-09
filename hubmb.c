/*
 ============================================================================
 Name        : hbmbnet.c
 Author      : Reyyan Oflaz
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>


#define CLIENT_ID_SIZE 	2
#define CLIENT_IP_ADDRESS_SIZE	 8
#define CLIENT_MAC_ADDRESS_SIZE 	11

#define CLIENT_INCOMIN_PORT_NUMBER_SIZE	 5
#define CLIENT_OUTGOING_PORT_NUMBER_SIZE	 5
#define CLIENT_MAX_DATA_SIZE 11

#define LINE_SIZE 30
#define CLIENT_NUMBER_OF_ARGUMENT 3

typedef enum client_data_types_t{
  ENUM_NETWORK_LAYER_APPLICATION,
  ENUM_NETWORK_LAYER_TRANSPORT,
  ENUM_NETWORK_LAYER_NETWORK,
  ENUM_NETWORK_LAYER_PHYSICAL

} client_data_types ;

typedef enum  bool_t{ ENUM_FALSE, ENUM_TRUE } bool;
/*
 * STACK
 */
typedef struct stack_t{
    char senderData[CLIENT_MAX_DATA_SIZE];
    char receiverData[CLIENT_MAX_DATA_SIZE];
    char messageChunk[50];
    client_data_types type;
	struct stack_t *next;
}Stack;

/*
 * QUEUE
 */
//typedef struct queue_t{
//	Stack *q_stack;
//	struct queue_t *next;
//}Queue;
typedef struct queue_t{

	Stack **q_buffer;			//pointer to address of a struct Stack
	int rear;
	int front;
	int capacity;
	int size;
}Queue;
//
//typedef struct buffer_t{
//	Queue *rear;
//	Queue *front;
//}Buffer;


typedef struct Client{
	char clientID[CLIENT_ID_SIZE];						//fixed size	1			//strcmp,strcpy
	char clientIPAddress[CLIENT_IP_ADDRESS_SIZE];		//fixed size	7
	char clientMACAddress[CLIENT_MAC_ADDRESS_SIZE];		//fixed size	10
	char **routingPair;				//malloc ile allocate et
	Queue *inQueue;
	Queue *outQueue;
}Client;


typedef struct Network{
	int numberOfClients;
	Client *clients;
}Network;


typedef struct network_detail_t{
    int sender_index;
    int next_hop_index;
    int receiver_index;
    int message_size;
    int message_chunk_size;
    int number_of_frame;
    int remaining;
    char messageChunkCopy[50];
    char incomingPortNumber[CLIENT_INCOMIN_PORT_NUMBER_SIZE];   //TODO move to client
    char outgoingPortNumber[CLIENT_OUTGOING_PORT_NUMBER_SIZE];
    client_data_types type;
}network_detail;


void readFile(FILE **file, char *fileName,const char *mode);
void readLineByLine(FILE *filePtr,char *line,size_t line_size,char *fileName,Client *clients);
void readClientFile(FILE *filePtr,char *line,size_t line_size,Network *network);
void readRoutingFile(FILE *filePtr,char *line,size_t line_size,Network *network);
void readCommandFile(FILE *filePtr,char *line,size_t line_size,Network *network,char *message_chunk_size,char *inPort,char *outPort);
void parseAndInitializeClient(char client_param[][LINE_SIZE],Network *network, int current_line_num);
void allocateClient();
int stringToInteger(char *current_line);
void tokenizeLine( char client_param[][LINE_SIZE], char *current_line);
void convertIntoAscii(char param[][4],Network *network,int client_num);
void allocate(Network *network);
void deallocate(Network *network,char *line);
void executeMessageCommand(char *line, Network *network, network_detail *network_dtl);
void executeShowFrameCommand(Network *network, char *line);
void executeShowQueueCommand(char *line);
void executeSendCommand(char *line);
void executePrintLogCommand(char *line);
int findClientIndex(Network *network, char *client_ID);
int messageFrameCount( network_detail *st_network_dtl);
void determineNextHop(Network *network, network_detail *st_network_detail);
void applicationLayerService(char *message,network_detail *st_network_detail,Network *network,Stack **top);
void transportLayerService(network_detail *st_network_detail,Network *network,Stack **top);
void networkLayerService(network_detail *st_network_detail,Network *network,Stack **top);
void physicalLayerService(network_detail *st_network_detail,Network *network, Stack **top);
/*
 *stack protototypes
 */
void initialize(Stack **top);
void Push(Stack **top, network_detail *st_network_detail,char *send, char*receive, client_data_types type, size_t s_size);
Stack* Pop(Stack **top);
/*
 * Queue
 */
void queue_initialize(Queue *queue,int init_size);
bool queue_isEmpty(Queue *queue);
void queue_enqueue(Queue *queue,Stack **top);
void queue_dequeue(Queue *queue);
Stack* queue_front(Queue *queue);

/*
 * Port number relation with application layer
 */
int main(int argc,char **argv) {
	Network network;
	FILE *clientFilePtr;
	FILE *routingFilePtr;
	FILE *commandsFilePtr;
	FILE *outputFilePtr;

	//Initialize file pointers pass them to parser function
	const size_t line_size = 300;
	char *line = malloc(sizeof(char) * (line_size+1));			//TODO free line
    errno = 0;

    //
    readFile(&clientFilePtr,argv[1],"r");
	fgets(line, line_size, clientFilePtr);
    network.numberOfClients = stringToInteger(line);

    allocate(&network);
	readClientFile(clientFilePtr,line,line_size,&network);

	//
	readFile(&routingFilePtr,argv[2],"r");
	readRoutingFile(routingFilePtr,line,line_size,&network);

	//reads commands File pass network port number from argument vector
	readFile(&commandsFilePtr,argv[3],"r");
	readCommandFile(commandsFilePtr,line,line_size,&network,argv[4],argv[5],argv[6]);

    fclose(clientFilePtr);
    fclose(routingFilePtr);
    fclose(commandsFilePtr);
	deallocate(&network,line);
	return EXIT_SUCCESS;
}

void readClientFile(FILE *filePtr,char *line,size_t line_size,Network *network){
	int line_counter=1;
    char clientParameters[3][30];           //
	while (fgets(line, line_size, filePtr) != NULL)  {
		if(line[strlen(line)-1] == '\n'){
			line[strlen(line)-1] = '\0';
		}
		printf("%s\n",line);
        tokenizeLine(clientParameters,line);
		parseAndInitializeClient(clientParameters, network, line_counter-1);
		line_counter++;
	}
}
void readFile(FILE **file,char *fileName,const char *mode){

	if(((*file) = fopen(fileName, mode)) == NULL){
		perror("Cannot open input file\n");
		exit(-1);
	}
}
void readCommandFile(FILE *filePtr,char *line,size_t line_size,Network *network,char *message_chunk_size,char *inPort,char *outPort){
    network_detail st_network_dtl;
    st_network_dtl.message_chunk_size = stringToInteger(message_chunk_size);
    printf(" size %zu\n",strlen(inPort));
    printf(" size %zu\n",sizeof(st_network_dtl.incomingPortNumber));
    memcpy(st_network_dtl.incomingPortNumber, inPort ,strlen(inPort));
    st_network_dtl.incomingPortNumber[strlen(inPort)]='\0';

    memcpy(st_network_dtl.outgoingPortNumber, outPort ,strlen(outPort));
    st_network_dtl.outgoingPortNumber[strlen(outPort)]='\0';
	while(fgets(line,line_size,filePtr)){
		if(line[strlen(line)-1] == '\n'){
			line[strlen(line)-1] = '\0';
		}
		if(strstr(line,"MESSAGE")!= NULL){
            executeMessageCommand(line,network,&st_network_dtl);
            continue;
		}
        else if(strstr(line,"SHOW_FRAME_INFO")!=NULL){
            executeShowFrameCommand(network,line);
            continue;
        }
        else if(strstr(line,"SHOW_Q_INFO")!= NULL){
            executeShowQueueCommand(line);
            continue;
        }

        else if(strstr(line,"SEND")!=NULL){
            executeSendCommand(line);
            continue;
        }
        else if(strstr(line,"PRINT_LOG")!=NULL){
            executePrintLogCommand(line);
            continue;
        }
        else {

            printf("invalid %s\n",line);
        }
	}
}
void executeMessageCommand(char *line,Network *network, network_detail *network_dtl){
    Stack *top;
    char messageCommand[4][LINE_SIZE];
    sscanf(line,"%s %s %s]",&messageCommand[0][0],&messageCommand[1][0],&messageCommand[2][0]);       //TODO fix 99  printf("%s\n",line);
    char *substring = strstr(line,"#");
    network_dtl->message_size = strlen(substring) - 2;   											//for null terminate
    size_t copy_size = network_dtl->message_size;
    char *message = (char*) malloc(sizeof(char) * (copy_size+1));
    memcpy(message, &substring[1], copy_size * sizeof(*message));
    message[copy_size]= '\0';
    network_dtl->sender_index = findClientIndex(network,&messageCommand[1][0]);
    network_dtl->receiver_index = findClientIndex(network,&messageCommand[2][0]);
    determineNextHop(network,network_dtl);
    network_dtl->number_of_frame = messageFrameCount(network_dtl);
    network_dtl->remaining = network_dtl->message_size % network_dtl->message_chunk_size ;
    int i;
    int offset;
    /*
     * initialize senders outgoing queue
     */

    queue_initialize(network->clients[network_dtl->sender_index].outQueue, 1);

    for(i=0; i<network_dtl->number_of_frame; i++){
    	initialize(&top);		//initialize head of stack

    	offset=network_dtl->message_chunk_size * i;

    	applicationLayerService(&message[offset], network_dtl, network, &top);
        transportLayerService(network_dtl, network, &top);
        networkLayerService(network_dtl, network, &top);
        physicalLayerService(network_dtl, network, &top);
        queue_enqueue(network->clients[network_dtl->sender_index].outQueue, &top);

    }
    free(message);
}

void determineNextHop(Network *network, network_detail *st_network_detail){
	int send_idx=st_network_detail->sender_index;
	int receive_idx=st_network_detail->receiver_index;
	int i;
	for(i=0; i<network->numberOfClients-1 ; i++){
		printf("routing table %s\n",&network->clients[send_idx].routingPair[i][0]);
		if(network->clients[receive_idx].clientID[0] == network->clients[send_idx].routingPair[i][0] ){  //OTDO find another comparison if necessary
			st_network_detail->next_hop_index = findClientIndex(network, &network->clients[send_idx].routingPair[i][1]);
			printf("next hop %s\n",network->clients[st_network_detail->next_hop_index].clientID);
		}
		else{
				//could not determine next hop
		}
	}
}
/*
 * 	//char messageChunk[st_network_detail->message_chunk_size+1];
 *  //	memcpy(&messageChunk[0], message, st_network_detail->message_chunk_size * sizeof(char));
 */
void applicationLayerService(char *message, network_detail *st_network_detail,Network *network,Stack **top){

	size_t copy_size =st_network_detail->message_chunk_size;
	memcpy(&st_network_detail->messageChunkCopy[0], message , copy_size * sizeof(char));
	st_network_detail->messageChunkCopy[copy_size] = '\0';


	int send_idx=st_network_detail->sender_index;
	int receive_idx=st_network_detail->receiver_index;
	size_t c_size=CLIENT_ID_SIZE;
	st_network_detail->type =ENUM_NETWORK_LAYER_APPLICATION;
	Push(top, st_network_detail, &network->clients[send_idx].clientID[0], network->clients[receive_idx].clientID,ENUM_NETWORK_LAYER_APPLICATION, c_size);
}
void transportLayerService(network_detail *st_network_detail,Network *network,Stack **top){
	//Push(top,st_network_detail,network->clients[st_network_detail->sender_index].);
	int c_size= 5;   //TODO
	st_network_detail->type=ENUM_NETWORK_LAYER_TRANSPORT;
	Push(top,st_network_detail,st_network_detail->incomingPortNumber,st_network_detail->outgoingPortNumber,ENUM_NETWORK_LAYER_TRANSPORT, c_size);
}
void networkLayerService(network_detail *st_network_detail,Network *network,Stack **top){
    int c_size=CLIENT_IP_ADDRESS_SIZE;
    int send_idx=st_network_detail->sender_index;
    int receive_idx=st_network_detail->receiver_index;
    st_network_detail->type =ENUM_NETWORK_LAYER_NETWORK;
    Push(top,st_network_detail,network->clients[send_idx].clientIPAddress,network->clients[receive_idx].clientIPAddress,ENUM_NETWORK_LAYER_NETWORK, c_size);

}
void physicalLayerService(network_detail *st_network_detail,Network *network, Stack **top){
	int s_size=CLIENT_MAC_ADDRESS_SIZE;
	int send_idx=st_network_detail->sender_index;
	int receive_idx= st_network_detail->next_hop_index;
	st_network_detail->type =ENUM_NETWORK_LAYER_PHYSICAL;
	Push(top,st_network_detail,network->clients[send_idx].clientMACAddress,network->clients[receive_idx].clientMACAddress,ENUM_NETWORK_LAYER_PHYSICAL,s_size);
}
void initialize(Stack **top)
{
    (*top) = NULL;
}



void Push(Stack **top,network_detail *st_network_detail,char *send, char*receive,client_data_types type,size_t c_size){        //return
	Stack *temp=(Stack*) malloc(sizeof(Stack));
	memcpy(temp->senderData,send, c_size * sizeof(char));    //null terminated
	memcpy(temp->receiverData, receive, c_size *sizeof(char));		//null terminated
	temp->type = type;
    if(st_network_detail->type == ENUM_NETWORK_LAYER_APPLICATION){
    	memcpy(temp->messageChunk, st_network_detail->messageChunkCopy, st_network_detail->message_chunk_size * sizeof(char));
    	temp->messageChunk[st_network_detail->message_chunk_size]= '\0';
    }
    printf("sender inside %s ",temp->senderData);
    printf("receiver inside %s ",temp->receiverData);
    printf("message chunk %s\n",temp->messageChunk);

    temp->next =  (Stack*)(*top);
    (*top) = temp;
}
/*
 *  The declaration allocates memory for the pointer, but not for the items it points to.
 *  Since an array can be accessed via pointers, you can work with *array1 as a pointer to an array whose elements are of type struct Test.
 *  But there is not yet an actual array for it to point to.
 *  queue->q_buffer[0] = &testStruct0;
 */
void queue_initialize(Queue *queue,int init_size){
	queue->front = -1;
	queue->rear = -1;
	queue->q_buffer =(Stack**) malloc(init_size * sizeof(Stack*));		//Not a pointer to the struct itself; it's a pointer to a memory location that holds the address of the struct.
	// TODO deallocate


}
bool queue_isEmpty(Queue *queue){
	if((queue->rear == -1) && (queue->front == -1)){
		return ENUM_TRUE;
	}
	return ENUM_FALSE;
}
void queue_enqueue(Queue *queue,Stack **top){
	if( queue_isEmpty(queue) == ENUM_TRUE){
		queue->front = 0;
		queue->rear = 0;
	}
	else{
		queue->rear =queue->rear+1;
	}
	//deallocates queue->q_buffer and reallocate
	queue->q_buffer =(Stack**) realloc(queue->q_buffer ,((queue->rear+1) * sizeof(Stack*)));			//todo free
	queue->q_buffer[queue->rear] = (Stack*)(*top);


}

void queue_dequeue(Queue *queue){
	if(queue_isEmpty(queue)==ENUM_TRUE){

	}
	else if(queue->front == queue->rear){
		queue->front = -1;
		queue->rear = -1;
	}
	else{
		queue->front = queue->front+1;
	}
}
Stack* queue_front(Queue *queue){
	return queue->q_buffer[queue->front];
}


//void Pop(){ //return data
//    Node *temp;
//    temp=top;
//    //n=tmp->data;
//    top=top->link;
//    free(temp);
//   // return n;
//}
//
//int Top()
//{
//    return top->data;
//}
//
//int isempty()
//{
//    return top==NULL;
//}void networkLayerService(){}

int messageFrameCount(network_detail *st_network_dtl){
    int return_val;
    if(st_network_dtl->message_size % st_network_dtl->message_chunk_size ==0){
        return_val=st_network_dtl->message_size / st_network_dtl->message_chunk_size;
    }
    else{
        return_val =st_network_dtl->message_size / st_network_dtl->message_chunk_size +1;
    }
    return return_val ;

}
int findClientIndex(Network *network,char *client_ID){
    int i;
    for(i=0;i< network->numberOfClients;i++){
        if(strstr(network->clients[i].clientID,client_ID)!=NULL){
            return i;
        }
    }
    return 0;
}
/*
 * SHOW_FRAME_INFO<space>client_ID<space>which_queue<space>frame_number
 *
 */
void executeShowFrameCommand(Network *network,char *line){
	char showFrameCommand[4][LINE_SIZE];
	tokenizeLine(showFrameCommand,line);
	int show_client_index = findClientIndex(network,&showFrameCommand[1][0]);
	int frame_index = stringToInteger(&showFrameCommand[3][0]);

//
//	while (network->clients[show_client_index].outQueue->q_buffer[frame_index]->next)
//	{
//		printf(" data %s",network->clients[show_client_index].outQueue->q_buffer[frame_index]->senderData);
//
//	}

    printf("line command %s\n client %d frame %d",line,show_client_index,frame_index);
}
void executeShowQueueCommand(char *line){
    printf("line command %s\n",line);
}
void executeSendCommand(char *line){
    printf("line command %s\n",line);
}
void executePrintLogCommand(char *line){
    printf("line command %s\n",line);
}

void readRoutingFile(FILE *filePtr,char *line,size_t line_size,Network *network){
    int line_counter=0;
    int local_idx;
    const char *delim = "-";			//can be used in later
    int client_num=0;
    char routingParameters[network->numberOfClients-1][4];
    while (fgets(line, line_size, filePtr) != NULL)  {
        if(line[strlen(line)-1] == '\n'){
            line[strlen(line)-1] = '\0';
        }
        if((line_counter % network->numberOfClients) == 4){
            client_num++;
        }
		else{
        //    printf("line num %d %s",line_counter,line);
            local_idx = line_counter % network->numberOfClients;
          //  printf("index %d\n",local_idx);
        //    memcpy(&routingParameters[local_idx][0], line, 4*sizeof(char));
         //   sscanf(line,"%s",&routingParameters[local_idx][0]);     //todo id can be string

            sscanf(line,"%c %c",&network->clients[client_num].routingPair[local_idx][0], &network->clients[client_num].routingPair[local_idx][1]);     //todo id can be string
            network->clients[client_num].routingPair[local_idx][2]='\0';

         //   memcpy(network->clients[client_num].routingPair[local_idx], &routingParameters[local_idx][0],sizeof(routingParameters[local_idx]));
            printf(" routing %c%c\n",network->clients[client_num].routingPair[local_idx][0],network->clients[client_num].routingPair[local_idx][1]);


        }
        line_counter++;

	}

}


void convertIntoAscii(char param[][4] ,Network *network,int client_num){
    int i=0;
    int j;
    for(i = 0; i<(network->numberOfClients-1); i++) {
     //   for(j = 0; param[ i ][ j ]; j++){
        network->clients[client_num].routingPair[i][0]= param[i][0];
        network->clients[client_num].routingPair[i][1]= param[i][2];
        //printf("%d",network->clients[client_num].routingPair[i][0]);
       // printf("%d",network->clients[client_num].routingPair[i][1]);
        printf("\n");
  }



}

void tokenizeLine( char param[][LINE_SIZE],char *current_line){
    char *split;
    int i=0;
    split= strtok (current_line," ");
    size_t copy_size;
    while(split != NULL) {
    	copy_size=strlen(split);

    	memcpy(&param[i][0], split, (copy_size+1)*sizeof(char));
        split = strtok(NULL, " ");
        i++;
	}


}
int stringToInteger(char *current_line){
	long keyLong;
	char *p;
	int client_number_of;
	const size_t copy_size = 10;
	//p = (char*)malloc(sizeof(char) * (copy_size+1));
	keyLong=strtol(current_line,&p,10);
    client_number_of = keyLong;
	/*
	if (errno != 0 || *p != '\0' || keyLong > INT_MAX) {
		printf("Error: not valid argument\n");
	} else {
		client_number_of = keyLong;
	}
	*/
//	free(p);
	return client_number_of;
}

void allocate(Network *network){
    network->clients = (Client*) malloc(sizeof(Client) * network->numberOfClients);
    int i;
    int j;
    for(i=0; i<network->numberOfClients; i++ ){
    	network->clients[i].inQueue =(Queue*) malloc(sizeof(Queue));
    	network->clients[i].outQueue = (Queue*) malloc(sizeof(Queue));
        network->clients[i].routingPair=(char**) malloc(sizeof(char*) * (network->numberOfClients-1));
        for(j = 0; j < (network->numberOfClients-1); j++){
            network->clients[i].routingPair[j] = (char *) malloc(3 * sizeof(char));
        }
    }

}
void parseAndInitializeClient(char client_param[][LINE_SIZE],Network *network,int current_line_num){

	int i=0;
	//todo strncat
	memcpy(network->clients[current_line_num].clientID, &client_param[i++][0], CLIENT_ID_SIZE*sizeof(char));
	memcpy(network->clients[current_line_num].clientIPAddress, &client_param[i++][0], CLIENT_IP_ADDRESS_SIZE *sizeof(char));
	memcpy(network->clients[current_line_num].clientMACAddress,&client_param[i][0],CLIENT_MAC_ADDRESS_SIZE *sizeof(char));

}
/*
 * Parse client
 * Parse routing
 * Parse and execute commands
 */
void readLineByLine(FILE *filePtr,char *line,size_t line_size,char *fileName,Client *clients){
	int lineCounter=0;
	int client_size;
	int numberOfClient;
	while (fgets(line, line_size, filePtr) != NULL)  {
		if(strcpy(fileName,"clients.dat")){
			client_size=5;						//TODO
			printf("%s",line);
				//	parseClientsFile(lineCounter);
		}
		else if(strcpy(fileName,"routing.dat")){
			printf("%s",line);
		}
		lineCounter++;
	}
}
/*
 * Takes first line and
 * client id client ip client mac
 *
 */
void parseClientsFile(int line_number,char *current_line){


}



void deallocate(Network *network,char *line){
	int i;
	int j;
	for(i=0; i<network->numberOfClients; i++ ){
		for(j = 0; j < (network->numberOfClients-1); j++){
			free(network->clients[i].routingPair[j]);
		}
		free(network->clients[i].inQueue);
		free(network->clients[i].outQueue);
		free(network->clients[i].routingPair);
	}

	free(network->clients);
	free(line);
}
