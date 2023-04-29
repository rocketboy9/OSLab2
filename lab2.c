//Header Block

//By: Drew Diettrich
//Purpose of the Project: The purpose of this project is to write code that will help us to understand multithreading using partitions
// We use sorting algorithms to show us that the sorting algorithms will take a shorter amount of time if the program is multithreading 
//as opposed to a single threaded program. In this program you are able to change the size, sorting algorithm and the Threshold of when to switch
//from uqicksort to an alternate sorting algorithm. You are also able to modify the number of pieces or partition, the number of threads being
//used, specify whether you want to sort by median of three in the quicksort and you have the option to start an early thread.
//First you partition and then you quicksort the numbers and then based off the threshold you  switch from the quicksort and use the either
//of the alternate algorithms.

//March 9-10 Writing code to support input from the command line and also wrote code for inputing from the .dat file
//March 11-13 Writing code for the partitioning
//March 14-15 Writing the code for Insertion and Shell sort and implementing multithreading
//March 16 finalizing and fixing bugs regarding my code for multithreading
//March 17 Implementing Median of three and Early
//March 18-20 fixing bugs 
//March 20-22 running scripts and fixing minor bugs along the way
//March 22 Finally done 


#define _GNU_SOURCE

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>


//Global variables for each of the components for the project
int SIZE = -1;          //SIZE is required so if it still -1 after the inputs get set then the program will return
bool ALTERNATE = true;  // s/shell/true    i/insertion/false    Defaulted to shell sort
int THRESHOLD = 10;     //default threshold value is 10
int SEED = -1;          //if this value is still -1 after the for loop, then it will become a rondom integer
bool MULTITHREAD = true;//default value of multithreading is true
int PIECES = 10;        //default number of pieces is 10
int MAXTHREADS = 4;     //default maxthreads is 4. If it equals 1 then MULTITHREAD == FALSE
bool MEDIAN = false;    //default median to false
bool EARLY = false;     //default to false for starting early

void Partition();//thread function for quicksort
void InsertionSort(int lo, int hi);
bool IsSorted();
bool IsSectionSorted(int lo, int hi);
void Quicksort(int lo, int hi);
void *Runner(void *param);
void InsertionSortForPartitions();
void newShellSort(int lo, int hi);

int *array;//number array that gets all the read in numbers
struct HiLo *HiLoArray;//Array to hold all of the partitions

struct HiLo{//struct for the partitions
    int hi;
    int lo;
    int indexToReplace;
    int diffOfHiLo;
};

struct HiLo EarlyPartition;//This struct holds the value of the segment in the early partition if early is enabled

