#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>


#define BUFFSIZE 32
#define WSIZE 10000

int maxWidth;



int wrapWords(int maxWidth,char* fullPath,int selectCase,char* directoryPath,char* fileName){

    int errorCondition = 0;
    char *buff = (char*) malloc(BUFFSIZE*sizeof(char));

    char word[WSIZE];
    memset(word,0,WSIZE-1);

    int readFD;
    int writeFD;

    if(selectCase == 1){
        // We are reading from stdin and writing to stdout
        readFD = 0;
        writeFD = 1;
    }

    else if(selectCase == 2){
        //We are reading from an input file, open the file
        
        readFD = open(fullPath,O_RDONLY);
        

        if(readFD < 0){
            
            perror("Error ");
            return 1;
        }

        //We are writing to stdOut, its file descriptor is 1
        writeFD = 1;


    }

    else{

        // We are reading from an input file, opeen the file
        
        readFD = open(fullPath,O_RDONLY);

        if(readFD < 0){
            free(buff);
            perror("Error ");
            return 1;
        }


        // From the wrappingQueue get the directoryPath and the fileName





        // We write to a new file starting with "wrap.fileName"
        // We save it to the directory that the file is from
        char *wrap = "wrap.";
        char newFilePath[1024];
        memset(newFilePath,0,1024);
        strcpy(newFilePath,directoryPath);

        if(selectCase == 4){
            strcat(newFilePath,"/");
        }

        strcat(newFilePath,wrap);
        strcat(newFilePath,fileName);
        //printf("Wrapping Function: Wrapping File Name: %s, New File Name: %s\n",fullPath,newFilePath);        
        
        // Open the new file in the directory
        writeFD = open(newFilePath,O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        //writeFD = openat(directoryFD,newFilePath,O_WRONLY | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    }



    // These variables are used in order to track the information for the word being read, and the currentline.
    // Currentwordsize is the size of the word that we are reading from buff
    // linecharcount is the amount of char's printed to the line currently
    //  prevCharRead is to keep track of the previous char that we read from buff
    // paragraphPrinted is to keep track if we printed a paragraph, we use this to filter consecutive '\n' chars
    // fileIsBlank checks if any words have been printed to the file, if they have not been then we skip the white space
    //      until have finally read a word.
    int currentWordSize = 0;
    int lineCharCount = 0;
    char prevCharRead;
    int paragraphPrinted = 0;
    int fileIsBlank = 0;

    char singleNewLine = '\n';
    char singleSpace = ' ';

    int bytesRead;

    // Using the file descriptor fd, open the file and start reading from it
    while((bytesRead = read(readFD,buff,BUFFSIZE))>0){
        
        // buffCounter is the place we are at in the buff
        int buffCounter = 0;

        while(buffCounter < BUFFSIZE){

            // Read the char from the buff, and check if it is a white space.
            char charRead = buff[buffCounter];
            int isWhitespaceChar = isspace(charRead);

            
            // If the charRead is a whitespace then we most likely are printing out the word
            if(isWhitespaceChar != 0){

                // If the current white space is a newline, and the previous char was a newline, then we consider printing a blank line
                // in order to seperate paragraphs.                
                if(prevCharRead == '\n' && charRead == '\n'){
                    
                    if(fileIsBlank == 0 && currentWordSize == 0){
                        buffCounter++;
                        prevCharRead = charRead;
                        continue;
                    }

                    // If paragraphPrinted == 1, then we just printed a paragraph and we ignore the newline char
                    if(paragraphPrinted == 1){
                        buffCounter++;
                        prevCharRead = '\n';
                        continue;
                    }

                    // If there are chars printed on the current line then we need to print a new line in order to get to the next blank line,
                    // we then print another new line in order to seperate the paragraphs.
                    // Set paragraphPrinted = 1 to make sure that we ignore newline chars if they are next in the buff array
                    // The line we are currently on is blank, therefore set lineCharCount = 0
                    else if(lineCharCount !=0){
                        write(writeFD,&singleNewLine,1);
                        write(writeFD,&singleNewLine,1);
                        
                        lineCharCount = 0;
                        paragraphPrinted = 1;
                    }

                    // In this situation we are currently on a blank line and we just need to print a new line char to create the paragraph break
                    // Set paragraphPrinted = 1 to make sure that we ignore newline chars if they are next in the buff array
                    else if(paragraphPrinted == 0){
                        write(writeFD,&singleNewLine,1);
                        
                        lineCharCount = 0;
                        paragraphPrinted = 1;
                    }
                }

                // There is no word in the word[] therefore we are dealing with consecutive whitespaces
                // Ignore any consective whitespaces that are not newline chars
                else if(currentWordSize == 0  && lineCharCount!= 0){
                    prevCharRead = charRead;
                    buffCounter++;
                    continue;
                }

                // We have a full word in the word[], and we are on a blank line
                else if(currentWordSize!=0   &&  lineCharCount ==0){
                    
                    // We need to calculate the spaceremaning if the word were to be printed with a new line char at the end
                    int spaceRemaining = maxWidth - 1 - currentWordSize;

                    // The word that we have is bigger then the maxWidth,
                    // we print word out and set the errorConditon = 1
                    if(currentWordSize >= maxWidth   ||  spaceRemaining < 0){
                        errorCondition = 1;
                        write(writeFD,word,currentWordSize);
                        write(writeFD,&singleNewLine,1);
                        
                        currentWordSize = 0;
                    }

                    // There is exactly enough space to print the word with the new line char
                    else if(spaceRemaining == 0){
                        write(writeFD,word,currentWordSize);                
                        write(writeFD,&singleNewLine,1);
                        currentWordSize = 0;
                    }

                    // The word is printed out normally
                    else{
                        write(writeFD,word,currentWordSize);
                        
                        lineCharCount += currentWordSize;
                        currentWordSize = 0;
                    }

                    // A word was just printed, therefore the we have to start taking consecutive newline chars into consideration
                    paragraphPrinted = 0;
                    fileIsBlank = 1;

                }

                // We have a full word in the word[], and we are not on a blank line
                else if(currentWordSize!=0   &&  lineCharCount!=0){
                    
                     // Calculate the spaceremaning if we print a space followed by the word and then a new line char
                    int spaceRemaining = maxWidth - 2 - lineCharCount - currentWordSize;
                    
                    
                    // The word is bigger then the maxWidth,
                    // Print a newline, print the word and then another new line
                    // Set errorCondition = 1
                    // Set lineCharCount = 0 to indicate that we are now on a new line
                    // Set currentWordSize = 0 to indicate that there is no word in the word[]
                    if(currentWordSize >= maxWidth){
                        errorCondition = 1;
                        write(writeFD,&singleNewLine,1);
                        write(writeFD,word,currentWordSize);
                        write(writeFD,&singleNewLine,1);
                        
                        lineCharCount = 0;
                        currentWordSize = 0;
                    }

                    // There is exactly enough space to print the space, the word and a new line char
                    // Set lineCharCount = 0 to indicate that we are now on a new line
                    // Set currentWordSize = 0 to indicate that there is no word in the word[]
                    else if(spaceRemaining == 0){
                        write(writeFD,&singleSpace,1);
                        write(writeFD,word,currentWordSize);
                        write(writeFD,&singleNewLine,1);
                        
                        lineCharCount = 0;
                        currentWordSize = 0;
                    }

                    // There is not enough space to print the space, the word and the newline char
                    // Must move to a new line and print out the word
                    // Set lineCharCount = currentWordSize to indice that the current line is not empty/blank
                    // Set currentWordSize = 0 to indicate that there is no word in the word[]
                    else if(spaceRemaining<0){
                        write(writeFD,&singleNewLine,1);
                        write(writeFD,word,currentWordSize);
                        
                        lineCharCount = currentWordSize;
                        currentWordSize = 0;
                    }

                    // There is plenty of space remaining on the current line
                    // Just print the space and the current word
                    // Increment lineCharCount by the size of the word printed plus 1
                    // Set currentWordSize = 0 to indicate that there is no word in the word[]
                    else{
                        write(writeFD,&singleSpace,1);
                        write(writeFD,word,currentWordSize);
                        
                        lineCharCount = lineCharCount +currentWordSize + 1;
                        currentWordSize = 0;

                    }

                    // A word was just printed, therefore the we have to start taking consecutive newline chars into consideration
                    paragraphPrinted = 0;
                    fileIsBlank = 1;
                }
        
                // A word was just printed, reset the memory to begin reading the next word
                memset(word,0,WSIZE-1);
                
            }
            
            //The char read was not a whitespace, therefore the char is apart of the currentword that is being read
            //Add the char to current word, and increase the size of the current word
            else{

            // Increment the buffcounter to read the next char from buff[], and set the prevCharRead to the char that was just read.
                word[currentWordSize] = charRead;
                currentWordSize++;

            }

            // Increment the buffcounter to read the next char from buff[], and set the prevCharRead to the char that was just read.
            buffCounter++;
            prevCharRead = charRead;
            


        }

        // We are done with the current contents of the buffer, reset the memory of the buff[] before the next call to read()
        memset(buff,0,BUFFSIZE-1);
    }

    if(lineCharCount !=0 && fileIsBlank !=0 && selectCase!=3){
        write(writeFD,&singleNewLine,1);
    }



    // Free the char* that was allocated earlier
    free(buff);


    if(selectCase == 2){
        // SelectCase = 2 -> Read from a file, and write to stdOUT
        // Need to close the filed we opened to read from
        close(readFD);
    }

    else if(selectCase == 3){
        // SelectCase = 3 -> Read from file, and write to a new file
        // Close both the file being read from, and the file being written to
        close(readFD);
        close(writeFD);
    }

    return errorCondition;



}


struct directoryQueue {
    char directoryPaths[2048][1024];
    int numOfItems;
    int isEmpty;
    pthread_mutex_t lock;
    pthread_cond_t pop_ready;
    int directoryThreadCount;
};

struct queues{
    struct directoryQueue* dirQueue;
    struct wrappingQueue* wrapQueue;
};

struct wrappingQueue {
    char directoryPaths[2048][1024];
    char fileNames[2048][256];
    int numOfItems;
    int isFinished;
    int exitWithError;
    pthread_mutex_t lock;
    pthread_cond_t pop_ready;
};


void* traverse(void *args){
    struct queues* qq = (struct queues*) args;
    struct directoryQueue *dq = (struct directoryQueue*) qq->dirQueue;
    struct wrappingQueue *wq = (struct wrappingQueue*) qq->wrapQueue;
    
    
    int threadIsAsleep = 0;

    int haveLock = 0;
    pthread_mutex_lock(&dq->lock);  
            haveLock = 1;

        while(dq->isEmpty == 0){
            
            while(dq->numOfItems == 0){

                
                if(threadIsAsleep == 0){
                    dq->directoryThreadCount--;
                }
                
                threadIsAsleep = 1;

                
                if(dq->directoryThreadCount == 0){
                    
                    dq->isEmpty = 1;
                    break;
                }

                pthread_cond_wait(&dq->pop_ready,&dq->lock);
                
            }

            if(threadIsAsleep == 1){
                
                threadIsAsleep = 0;
                
                if(dq->directoryThreadCount == 0 && dq->numOfItems == 0){
                    
                    if(haveLock == 1){
                        pthread_mutex_unlock(&dq->lock);
                    }

                    pthread_cond_signal(&dq->pop_ready);
                    pthread_cond_broadcast(&wq->pop_ready);
                    return NULL;
                }

                dq->directoryThreadCount++;
                
            }              

            if(dq->numOfItems > 0){

                // Pop a directoryPath from the top of the directoryQueue
                if(haveLock == 0){
                    pthread_mutex_lock(&dq->lock);
                    haveLock = 1;
                }

                dq->numOfItems--;
                char* directoryPath = malloc(sizeof(char) * (size_t)(6 + strlen(dq->directoryPaths[dq->numOfItems])));
                strcpy(directoryPath,dq->directoryPaths[dq->numOfItems]);
                

                pthread_mutex_unlock(&dq->lock);
                haveLock = 0;
                

                // Open the given directory
                DIR *dir;
                struct dirent *dp;
                dir = opendir(directoryPath);

                // While the directory is open, read the files
                while((dp=readdir(dir)) != NULL){
                    
                    // Get the file name
                    
                    char *fileName = (char*) malloc(sizeof(char) * (size_t)(strlen(dp->d_name) + 2));
                    strcpy(fileName,dp->d_name);
                    int length = strlen(fileName);
                    int skip = 0;
                    

                    // Avoid files that start with "." or "wrap."
                    if(fileName[0] == '.'){
                        
                        skip = 1;
                    }

                    else if(length >= 5){
                        if(strncmp(fileName,"wrap.",5) ==0){
                           
                            skip = 1;
                        }
                    }
        

                    if(skip == 0){
                        int directoryLength = strlen(directoryPath);
                      
                        char* fullFilePath = (char*) malloc(sizeof(char)*(size_t)(directoryLength+length+6));
                        char* backSlash = "/";
                        strcpy(fullFilePath,directoryPath);
                        strcat(fullFilePath,backSlash);
                        strcat(fullFilePath,fileName);                   
                        

                        if(access(fullFilePath,F_OK) != 0){
                            free(fullFilePath);
                            continue;
                        }

                        struct stat sc;
                        stat(fullFilePath,&sc);

                        //S_ISDIR(sc.st_mode) -> check if it is a directory
                        if(S_ISDIR(sc.st_mode)){
                            

                            if(haveLock == 0){
                                pthread_mutex_lock(&dq->lock);
                                haveLock = 1;
                            }
                        

                            strcpy(dq->directoryPaths[dq->numOfItems],fullFilePath);
                            dq->numOfItems++;

                            
                            
                        
                            
                            pthread_mutex_unlock(&dq->lock);
                            haveLock = 0;
                            pthread_cond_signal(&dq->pop_ready);
                        }

                        //S_ISREG(sc.st_mode) -> check if is a regular file
                        else if(S_ISREG(sc.st_mode)){

                            pthread_mutex_lock(&wq->lock);
                                
                                strcpy(wq->directoryPaths[wq->numOfItems],directoryPath);
                                strcat(wq->directoryPaths[wq->numOfItems],backSlash);
                                strcpy(wq->fileNames[wq->numOfItems],fileName);
                                    
                                wq->numOfItems++;

                        
                                

                            pthread_mutex_unlock(&wq->lock);

                            pthread_cond_signal(&wq->pop_ready);

                        }
                        free(fullFilePath);
                    }
                    free(fileName);            
                }
                free(directoryPath);
                closedir(dir);

                if(haveLock == 0){
                    pthread_mutex_lock(&dq->lock);
                    haveLock = 1;
                }
            }

        }
           

    if(haveLock == 1){
        pthread_mutex_unlock(&dq->lock);
    }

    pthread_cond_signal(&dq->pop_ready);
    pthread_cond_broadcast(&wq->pop_ready);


    return NULL;
    


}





void* wrap(void *args){

    struct queues* qq = (struct queues*) args;
    struct directoryQueue *dq = (struct directoryQueue*) qq->dirQueue;
    struct wrappingQueue *q = (struct wrappingQueue*) qq->wrapQueue;

    

    int haveLock = 0;
    pthread_mutex_lock(&q->lock);
    haveLock = 1;
    
    while(q->isFinished == 0){
            

        if(q->numOfItems > 0){
            
            while(q->numOfItems > 0){
                q->numOfItems--;
                char directoryPath[4096];
                char fileName[256];
                strcpy(fileName,q->fileNames[q->numOfItems]);
                strcpy(directoryPath,q->directoryPaths[q->numOfItems]);
                
                pthread_mutex_unlock(&q->lock);
                haveLock = 0;
                pthread_cond_signal(&q->pop_ready);
                
                
                char fullfFilePath[1024];
                strcpy(fullfFilePath,directoryPath);
                strcat(fullfFilePath,fileName);

                int selectCase = 3;
                
                int status = wrapWords(maxWidth,fullfFilePath,selectCase,directoryPath,fileName);

                if(haveLock == 0){
                    pthread_mutex_lock(&q->lock);
                    if(status == 1){
                        q->exitWithError = 1;
                    }
                    haveLock = 1;
                }               
            }

            if(haveLock == 0){
                pthread_mutex_lock(&q->lock);

                if(dq->directoryThreadCount == 0 && q->numOfItems == 0){
                    
                    pthread_mutex_unlock(&q->lock);
                    pthread_cond_broadcast(&q->pop_ready);
                    return NULL;

                }


                haveLock = 1;                
            }

        }

        while(q->numOfItems == 0){

            pthread_mutex_lock(&dq->lock);
            if(dq->directoryThreadCount == 0){
                q->isFinished = 1;
                if(haveLock == 1){
                    pthread_mutex_unlock(&q->lock);
                    haveLock = 0;
                }

                
                pthread_mutex_unlock(&dq->lock);
                pthread_cond_broadcast(&q->pop_ready);

                return NULL;
            }            
            pthread_mutex_unlock(&dq->lock);
            
            pthread_cond_wait(&q->pop_ready,&q->lock);
        }        




    }

    
    if(haveLock == 1){
        pthread_mutex_unlock(&q->lock);
    }
    pthread_cond_broadcast(&q->pop_ready);

    return NULL;
}




int main(int argc,char** argv){


    if(argc == 2){

        int maxWidth = strtol(argv[1],NULL,10);
        
        int status = wrapWords(maxWidth,NULL,1,NULL,NULL);

        if(status == 1){
            exit(EXIT_FAILURE);
        }

    }

    else if(argc == 3){

        char* input = argv[2];
        int maxWidth = strtol(argv[1],NULL,10);

        struct stat sb;
        stat(input,&sb);

        //  Check if the input is regular file
        if(S_ISREG(sb.st_mode)){

            // The directory is the current directory
            char* directoryPath = malloc(sizeof(char)*(size_t)(strlen("./")+2));
            strcpy(directoryPath,"./");

            // Get the fullFilePath to pass to wrapping function
            char* fullFilePath = malloc(sizeof(char)*(size_t)(strlen(directoryPath) + strlen(input) + 6));
            strcpy(fullFilePath,directoryPath);
            strcat(fullFilePath,input);

            // Wrap the text of the file
            int status = wrapWords(maxWidth,fullFilePath,2,directoryPath,input);

            free(directoryPath);
            free(fullFilePath);

            if(status == 1){
                exit(EXIT_FAILURE);
                return 0;
            }
        }

        //  Check if the input is a directory
        else if(S_ISDIR(sb.st_mode)){


            // Get the informatino about the directory
            DIR *dir;
            struct dirent *dp;
            char *fileName;

            char* directoryPath = malloc(sizeof(char) * (size_t) (strlen(input)+6));
            strcpy(directoryPath,"./");
            strcat(directoryPath,input);

            int directoryPathLength = (int) strlen(directoryPath);

            //Open the directory using the command line argument
            dir = opendir(directoryPath);

            // In the writeup we avoid every file that starts with either "." or "wrap."
            char *avoid = "wrap.";


            // We need to keep track of the status of the files that are word wrapped
            // The status of files is kept in fileStatuses, and we keep track of the number of files
            int fileStatuses[10000];
            memset(fileStatuses,0,9999);
            int numOfFiles = 0;

            // Begin looking at the files in the opened directory
            while((dp = readdir(dir)) != NULL){
                // Get the name of the directory and get the file type information
                fileName = dp->d_name;
                char* fullFilePath = malloc(sizeof(char) * (size_t)(directoryPathLength + strlen(fileName) + 6));
                strcpy(fullFilePath,directoryPath);
                strcat(fullFilePath,"/");
                strcat(fullFilePath,fileName);
                
                struct stat sc;
                stat(fullFilePath,&sc);

                // Check if the file is a regular file before opening
                if(S_ISREG(sc.st_mode)){
                    
                    int fileNameLength = strlen(fileName);
                    int skip;
                    
                    // Do not open files that start with "."
                    if(fileName[0] == '.'){
                        continue;
                    }

                    else if(fileNameLength >=5){
                        skip = strncmp(fileName,avoid,5);
                        
                        
                        // The file does not start with "wrap.", therefore wrap the file
                        if(skip!=0){
                            printf("wrapping file\n");
                            fileStatuses[numOfFiles] = wrapWords(maxWidth,fullFilePath,4,directoryPath,fileName);
                            numOfFiles++;
                        }
                    }

                }

                free(fullFilePath);

            }

            free(directoryPath);

            closedir(dir);


            for(int i=0;i<=numOfFiles;i++){
                if(fileStatuses[numOfFiles] == 1){
                    exit(EXIT_FAILURE);
                    return 0;
                }
            }


            return 0;



            
        }


    }

    else if(argc >=4){
        
        // wrapType is the argument "-rM,N"
        char* wrapType = argv[1];
        int wrapTypeLength = (int) strlen(wrapType);

        int createWrapThreadCount = 0;
        int createDirThreadCount  = 0;

        if(wrapTypeLength == 1){
            printf("Error: Invalid input argument, argument should be type of '-rM,N'\n");
            return 0;
        }

        if((strncmp("-r",wrapType,2) != 0)){
            printf("Error: Invalid input argument, argument should be type of '-rM,N'\n");
            return 0;            
        }

        if(wrapTypeLength == 2){
            createWrapThreadCount = 1;
            createDirThreadCount = 1;
        }


        else if(wrapTypeLength == 3){
            createDirThreadCount = 1;
            createWrapThreadCount = atoi(&wrapType[2]);
        }

        else if(wrapTypeLength == 5){
            createDirThreadCount = atoi(&wrapType[2]);
            createWrapThreadCount = atoi(&wrapType[4]);
        }

        // the maxWidth that the files should be wrapped to
        maxWidth = atoi(argv[2]);

        // the name of the input directory
        char* inputDirectory = argv[3];


        // Create the directoryQueue and initialize its fields
        struct directoryQueue *dq = malloc(sizeof(struct directoryQueue));
        dq->numOfItems = 0;
        dq->isEmpty = 0;
        dq->directoryThreadCount = createDirThreadCount;
        pthread_mutex_init(&dq->lock,NULL);
        pthread_cond_init(&dq->pop_ready,NULL);
    

        struct wrappingQueue *wq = malloc(sizeof(struct wrappingQueue));
        wq->numOfItems = 0;
        wq->isFinished = 0;
        wq->exitWithError = 0;
        pthread_mutex_init(&wq->lock,NULL);
        pthread_cond_init(&wq->pop_ready,NULL);
        
        
        struct queues* qq = malloc(sizeof(struct queues));
        qq->dirQueue = dq;
        qq->wrapQueue = wq;



        


        // Get the directory given that the user provided
        int sizeOfInputDir = strlen(inputDirectory);


        // Take the given directory and put it in the form of ./dirName
        char* dirPath = (char*) malloc((sizeof(char)) * (size_t)(sizeOfInputDir+40));
        memcpy(dirPath,"./",2);
        memcpy(dirPath+2,inputDirectory,sizeOfInputDir);
        memcpy(dirPath+2+sizeOfInputDir,"\0",1);

        

        // Add the directory to the directoryQueue
        strcpy(dq->directoryPaths[dq->numOfItems],dirPath);
        dq->numOfItems++;


        // Create an additional thread for traversal
        pthread_t directoryIDS[createDirThreadCount];
        for(int i = 0;i < createDirThreadCount;i++){
            pthread_create(&directoryIDS[i],NULL,traverse,qq);
        }


        // Create pthread for wrapping
        pthread_t wrapperIDS[createWrapThreadCount];
        for(int i = 0;i < createWrapThreadCount;i++){
            pthread_create(&wrapperIDS[i],NULL,wrap,qq);
        }



        

    
        for(int i =0;i<createDirThreadCount;i++){
            pthread_join(directoryIDS[i],NULL);

        }


        
        while(wq->numOfItems != 0){
            //printf("Items reaming:%d\n",wq->numOfItems);
            pthread_cond_signal(&wq->pop_ready);
        }

        

        for(int i =0;i<createWrapThreadCount;i++){
            if(i == 1){
                pthread_cond_broadcast(&wq->pop_ready);
            } 
            pthread_join(wrapperIDS[i],NULL);
           
        }

        int exitWithError = 0;
        if(wq->exitWithError == 1){
            exitWithError = 1;
        }

    
        free(dirPath);
        free(dq);
        free(wq);
        free(qq);

        if(exitWithError == 1){
            exit(EXIT_FAILURE);
        }

    }














}

