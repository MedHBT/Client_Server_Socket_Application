/*********************************LIBRARY***********************************/
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
#include <sys/stat.h>
#include <dirent.h>
#include <libgen.h>
#include <time.h>
/********************GLOBAL DECLARATION VARIVALES****************************/
#define LENGTH 4096 /*max text line length*/
#define PORT 3000 /*port*/
#define LISTENQ 1 /*maximum number of client connections*/
int  NUMCLIENT = 0;

/****************************START OF FONCTION CODE**************************/
//Error message handler
void error(const char *msg)
{
  perror(msg);
  exit(1);
}
//Find the number of string's occurrence in a file and return it
int searchOcc (char *fileName, char *strSearch)
{
      FILE *f;
      char strSource[LENGTH];
      char *p;
      int nb = 0;
      int lengthct = strlen (strSearch);
      f = fopen(fileName,"r");
      if( f == NULL ){
        
         return 0;
      }
      while (fgets(strSource, LENGTH, f) != NULL){
            p = strSource;
         while(p = strstr(p, strSearch)){
            nb++;
            p++;
        }
      }
      fclose(f);
      return nb;
}
//Substitute a string with a new one.The count varibale can fixe the number of substitution,
//it's intialized to NULL in all the programm to substitute all availble string.
char* substitute(const char *str, const char *oldstr,const char *newstr, int *count)
{
   const char *tmp = str;
   char *result;
   int   found = 0;
   int   length, reslen;
   int   oldlen = strlen(oldstr);
   int   newlen = strlen(newstr);
   int   limit = (count != NULL && *count > 0) ? *count : -1;

   /* Count the number of times that the original string is found */
   while ((tmp = strstr(tmp, oldstr)) != NULL && found != limit)
      found++, tmp += oldlen; /* Reprend après la chaine trouvée */

   /* Calculate the memory requirements for the new string */
   length = strlen(str) + found * (newlen - oldlen);
   if ( (result = (char *)malloc(length+1)) == NULL) {
      fprintf(stderr, "[Server System] ERROR: Not enough memory\n");
      found = -1;
   } else {
      tmp = str;
      limit = found; /* Countdown */
      reslen = 0;  /* current length of the result */
      /* For each sub-string found, is placed that of substitution */
      while ((limit-- > 0) && (tmp = strstr(tmp, oldstr)) != NULL) {
         length = (tmp - str); /* Number of unchanged characters  */
         strncpy(result + reslen, str, length); /* Copy unchanged part */
         strcpy(result + (reslen += length), newstr); /* Add newstr */
         reslen += newlen;
         tmp += oldlen;
         str = tmp;
      }
      strcpy(result + reslen, str); /* Add end of unchanged the string */
   }
   if (count != NULL) *count = found;
   return result;
}
//Replace all occurrence of string in a file with a new one
FILE* strReplace(char* subpath,char *fileName,char *strA , char *strB ,int * replaceOcc){
    FILE *f1;
    FILE *f2;
    char strSource[LENGTH];
    f1 = fopen(fileName,"r");
    if( f1 == NULL )
      return NULL;
    f2 = fopen(subpath,"w");
    while (fgets(strSource, LENGTH, f1) != NULL){
        fprintf(f2,"%s", substitute(strSource,strA,strB,replaceOcc));
    }
    fclose(f1);
    fclose(f2);
    return f2;
}
//Find a given string in a file
int strFind(char *fileName,char *strSearch) {
  FILE *f;
  char c;
  int found=0;
  int pos=0;
  int length;
  f = fopen(fileName,"r");
  if( f == NULL ){
    printf("[Server System] ERROR: File %s Cannot be opened file on server.\n",fileName);
    return 0;
  }
  length=strlen(strSearch);
  while( fread(&c,1,1,f)>0 && !found ) {
    if( c==strSearch[pos] ) {
      pos++;
    } else {
      if(pos!=0) {
        // On doit rembobiner !
        fseek(f,-pos,SEEK_CUR);
        pos=0;
      }
    }
    found = (pos==length);
  }
  fclose(f);
  return found;
}
//Remove a substring from a string
void removeSubstring(char *strSource,char *strToRemove)
{
  while( strSource=strstr(strSource,strToRemove) )
    memmove(strSource,strSource+strlen(strToRemove),1+strlen(strSource+strlen(strToRemove)));
}
//Remove string from file
FILE* strRemove(char* subpath,char *fileName,char *strToRemove)
{
    FILE *f1;
    FILE *f2;
    char strSource[LENGTH];
    f1 = fopen(fileName,"r");
    if( f1 == NULL ){
      printf("[Server System] ERROR: File %s Cannot be opened file on server.\n",fileName);
      return NULL;
    }
    f2 = fopen(subpath,"w+");
    if( f2 == NULL ){
      printf("[Server System] ERROR: File %s Cannot be opened file on server.\n",subpath);
      return NULL;
    }
    while (fgets(strSource, LENGTH, f1) != NULL){
            removeSubstring(strSource,strToRemove);
            fprintf(f2,"%s",strSource);
    }
    fclose(f1);
    fclose(f2);
    return f2;
}
//Receive file from client
void receiveFile(char* fr_name,int nsockfd){
    char revbuf[LENGTH]; /* Receiver buffer */
    FILE *fr = fopen(fr_name, "w+");/* Create File */
    if(fr == NULL)
      printf("[Server System] ERROR: File %s Cannot be opened file on server.\n", fr_name);
    else
    {
      bzero(revbuf, LENGTH); /* Flash the buffer(set to 0) */
      int fr_block_sz = 0;
      while((fr_block_sz = recv(nsockfd, revbuf, LENGTH, 0)) > 0) /* Receive file line by line via buffer */
      {
          int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
          if(write_sz < fr_block_sz){
              error("[Server System] ERROR: File write failed on server.\n");
          }
          bzero(revbuf, LENGTH); /* Flash the buffer(set to 0) */
          if (fr_block_sz == 0 || fr_block_sz != 512){
             break;
          }
      }
      if(fr_block_sz < 0){
            if (errno == EAGAIN)
            {
                  printf("[Server System] ERROR: recv() timed out.\n");
              }
              else
              {
                  fprintf(stderr, "[Server System] ERROR: recv() failed due to errno = %d\n", errno);
                  exit(1);
              }
          }
      printf("[Server] Ok received from client!\n");
      fclose(fr); 
    }
}
//Send file to the client
void sendFile(char* fs_name,int nsockfd){

        char sdbuf[LENGTH]; /* Send buffer */
        printf("[Server] Sending %s to the Client...\n", fs_name);
        FILE *fs = fopen(fs_name, "r"); /* File to send */
        if(fs == NULL)
        {
            fprintf(stderr, "[Server System] ERROR: File %s not found on server. (errno = %d)\n", fs_name, errno);
            exit(1);
        }
        bzero(sdbuf, LENGTH); /* Flash the buffer(set to 0) */
        int fs_block_sz; 
        while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs))>0)
        {
            if(send(nsockfd, sdbuf, fs_block_sz, 0) < 0)
            {
                fprintf(stderr, "[Server System] ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
                exit(1);
            }
            bzero(sdbuf, LENGTH); /* Flash the buffer(set to 0) */
        }
        printf("[Server] Ok sent to client!\n");
}
//Copy a dublicate a file by given names to the new one
void copyFile(char* fileName,char* fileNameOutput,char* mode){
   char ch;
   FILE *src, *tgt;
   src = fopen(fileName, "r");/* Open the source file to copy from it */
   if( src == NULL )
   {
      printf("[Server System] ERROR: File %s Cannot be opened.\n[Server System] Press any key to exit...\n",fileName);
      exit(EXIT_FAILURE);
   }
   tgt = fopen(fileNameOutput, mode);/* Open the target file to copy to it */
   if( tgt == NULL )
   {
      fclose(src);
      printf("[Server System] ERROR: File %s Cannot be opened.\n[Server System] Press any key to exit...\n",fileNameOutput);
      exit(EXIT_FAILURE);
   }
   while( ( ch = fgetc(src) ) != EOF ) /* Copy process caracter by caracter */
      fputc(ch, tgt);
   printf("[Server] File copied successfully.\n");
   fclose(src);
   fclose(tgt);
}
//Generate the right file path for the given file name
void getPath(char* path, char* subpath, char* pathToAdd){
   strcpy (subpath,path); /* clone the path to the subpath*/
   strcat(subpath,pathToAdd); /* Add the name of the file */
}
//Execute the serveces provided by the server
int applyServices(char* path,char* fileName,char* fileNameApplyTo,int nsockfd,int *sig,int *sigTab){
    int choix;/* Client choice */
    int found;/*StrFind result*/
    int numbOcc;/*StrOcc result*/
    char str1[LENGTH];/*The first string extracted from temperary file*/
    char str2[LENGTH];/*The second string extracted from temperary file*/
    char subpath[LENGTH];/*Path for the output.txt*/
    char subpath1[LENGTH];/*Path for the tempOuput1.txt*/
    char subpath2[LENGTH];/*Path for the tempOuput2.txt*/
    getPath(path,subpath,"/output.txt");/*Generate path for the output file*/
    getPath(path,subpath1,"/tempOuput1.txt");/*Generate the path for temperary file of the third option*/
    getPath(path,subpath2,"/tempOuput2.txt");/*Generate the path for temperary file of the fourth option*/
    FILE* fp;
    FILE* f_response = fopen(fileName,"r");/*Open the response file to get the client preferences*/
    if(f_response == NULL)
       printf("[Server System] ERROR: File %s Cannot be opened.\n", fileName);
    fscanf(f_response,"%d",&choix);/*copy the response from the file*/
    switch(choix){/*switch will choose the desired service to execute*/
    case 1:/*Case of deleting string from the file*/
        fscanf(f_response,"%s",str1);/*Get the string to delete*/
        strRemove(subpath,fileNameApplyTo,str1);/*Delete the string from receivedFile and put the result in the outputFile */
        copyFile(subpath,fileNameApplyTo,"w+");/*Overwrite the file to be ready for the next loop*/
        sigTab[0]=1;sigTab[1]=0;sigTab[2]=0;  /* Save the last crossing */
        break;
    case 2:/*Case of replacing string A with string B*/
        fscanf(f_response,"%s",str1);/*Get the first string to replace*/
        fscanf(f_response,"%s",str2);/*Get the second string to replace with*/
        strReplace(subpath,fileNameApplyTo,str1,str2,NULL);/*Replace the first string by the second string*/
        copyFile(subpath,fileNameApplyTo,"w+");/*Overwrite the file to be ready for the next loop*/
        sigTab[0]=1;sigTab[1]=0;sigTab[2]=0;  /* Save the last crossing */
        break;
    case 3:/*Case of searching the existence of a string in the file*/
        //if(*sig == 0){fp = fopen(subpath,"w+");fclose(fp);}/*create ouput.txt file if the user choose case 3 or 4 in first place*/
        fscanf(f_response,"%s",str1);/*Get the given string to search it in the file*/
        found = strFind(fileNameApplyTo,str1);/*Search the string and return the result(0 for not found else for found)*/
        fp = fopen(subpath1,"w+");/*Save the result in a temperary file*/
        if( fp == NULL )
           printf("[Server System] ERROR: File %s Cannot be opened.\n", "output.txt");
        if(found != 0)/*For each case it have an output*/
           fprintf(fp,"\nLA CHAINE ''%s'' QUE VOUS CHERCHEZ SE TROUVE DANS LE FICHIER\n",str1);
        else
           fprintf(fp,"\nLA CHAINE ''%s'' QUE VOUS CHERCHER N'EXISTE PAS DANS LE FICHIER\n",str1);
        fclose(fp);
         sigTab[0]=0;sigTab[1]=1;sigTab[2]=0;sigTab[3]=1;
        break;
    case 4:/*case of searching the number of occurrence of a string*/
        fscanf(f_response,"%s",str1);/*Get the given string to search it in the file*/
        numbOcc = searchOcc(fileNameApplyTo,str1);/*Calculate the nomber of occurrence of the string in the file*/
        fp = fopen(subpath2,"w+");/*Save the result in a temperary file*/
        if( fp == NULL )
           printf("[Server System] ERROR: File %s Cannot be opened.\n", "output.txt");
        fprintf(fp,"\nLE NOMBRE D'OCCURENCE DE LA CHAINE ''%s'' EST : %d\n",str1,numbOcc);
        fclose(fp);
        sigTab[0]=0;sigTab[1]=0;sigTab[2]=1;sigTab[4]=1;/* Save the last crossing */
        break;
    case 5:
        if(*sig == 0){
          fp = fopen(subpath,"w+");/*Create the output file for the first crossing*/
          if( fp == NULL )
             printf("[Server System] ERROR: File %s Cannot be opened.\n", "output.txt");
          fprintf(fp,"LE FICHIER EST VIDE.VOUS N'AVEZ EXECUTER AUCUN SERVICE.\n");/*Precise that the file is empty*/
          fclose(fp);
          sendFile(subpath,nsockfd);/*Send the file for the client to visualize it*/
        }
        else{
                if(sigTab[0] == 0 && sigTab[1] == 1 && sigTab[2] == 0){
                       copyFile(subpath1,subpath,"w+");
                       sendFile(subpath,nsockfd);/*Send the file for the client to visualize it*/
                }
                else if(sigTab[0] == 0 && sigTab[1] == 0 && sigTab[2] == 1){
          	       copyFile(subpath2,subpath,"w+");
          	       sendFile(subpath,nsockfd);/*Send the file for the client to visualize it*/
       	        }
                else
                       sendFile(subpath,nsockfd);/*Send the file for the client to visualize it*/
        }
        break;
    case 6:
        if(*sig == 0){/*For the first crossing of this loop create the output*/
           copyFile(fileNameApplyTo,subpath,"w+");
           fp = fopen(subpath,"a");
           if( fp == NULL )
              printf("[Server System] ERROR: File %s Cannot be opened.\n", "output.txt");
           fprintf(fp,"\nVOUS N'AVEZ EXECUTER AUCUN SERVICE.VOTRE FICHIER N'EST PAS MODIFIER.");/*Print the output message in the file*/
           fclose(fp);
        }
        else{
           if(!(sigTab[0] == 1 && sigTab[1] == 0 && sigTab[2] == 0) && sigTab[3] == 1)/* Test the order of execution*/
              copyFile(subpath1,subpath,"a");
           if(!(sigTab[0] == 0 && sigTab[1] == 0 && sigTab[2] == 1) && sigTab[4] == 1)/* Test the order of execution*/
              copyFile(subpath2,subpath,"a");
        }
        break;
    }
    fclose(f_response);
    return choix;
}
//Create Repertory for each incoming client.
void createRepertory(char* path,int numClient){
  char nClient[LENGTH];
  sprintf(nClient,"%d",numClient);
  getPath("./client",path,nClient);
  mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  
}
//Remove Repertory and all included file
int removeDirectory(char* dirname) {
  DIR *dp;
  struct dirent *ep;
  char abs_filename[FILENAME_MAX];
  dp = opendir (dirname);
  if (dp != NULL)
    {
      while (ep = readdir (dp)) {
        struct stat stFileInfo;
        snprintf(abs_filename, FILENAME_MAX, "%s/%s", dirname, ep->d_name);
        if (lstat(abs_filename, &stFileInfo) < 0)
          perror ( abs_filename );
        if(S_ISDIR(stFileInfo.st_mode)) {
          if(strcmp(ep->d_name, ".") && 
             strcmp(ep->d_name, "..")) {
            printf("%s directory\n",abs_filename);
            removeDirectory(abs_filename);
          }
        } 
        else{
                  printf("[Server] %s file removed!\n",abs_filename);
                  remove(abs_filename);
        }
      }
      (void) closedir (dp);
  }
  else
    perror ("Couldn't open the directory");
  remove(dirname);
  printf("[Server] Directory of %s removed!",dirname);
  return 0;
}
//Execute the providing server services
void processingServices(int numClient,int nsockfd){
  char path[LENGTH] ;/*Path for the directory*/
  char subpath1[LENGTH] ;/*Path for the received file*/
  char subpath2[LENGTH] ;/*Path for the temp file */
  char subpath3[LENGTH] ;/*Path for the output file*/
  int choix;
  int sig= 0;
  int sigTab [5] ={0,0,0,0,0};
  createRepertory(path,numClient);
  getPath(path,subpath1,"/receive.txt");/*Generate path*/
  getPath(path,subpath2,"/temp.txt");/*Generate path*/
  getPath(path,subpath3,"/output.txt");/*Generate path*/
  receiveFile(subpath1,nsockfd);/*Receive the client file to modifie it*/
  sendFile("menu_service_server.txt",nsockfd);/*Send Menu Service file*/
  copyFile(subpath1,subpath3,"w+");
  do{
           receiveFile(subpath2,nsockfd);/* Receive Response Service */
           choix = applyServices(path,subpath2,subpath1,nsockfd,&sig,sigTab);/* Read Response Service and Apply it */
           sig = 1;
    }while(choix != 6);
  sendFile(subpath3,nsockfd); 
  //Remove all temporary file and the client directory
  removeSubstring(path,"./");
  removeDirectory(path);
}
//Signal Traitment for child pid genereted by fork()
void signal_handler(int sig){
        
	while(waitpid(-1,NULL,WNOHANG)>0)
             ;
        NUMCLIENT --;/*decrease the number of client if one of blocking child finished */
        return;
        
}
/******************************END OF FONCTION CODE**************************/