int main(int argc, char* argv[], char* env[]){

    //--------------------------------------------------------------------------------------------------------------------------
    //Code that is used for gathering command line input
    for(int i = 1; i<argc; i+=2){//for loop for setting values for the program parameters
        char* current = argv[i];//Specifies the current command

        if(strcmp(current, "-n") == 0){//If statement for setting the size
            SIZE = atoi(argv[i+1]);//adds 1 to i to see the actual value that is specified by -n
        }
        else if(strcmp(current, "-a") == 0){//if statement for setting the alternative sorting algorithm
            if(strcmp(argv[i+1], "i") == 0 || strcmp(argv[i+1], "I") == 0){
                ALTERNATE = false;
            }
        }
        else if(strcmp(current, "-s") == 0){//if statement for setting the threshold
            THRESHOLD = atoi(argv[i+1]);
        }
        else if(strcmp(current, "-r") == 0){//if statement for setting the seed
            SEED = atoi(argv[i+1]);
        }
        else if(strcmp(current, "-m") == 0){//if statement for specifying whether or not to multithread
            if(strcmp(argv[i+1], "n") == 0 || strcmp(argv[i+1], "N") == 0){
                MULTITHREAD = false;
            }
        }
        else if(strcmp(current, "-p") == 0){//specifies how many partitions to divide the original list into
            PIECES = atoi(argv[i+1]);
        }
        else if(strcmp(current, "-t") == 0){//specifies max number of threads
            MAXTHREADS = atoi(argv[i+1]);
        }
        else if(strcmp(current, "-m3") == 0){//determines whether each segment will be partitioned using the MEDIAN-OFTHREE technique
            if(strcmp(argv[i+1], "y") == 0 || strcmp(argv[i+1], "Y") == 0){
                MEDIAN = true;
            }
        }
        else if(strcmp(current, "-e") == 0){//Determines whether we start threading right after the first partition is made
            if(strcmp(argv[i+1], "y") == 0 || strcmp(argv[i+1], "Y") == 0){
                EARLY = true;
            }
        }
    }
    //--------------------------------------------------------------------------------------------------------------------------

    if(SIZE < 1){
        printf("Size is a required parameter\nexiting\n");
        return 0;
    }

    if(SEED == -1){//code for generating a random number as the seed for the program if the seed is uninitialized
        struct timeval randomValue;
        srand(gettimeofday(&randomValue, NULL));//this code seeds the random number generator with a random number
        SEED = rand() % 1000000000;
    }

    clock_t start = clock();

    struct timeval totalStartTime, totalEndTime; //initializes the start and end times for the total times 
    gettimeofday(&totalStartTime, NULL);
    struct timeval loadStartTime, loadEndTime; //initializes the start and end times for the loading times 
    gettimeofday(&loadStartTime, NULL);


    array = (int*) malloc(SIZE * sizeof(int));//creates space for the array to adjust for the SIZE specified by the user
    HiLoArray = (struct HiLo*) malloc(PIECES * sizeof(struct HiLo));

    FILE *file;     //these three lines of code read from the file and put the numbers into the array variable
    file = fopen("random.dat", "rb");
    long seed = SEED * sizeof(int);//depending on the seed the numbers will be read differently from the file
    

    fseek(file, seed, SEEK_SET);//looks for a place in the file to start reading numbers
    size_t firstReadSize = fread(array, sizeof(int), SIZE, file);//reads from the file and puts the values into the array pointer

    if(firstReadSize<SIZE){//If the full SIZE is not read in that means that the reader hit the end of the file
        int whatsLeft = SIZE-firstReadSize;//To read more I start at 0 of the file and start readingm more of them until
        fseek(file, 0, SEEK_SET);           // the array has SIZE numbers
        size_t secondReadSize = fread(&array[firstReadSize], sizeof(int), whatsLeft, file);
    }

    gettimeofday(&loadEndTime, NULL);

    struct timeval sortStartTime, sortEndTime; //initializes the start and end times for the real times 
    gettimeofday(&sortStartTime, NULL);
    
   
    pthread_t tidEarly;
    pthread_attr_t attrEarly;
    if(MULTITHREAD){

        //This section of code is for the early functionality. I did it exactly how u said to do it in the directions
        //It finds the second biggest number out of 11 values in different location in the code
        if(EARLY){
            int locations[11];//initializing arrays for early
            int values[11];

            int tenPercent = .1 * (double)SIZE;//This block of code is calculating an index for each percentage for the size of array
            int twentyPercent = .2 * (double)SIZE;
            int thirtyPercent = .3 * (double)SIZE;
            int fourtyPercent = .4 * (double)SIZE;
            int fiftyPercent = .5 * (double)SIZE;
            int sixtyPercent = .6 * (double)SIZE;
            int seventyPercent = .7 * (double)SIZE;
            int eightyPercent = .8 * (double)SIZE;
            int ninetyPercent = .9 * (double)SIZE;

            locations[0] = 0;//locations array with the values index
            locations[1] = tenPercent;
            locations[2] = twentyPercent;
            locations[3] = thirtyPercent;
            locations[4] = fourtyPercent;
            locations[5] = fiftyPercent;
            locations[6] = sixtyPercent;
            locations[7] = seventyPercent;
            locations[8] = eightyPercent;
            locations[9] = ninetyPercent;
            locations[10] = SIZE - 1;

            values[0] = array[0];//values array with the values of the indexes of each 10%
            values[1] = array[tenPercent];
            values[2] = array[twentyPercent];
            values[3] = array[thirtyPercent];
            values[4] = array[fourtyPercent];
            values[5] = array[fiftyPercent];
            values[6] = array[sixtyPercent];
            values[7] = array[seventyPercent];
            values[8] = array[eightyPercent];
            values[9] = array[ninetyPercent];
            values[10] = array[SIZE - 1];

            int currentMax;//finds the max

            for(int x = 0; x<11; x++){
                if(x==0){
                    currentMax = values[x];
                }
                if(values[x] > currentMax){
                    currentMax = values[x];
                }
            }

            int secondBiggest;//variable for the second max
            int location;
            for (int i = 0; i < 11; i++)//finds the second max
            {
                if(i==0 && currentMax != values[0]){
                    secondBiggest = values[0];
                }
                else if(i == 0){
                    secondBiggest = 0;
                }

                if(i != 0 && values[i] > secondBiggest && values[i] != currentMax){
                    secondBiggest = values[i];
                    location = i;
                }
            }

            
            int temp;

            //swap array[location of second biggest Number in the values array in the main array] with array[0]
            temp = array[0];
            array[0] = array[locations[location]];
            array[locations[location]] = temp;

        }

        
        int EarlyThreadIdentifier = -1;//If thread is early this will be inputed into the pthread_create() function to let it know 
                                        //to run the early thread

        for(int x = 0; x<PIECES-1; x++){//This loop is for the partitioning. If the code has early enabled then it will go into that if statement and
            Partition();                //start and early thread.

            if(EARLY && MULTITHREAD && x == 0){
                EarlyPartition = HiLoArray[1];
                HiLoArray[1].hi = 0;
                HiLoArray[1].lo = 0;
                HiLoArray[1].diffOfHiLo = 0;
                HiLoArray[1].indexToReplace = 0;

                printf("EARLY Launching %d to %d (%.2f percent)\n", EarlyPartition.lo, EarlyPartition.hi, ((double)(EarlyPartition.hi - EarlyPartition.lo) /(double)SIZE)*100);
                
                pthread_attr_init(&attrEarly);
                pthread_create(&tidEarly, &attrEarly, Runner, EarlyThreadIdentifier);//Creates a thread called runner that runs the sorts

                Partition();//creates an additional partition because the EARLY functionality took a piece
            }
        }

        //Organize the hilo array by size
        InsertionSortForPartitions();

        for(int x = 0; x<PIECES; x++){//prints partitions and there percentages
            printf("%2d: %10d - %10d ( %10d - %5.2f)\n", x, HiLoArray[x].lo, HiLoArray[x].hi+1, (HiLoArray[x].hi+1) - HiLoArray[x].lo, ((double)((HiLoArray[x].hi+1) - HiLoArray[x].lo)/SIZE) *100.0);
        }
    }

    pthread_t tid[MAXTHREADS];//Create MAXTHREADS threads in an array in order to keep them organized
    pthread_attr_t attr[MAXTHREADS];
    int offset = 0;
    
    if(MULTITHREAD == false){//If multithreading is false run quicksort over the whole array
        Quicksort(0, SIZE);
    }
    else{
        //This code is for multithreading. It loops to create PIECES threads and can only have MAXTHREADS running at a time
        int availableThread = -1;//set to -1 when the loop is looking for a thread that is available
        for(int z = 0; z<PIECES; z++){//loops through all of the pieces
            if(z>=MAXTHREADS){
                
                while(true){//this while(true) loop looks for a thread that is not busy
                    availableThread = -1;
                    for(int t = 0; t<MAXTHREADS; t++){//loops through the available threads
                        if(pthread_tryjoin_np(tid[t], 0) != EBUSY){//If a thread is busy then pthread_tryjoin_np will return 16 or EBUSY
                            availableThread = t;//next thread to be used for the the next pthread_create()
                            pthread_attr_init(&attr[availableThread]);
                            pthread_create(&tid[availableThread], &attr[availableThread], Runner, z);
                            //printf("creating thread based on available \nThreadID: %d assigned to PIECE: %d\n", availableThread, z);
                            break;
                        }
                    }

                    if( availableThread != -1){//if there is a candidate for the next thread then available thread will not be -1
                        break;                  //and it will break from the while(true) loop
                    }
                    else{
                        usleep(100000);         //used for managing the polling
                    }
                }
            }
            
            if(z<MAXTHREADS){//creates a thread while MAXTHREADS has not been reached
                pthread_attr_init(&attr[z]);
                pthread_create(&tid[z], &attr[z], Runner, z);
            }
        }

        for (int x = 0; x<MAXTHREADS; x++){//Attempts to join the threads that might still be working for some reason
            pthread_join(tid[x], NULL);     
            pthread_tryjoin_np(tid[x], 0);
        }

    }


    //This code checks to see if the EARLY thread has completed
    int check;
    if(EARLY && MULTITHREAD){
        check = pthread_join(tidEarly, NULL);

        if(check){
            printf("EARLY THREAD STILL RUNNING AT END\n");
        }
        else{
            printf("EARLY THREAD COMPLETED EXECUTION\n");
        }
    }
    //This is code for when to stop the time so the program can print out the time values
    gettimeofday(&sortEndTime, NULL);
    gettimeofday(&totalEndTime, NULL);
    clock_t end = clock();


    //This chunk of code changes the time values into values 
    double cpuTime = (double)(end-start)/sysconf(_SC_CLK_TCK);
    cpuTime = cpuTime/10000;

    int secondsDiff = totalEndTime.tv_sec - totalStartTime.tv_sec;
    int microsecondsDiff = totalEndTime.tv_usec - totalStartTime.tv_usec;
    int milliseconds = ((1000*secondsDiff)+(microsecondsDiff/1000));
    double totalResult = (double)milliseconds/1000;

    secondsDiff = sortEndTime.tv_sec - sortStartTime.tv_sec;
    microsecondsDiff = sortEndTime.tv_usec - sortStartTime.tv_usec;
    milliseconds = ((1000*secondsDiff)+(microsecondsDiff/1000));
    double sortResult = (double)milliseconds/1000;

    secondsDiff = loadEndTime.tv_sec - loadStartTime.tv_sec;
    microsecondsDiff = loadEndTime.tv_usec - loadStartTime.tv_usec;
    milliseconds = ((1000*secondsDiff)+(microsecondsDiff/1000));
    double loadResult = (double)milliseconds/1000;

    //Used for printing the time that the program took to run
printf("Load: %8.3f Sort (Wall/CPU): %8.3f / %8.3f  Total: %8.3f\n", loadResult, sortResult, cpuTime, totalResult);

    //IsSorted();
    if (!IsSorted()) printf("ERROR – Data Not Sorted\n");//Checks if the code is sorted
}

