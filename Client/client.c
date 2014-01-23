#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>          
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 3000
#define LENGTH 4096

/*******************************************************************/
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void displayFile(char* fileName){
   char line[LENGTH];
   FILE *file=fopen(fileName,"r");
   if(file == NULL)
		printf("[Client System] ERROR: File %s Cannot be opened.\n", fileName);
   while(fgets(line, LENGTH, file) != NULL)
    {
            printf("%s", line);
    }
    fclose(file);
}

void sendFile(char* fs_name,int sockfd){

		char sdbuf[LENGTH]; 
		printf("[Client] Sending %s to the Server... \n", fs_name);
		FILE *fs = fopen(fs_name, "r");
		if(fs == NULL)
		{
			printf("[Client System] ERROR: File %s not found.\n", fs_name);
			exit(1);
		}

		bzero(sdbuf, LENGTH); 
		int fs_block_sz; 
		while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0)
		{
		    if(send(sockfd, sdbuf, fs_block_sz, 0) < 0)
		    {
		        fprintf(stderr, "[Client System] ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
		        break;
		    }
		    bzero(sdbuf, LENGTH);
		}
		printf("[Client] Ok File %s from Client was Sent!\n", fs_name);
}

void receiveFile(char* fr_name,int sockfd){
        char revbuf[LENGTH];
        printf("[Client] Receiveing file from Server and saving it...\n");
	//char* fr_name = "final.txt";
	FILE *fr = fopen(fr_name, "w+");
	if(fr == NULL)
		printf("[Client System] ERROR: File %s Cannot be opened.\n", fr_name);
	else
	{
	    bzero(revbuf, LENGTH); 
	    int fr_block_sz = 0;
	    while((fr_block_sz = recv(sockfd, revbuf, LENGTH, 0)) > 0)
	    {
		int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
	        if(write_sz < fr_block_sz){
	            error("[Client System] ERROR: File write failed.\n");
	        }
		bzero(revbuf, LENGTH);
		if (fr_block_sz == 0 || fr_block_sz != 512){
		    break;
		}
	    }
	    if(fr_block_sz < 0){
		if (errno == EAGAIN){
		             printf("[Client System] ERROR: recv() timed out.\n");
		}
		else{
			     fprintf(stderr, "[Client System] ERROR: recv() failed due to errno = %d\n", errno);
		}
	    }
	    printf("[Client] Ok received from server!\n");
	    fclose(fr);
	}
}

int servicesResponse(char *fileName){
    
    int choix;
    char str1[LENGTH];
    char str2[LENGTH];
    
    FILE *temp=fopen("temp.txt","w+");
    if(temp == NULL)
		printf("[Client System] ERROR: File %s Cannot be opened.\n", "temp.txt");
    
    displayFile(fileName);
    do{
            printf("[MENU] Donnez votre choix\n");
            printf("[Client] ");
            scanf("%d",&choix);
    }while(choix>6 || choix<1);
    fprintf(temp,"%d\n",choix);
    switch(choix){
    case 1:
        printf("[MENU] Donnez la chaine de caractere que vous voulez supprimer\n");
        printf("[Client] ");
        scanf("%s",str1);
        fprintf(temp,"%s\n",str1);
        break;
    case 2:
        printf("[MENU] Donnez la chaine de caractere A que vous voulez remplacer\n");
        printf("[Client] ");
        scanf("%s",str1);
        fprintf(temp,"%s\n",str1);
        printf("[MENU] Donnez la chaine de caractere B que vous voulez mettre a la place de A\n");
        printf("[Client] ");
        scanf("%s",str2);
        fprintf(temp,"%s\n",str2);
        break;
    case 3:
        printf("[MENU] Donnez la chaine de caractere que vous voulez chercher\n");
        printf("[Client] ");
        scanf("%s",str1);
        fprintf(temp,"%s\n",str1);
        break;
    case 4:
        printf("[MENU] Donnez la chaine de caractere dont vous voulez avoir le nombre d'occurences\n");
        printf("[Client] ");
        scanf("%s",str1);
        fprintf(temp,"%s\n",str1);
        break;
    }
    
    
    fclose(temp);
    return choix;
}

void displayServices(int sockfd){
     /* Send File to Server */
     sendFile("tosend.txt",sockfd);
	
     /* Receive Services Menu File from server*/
        receiveFile("menu_service_server.txt",sockfd);
        int choix;
        do{
               /* Read Services and write response */
               choix = servicesResponse("menu_service_server.txt");
               /* Send Services Response */
               sendFile("temp.txt",sockfd);
               if(choix == 5){
                   receiveFile("tempInput.txt",sockfd);
                   printf("[MENU] Previsualisation du resultat des modifications\n");
                   displayFile("tempInput.txt");
               }
        }while(choix != 6);

	/* Receive File from Server */
        receiveFile("input.txt",sockfd);
        remove("temp.txt");
        remove("menu_service_server.txt");
        remove("tempInput.txt");
        
}

/******************************************************************************************/

int main(int argc, char **argv) 
{
   int sockfd;

   struct sockaddr_in servaddr;
   char sendline[LENGTH], recvline[LENGTH];
	

	
   //Create a socket for the client
   //If sockfd<0 there was an error in the creation of the socket
   if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
      perror("[Client System] ERROR: Problem in creating the socket");
      exit(2);
   }
	
   //Creation of the socket
   memset(&servaddr, 0, sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr= inet_addr("127.0.0.1");
   servaddr.sin_port =  htons(PORT); //convert to big-endian order
	
   //Connection of the client to the socket 
   
   if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
      perror("[Client System] ERROR: Problem in connecting to the server");
      exit(0);
   }
   
   displayServices(sockfd);

   exit(0);
}