int main (int argc, char **argv)
{
 int sockfd, nsockfd; /*Socket Id*/
 struct sigaction sig;
 pid_t childpid;
 socklen_t clilen;
 char buf[LENGTH];
 int randNum;
 struct sockaddr_in cliaddr, servaddr;
 //Create a socket for the soclet
 //If sockfd<0 there was an error in the creation of the socket
 if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
  perror("[Server] Problem in creating the socket");
  exit(2);
 }
 printf("[Server] Obtaining socket descriptor successfully.\n");

 //preparation of the socket address
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 servaddr.sin_port = htons(PORT);

 //bind the socket
 bind (sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
 printf("[Server] Binded tcp port %d in addr 127.0.0.1 sucessfully.\n",PORT); 

 //listen to the socket by creating a connection queue, then wait for clients
 listen (sockfd, LISTENQ);
 printf("%s\n","[Server] Server running...waiting for connections.");

 
signal(SIGCHLD,signal_handler);/*Treat the child signal*/
 while(1) {/*This loop keep the server running and waiting for the client connection*/
  
  	clilen = sizeof(cliaddr);
  	//accept a connection
  	if(NUMCLIENT <10 && NUMCLIENT>=0){
    		
    		nsockfd = accept (sockfd, (struct sockaddr *) &cliaddr, &clilen);
    		printf("%s\n","[Server] Received request...");
    		NUMCLIENT++;/*For each child created the numclient increase*/
    		if ( (childpid = fork ()) == 0 ) {/*child process*/
                               printf ("%s\n","[Server] Child created for dealing with client requests");
                               close (sockfd);/*close the client socket*/
                               srand(time(NULL));
                               randNum = rand();
                               processingServices(randNum,nsockfd);/*Execute the serveces*/
                               exit(0);
    		}
                else
                   close(nsockfd);/*close the server socket*/
  	}
       
 }

 
}