//Thread function that deals with partitioning the number array into different parts
//The first part checks to see if there are entries in the high low array yet. If there
//aren't then it will put both parts into the array. If there are already things in the
//array then change one entry and add one entry on to the end of the array.
//Also it will partition based on the size of the array.
void Partition(){
    
    struct HiLo hilo;
    int maxSegment = 0;
    int maxSegmentIndex = 0;

    if(HiLoArray[0].hi == 0 && HiLoArray[0].lo == 0){
        hilo.hi = SIZE - 1;
        hilo.lo = 0;
        hilo.indexToReplace = 0;
    }
    else{
        for(int x = 0; x<PIECES; x++){
            if(HiLoArray[x].hi != 0 || HiLoArray[x].lo != 0){
                if(maxSegment < (HiLoArray[x].hi - HiLoArray[x].lo)){
                    maxSegment = HiLoArray[x].hi - HiLoArray[x].lo;
                    maxSegmentIndex = x;
                }
            }
        }

        hilo = HiLoArray[maxSegmentIndex];
    }

    printf("Partitioning %10d - %10d (%10d)...", hilo.lo, hilo.hi, hilo.hi - hilo.lo);

    int temp;
    //partition the array
    int i = hilo.lo;
    int j = hilo.hi + 1;
    int x = array[i];

    if(MEDIAN == true && (hilo.hi+1-hilo.lo)>5){
        int leftMost = array[i];
        int rightMost = array[hilo.hi];
        int middle = array[((j-i)/2) + hilo.lo];//sets the values of the left right and middle values
        
        if(leftMost>rightMost && leftMost<middle ||
        leftMost<rightMost && leftMost>middle){//leftMost is median
            x=leftMost;
        }
        else if(rightMost>leftMost && rightMost<middle||
        rightMost<leftMost && rightMost>middle){//rightmost is median. swap rightmost with array[0]
            temp = array[hilo.hi];
            array[hilo.hi] = array[i];
            array[i] = temp;

            x = array[i];
        }
        else if(middle>rightMost && middle<leftMost ||
        middle<rightMost && middle>leftMost){//middle is median. swap middle with array[0]
            array[((j-i)/2) + hilo.lo] = array[i];
            array[i] = middle;

            x = array[i];
        }
    }

    
    do{
        do i++; while (array[i] < x);//code from the lecture slides on how to partition
        do j--; while (array[j] > x);
        if(i<j){
            temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
        else{
            break;
        }
    } while(true);//J is the pivot value

    temp = array[hilo.lo];
    array[hilo.lo] = array[j];
    array[j] = temp;

    //creates HiLo values in place of previous values in the array
    HiLoArray[hilo.indexToReplace].hi = j-1;
    HiLoArray[hilo.indexToReplace].lo = hilo.lo;
    HiLoArray[hilo.indexToReplace].diffOfHiLo = (j-1) - hilo.lo;

    //Finds a HiLoArray spot that has not been used yet
    for(int x = 0; x<PIECES; x++){
        if(HiLoArray[x].hi == 0 && HiLoArray[x].lo == 0){
            HiLoArray[x].hi = hilo.hi;
            HiLoArray[x].lo = j+1;
            HiLoArray[x].indexToReplace = x;
            HiLoArray[x].diffOfHiLo = hilo.hi - (j+1);
            break;
        }
    } 

    //prints the partitions and their data as they're being created
    printf("result: %10d - %10d (%5.2f / %5.2f)\n", (j-1) - hilo.lo, hilo.hi - (j+1), (double)((j-1) - hilo.lo) *100 / (hilo.hi-hilo.lo), (double)(hilo.hi - j+1) / (hilo.hi-hilo.lo) *100);
    
}

//This is my quicksort function. The first if statement checks if lo == hi(return if true) and it checks if (hi-lo < THRESHOLD)
//If this is true then the segment of numbers will immediately sort with the alternative sorting algorithm
//The next chunk of code is the partitioning part. If median of three is enabled than it will use that functionality for more equal partitioning
//The next chunk of code looks at the newly made partitions and decides whether to leave the segment of numbers alone, switch 2 numbers, use the
//alternate sorting algorithm or using quicksort again if the threshold isn't met yet. Since Quicksort is recursive, it will recur unless
//the threshold is met, if the threshold is met then the segment of code will go into the alternate sorting algorithm.
void Quicksort(int lo, int hi){

    if(lo == hi){//if same value: return
        return;
    }
    else if((hi-lo)<THRESHOLD){//if size less than threshold: use alternative sorting algorithm and then return since everything should be sorted
        if(ALTERNATE == true){
            newShellSort(lo, hi);
        }
        else if(ALTERNATE == false){
            InsertionSort(lo, hi);
        }

        return;
    }

    //In this part of the code I put values into different variables
    struct HiLo hilo;
    int temp;
    int i = lo;
    int j = hi;
    int x;

    x = array[i];//if median of three is not true then the value = array[0]
    
    do{
        do i++; while (array[i] < x);//code from the lecture slides on how to partition
        do j--; while (array[j] > x);
        if(i<j){
            temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
        else{
            break;
        }
    } while(true);//J is the pivot value
    temp = array[lo];
    array[lo] = array[j];
    array[j] = temp;

    if(IsSectionSorted(lo, j) == true || lo == (j-1)){//if section is already sorted do nothing
        //do nothing
    }
    else if(((j-1) - lo) == 1){//if two values are out of order, switch them
        temp = array[j-1];
        array[j-1] = array[lo];
        array[lo] = temp;
    }
    else if(((j-1) - lo) < THRESHOLD){//If hi - lo is less than threshold then use the alternative sorting algorithm
    
        if(ALTERNATE == true){
            newShellSort(lo, j);
        }
        else if(ALTERNATE == false){
            InsertionSort(lo, j);
        }
    }
    else{
        Quicksort(lo, j-1+1);//If none of this is true then enter the values into quicksort
    }

    if(IsSectionSorted(j+1, hi) == true || hi == (j+1)){//if section is already sorted do nothing
        //do nothing
    }
    else if((hi - (j+1)) == 1){//if two values are out of order, switch them
            temp = array[j+1];
            array[j+1] = array[hi];
            array[hi] = temp;
        }
    else if ((hi - (j+1)) < THRESHOLD){//If hi - lo is less than threshold then use the alternative sorting algorithm
        if(ALTERNATE == true){
            newShellSort(j+1, hi);
        }
        else if(ALTERNATE == false){
            InsertionSort(j+1, hi);
        }
    }
    else{//If none of this is true then enter the values into quicksort
        Quicksort(j+1, hi);
    }
}

//takes a high and low value of array locations and sorts the values in between using insertion sort
void InsertionSort(int lo, int hi){
    int size = hi-lo;
    int key;
    int current;

    for (int x = lo; x<size+lo; x++){
        key = array[x];
        current = x - 1;

        while(current>=lo && array[current] > key){
            array[current+1] = array[current];
            current = current - 1;
        }
        array[current + 1] = key;
    }
}

//Hibbards sequence with Shell Sort
//This is modified code from the lectures in order to take in a high and a low value
void newShellSort(int lo, int hi){
    int temp;
    int N = hi-lo;
    int k = 1; while (k <= N) k *= 2; k = (k / 2) - 1;//this is code for hibbards sequence
    do {    
        for (int i = lo; i < ((N+lo) - k); i++) // for each comb position
        for (int j = i; j >= lo; j -= k)    // Tooth-to-tooth is k
            if (array[j] <= array[j + k]) break; // move upstream/exit?
            else{
                temp = array[j];
                array[j] = array[j+k];
                array[j+k] = temp;
                //swap X[j] and X[j + k];
            }
        
        k = k >> 1; // or k /= 2;
    }
    while (k > 0 );

}

//Checks if the array is sorted. returns true if sorted. false if not
bool IsSorted(){
    bool sorted = true;
    int temp;
    printf("sorted?:");

    for (int i = 0; i < SIZE; i++)
    {
        if(i==0){
            temp = array[i];
            continue;
        }

        if(temp>=array[i]){
            return false;
        }
        temp = array[i];
    }
    printf("yes\n");
    return true;
}

//checks if a segment of the array is sorted
//I use this in quicksort to see if a segment still needs sorted.
bool IsSectionSorted(int lo, int hi){

    bool isSorted = false;
    int temp;
    int size = hi - lo;
    for (int x = lo; x<size+lo; x++){
        if(x==lo){
            temp = array[x];
            continue;
        }

        if(temp>=array[x]){
            return false;
        }
        temp = array[x];
    }
    return true;
}

//For multithreading quicksorts
//takes in an integer that is the index of the HiLoArray. HiLoArray holds the segments of the array that need to be sorted
//If early is enabled then the EARLY thread will have the param == -1
void *Runner(void *param){

    int x  = (int*) param;
    if(x == -1 && EARLY == true){
        Quicksort(EarlyPartition.lo, EarlyPartition.hi+1);
    }
    else{
        //prints when it is launching a thread and which thread and how big it is
        printf("Launching thread to sort %10d - %10d (%10d): (row %4d)\n", HiLoArray[x].lo, HiLoArray[x].hi+1, (HiLoArray[x].hi+1) - (HiLoArray[x].lo), x);
        Quicksort(HiLoArray[x].lo, HiLoArray[x].hi+1);
    }

    pthread_exit(NULL);//Exits the thread
}

//I made this function to sort the HiLo array by size so that it would sort the big pieces first
void InsertionSortForPartitions(){
    int size = PIECES;

    struct HiLo key;
    int current;

    for (int x = 1; x<size; x++){
        key = HiLoArray[x];
        current = x - 1;
                
        while(current>=0 && HiLoArray[current].diffOfHiLo < key.diffOfHiLo){
            HiLoArray[current+1] = HiLoArray[current];
            current = current - 1;
        }
        HiLoArray[current + 1] = key;
    }
}
