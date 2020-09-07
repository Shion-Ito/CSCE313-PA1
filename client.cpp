/*
    Tanzir Ahmed
    Department of Computer Science & Engineering 
    Texas A&M University
    Date  : 2/8/20
 */
#include "common.h" 
#include "FIFOreqchannel.h"
#include <string>
#include <fstream>
#include <sys/time.h>
#include <sys/wait.h>
using namespace std;



int main(int argc, char *argv[]){
   
    ofstream fout; // creating a ofstream variable for writing on file 
    struct timeval start, end; // used for reading time
    double time_taken; //variable to save the time
    int Max_Buffer = MAX_MESSAGE; //Creating a buffer variable in order to be changeable when -m comes

    // below are the variables used for each flag that needs to be recorded 
    
    int choice;
    int person = NULL;
    double time = NULL;
    int ecg = NULL;
    string filename = "";
    bool new_chan = false;
    char* m_size = "";

    //Using getopt to read user choice 
    while((choice = getopt(argc, argv, "cp:t:e:m:f:"))!= -1){
        switch (choice)
            {
            case 'p':
                person = atoi(optarg);
                break;
            case 't':
                time = atof(optarg);
                break;
            case 'e':
                ecg = atoi(optarg);
                break;
            case 'f':
                filename = string(optarg);
                break;
            case 'c':
                new_chan =true;
                break;
            case 'm':
                m_size = optarg;
                break;
            default:
                cout << "unknown character used" <<endl;
                abort();
            }

    }

    //Always forks to create a child process so that we can check for change in the buffer capacity
    int layers = fork(); //The layer number we are in.

    //This is inside the child process
    if( layers == 0){
        if (m_size != "" ){ 
            
            cout << "Child " << layers<<endl;
            cout << "Changing the buffer size to " << m_size << endl;
            char* args[] = {"./server", "-m", m_size , NULL};
            execvp("./server",args);

        }
        else{ 
            cout << "Child " << layers<<endl;
            cout << "Not changing the buffer size" << endl;
            char* args[] = {"./server",NULL};
            execvp("./server",args);   
       
        }
    }


    else{
        if(m_size != ""){
            Max_Buffer = atoi(m_size);
        } 

        FIFORequestChannel chan ("control", FIFORequestChannel::CLIENT_SIDE);

        cout << "The max buffer size is -> " << Max_Buffer << endl;
        // If time and ecg is not inserted

        if(new_chan){
            cout <<"Requesting new Channel..." <<endl;



            MESSAGE_TYPE m = NEWCHANNEL_MSG;
            chan.cwrite (&m, sizeof (MESSAGE_TYPE));
            char name[100];
            chan.cread(name,sizeof(name));

            cout << "New channel name =  " << name << endl;
        
            FIFORequestChannel chan2(name, FIFORequestChannel::CLIENT_SIDE);
            
            char buf [Max_Buffer];
            cout << "Printing out Single Data... -> ";
            datamsg* x = new datamsg (10,0.004,2);
            
            chan2.cwrite (x, sizeof (datamsg)); 
            
            int nbytes = chan2.cread (buf, Max_Buffer);
            double reply = *(double *) buf;
            
            cout << reply << endl;

            delete x;

            // closing the channel    
            MESSAGE_TYPE q = QUIT_MSG;
            chan2.cwrite (&q, sizeof (MESSAGE_TYPE));
        

        }


        if (time != NULL && person!= NULL && ecg != NULL && filename == ""){
            
            char buf [Max_Buffer];

            cout << "Printing out Single Data... -> ";
            fout.open("received/patient.csv");
            datamsg* x = new datamsg (person,time,ecg);
            
            chan.cwrite (x, sizeof (datamsg)); 
            
            int nbytes = chan.cread (buf, Max_Buffer);
            double reply = *(double *) buf;
            
            cout << reply << endl;
            gettimeofday(&end,NULL);
            fout.close();

            time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
            time_taken = (time_taken + (end.tv_usec -  start.tv_usec)) * 1e-6;

            cout << "The time taken is -> " << time_taken <<endl;
            delete x;

        }
        
        if(filename != ""){
            cout << "Record filename -> " << filename << endl;

            // Asking for the file length
            filemsg fm (0,0);
            
            char buf [sizeof (filemsg) + filename.size()+1];
            memcpy (buf, &fm, sizeof (filemsg));
            strcpy (buf + sizeof (filemsg), filename.c_str());
            chan .cwrite(buf, sizeof(buf));

            __int64_t filelen;
            chan.cread (&filelen, sizeof(__int64_t));
            
            // Showing the File Length
            cout << "File Length = " << filelen << " bytes." << endl;





            // Copying the file.
            cout << "Copying the files... " <<  endl;
            int loop = ceil(filelen/Max_Buffer);
            if(filelen%Max_Buffer > 0) loop++;
            cout << "the loop number is " << loop <<endl;


            int windowsize = Max_Buffer;

            fout.open("received/"+filename);
            gettimeofday(&start,NULL);

            for(int i = 0; i< loop; i++){
                if(filelen<Max_Buffer) windowsize = abs(filelen);            
                filemsg f (Max_Buffer*i,windowsize);

                char* recvbuf = new char[windowsize];
                memcpy(recvbuf, &f, sizeof (filemsg));
                strcpy (recvbuf + sizeof (filemsg), filename.c_str());
                chan.cwrite(recvbuf, windowsize);
                chan.cread(recvbuf,windowsize);
                
                fout.write(recvbuf,windowsize);
                filelen-=Max_Buffer;
                delete recvbuf;
            }
            gettimeofday(&end,NULL);
            fout.close();

            time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
            time_taken = (time_taken + (end.tv_usec -  start.tv_usec)) * 1e-6;

            cout << "The time taken is -> " << time_taken <<endl;



        }
       
        // copying 1000 data points depending on person value and ecg output
        if(person != NULL && ecg != NULL && time == NULL){
            

            cout << "Copying data from patient number " << person << endl;
            char buf [Max_Buffer];

            fout.open("received/patient.csv");
            gettimeofday(&start,NULL);
            for(int i = 0; i<1000 ; i++){    
                datamsg x = datamsg (person,.004*i,ecg);


                chan.cwrite (&x, sizeof (datamsg)); 
                int nbytes = chan.cread (buf, Max_Buffer);
                double reply_x = *(double *) buf;

                fout << reply_x <<  endl;
            }

            gettimeofday(&end,NULL);

        
            time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
            time_taken = (time_taken + (end.tv_usec -  start.tv_usec)) * 1e-6;

            cout << "The time taken is -> " << time_taken <<endl;

            fout.close();


        }
    
            // closing the channel    
            MESSAGE_TYPE m = QUIT_MSG;
            chan.cwrite (&m, sizeof (MESSAGE_TYPE));
            wait(0);


}

}

//Sources used: Piazza, geeksforgeeks, textbook