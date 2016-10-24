/* Implementing link-state protocol over UDP, using socket programming. Also creation of LSA packets to demonstrate basic LSA flooding mechanism
:Test_LSP_1.cpp : Defines the entry point for the console application.
: Test_Server_LSP_2.cpp : Defines the entry point for the console application.
: Packet format, bytes: 1-protocol, 1-type,1-router_id1, 1-router_id2, 1-link_cost, 1-sequence#, 1-age
: Routing table format : routing table-{|router-id1(1)|routerid2(1)|cost(1)|sequence#(1)|age(1)]
*/


#include "stdafx.h"
#include<stdio.h>
#include<iostream>
#include<fstream>
#include <process.h> 
#include<string>
#include<mutex>
#include<winsock2.h>
#include "conio.h"
#include<vector> 
#pragma comment(lib,"ws2_32.lib") 
#define PACKETLEN 7
#define SERVER "127.0.0.1"
#define R1LISTENPORT 12345
#define R2LISTENPORT 12346
#define R3LISTENPORT 12347
#define R4LISTENPORT 12348
#define R5LISTENPORT 12349
#define R6LISTENPORT 12350
#define R7LISTENPORT 12351
#define R8LISTENPORT 12352
#define R9LISTENPORT 12353
#define R10LISTENPORT 12354

using namespace std;
int router_matrix[10][10];
char routers[10][10] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
int my_id;
int router_count = 0;
int send_matrix[10];
int send_matrix_new[10];
unsigned short router_listen_ports[10] = { R1LISTENPORT, R2LISTENPORT, R3LISTENPORT, R4LISTENPORT,
R5LISTENPORT, R6LISTENPORT, R7LISTENPORT, R8LISTENPORT, R9LISTENPORT, R10LISTENPORT };
char routing_table[30][5];
long routing_table_entry_timer_old[30];
long routing_table_entry_timer_new[30];
unsigned long timeout = 11000;
int routing_table_entry_count = 0;
unsigned short my_port;
std::mutex mtx;


/* write a function that splits any strings into tokens */

vector<string> splitString(string input, string delimiter)
{
  vector<string> output;
  size_t start = 0;
  size_t end = 0;

  while (start != string::npos && end != string::npos)
   {
     start = input.find_first_not_of(delimiter, end);

     if (start != string::npos)
      {
        end = input.find_first_of(delimiter, start);

        if (end != string::npos)
           {
             output.push_back(input.substr(start, end - start));
           }
        else
           {
            output.push_back(input.substr(start));
           }
       }
    }
  return output;
}

int compareMem(void * p1, void *p2, int len)
{
   char *ptr1 = (char*)p1;
   char *ptr2 = (char*)p2;
   for (int c = 0; c < len; c++)
     {
       if (*ptr1 != *ptr2)
       return -1;
       ptr1++;
       ptr2++;
     }
    return 0;
}

/* write a function to read config file and initailaize the router */

void initializeRouter()
{

   memset(routing_table, -1, sizeof(routing_table));//initialize routing table with -1
   ifstream config;
   config.open("config.txt");
   string line;
   int line_number = 0;
   int row = 0, col = 0;
   while (1)
    {
      line_number++;
      getline(config, line);
      if (line_number == 1)
        {
          my_id = stoi(line);
        }
    else
        {
          vector <string> tokens;
          string str;
          tokens = splitString(line, ",");
          col = 0;
          for (auto it = begin(tokens); it != end(tokens); ++it)
             {
               str = *it;
               router_matrix[row][col++] = stoi(str);
             }
          row++;
        }
    if (config.eof())
        break;
}

my_port = router_listen_ports[my_id];
/*prepare send matrix */
for (int c = 0, entry = 0; c < 10; c++)
   {
     send_matrix[c] = router_matrix[my_id][c];
     /* insert initial info in routing table 
     routing table-{|router-id1(1)|routerid2(1)|cost(1)|sequence#(1)|age(1)]* /
    if (send_matrix[c] > 0)
       {
         routing_table[entry][0] = (char)my_id;
         routing_table[entry][1] = (char)c;
         routing_table[entry][2] = (char)send_matrix[c];
         routing_table[entry][3] = (char)1;
         routing_table[entry++][4] = (char)60;
       }
   }
/* print initial routing table
cout << endl << "Initial routing table" << endl;
for (int r = 0, c = 0; r < 30; r++)
    {
     if (routing_table[r][0] != (char)-1)
        {
             ::printf("\LinkID1:%d, LinkID2:%d, Cost:%d, Sequence:%d, Age:%d\n", routing_table[r][0], routing_table[r][1],
             routing_table[r][2], routing_table[r][3], routing_table[r][4]);
        }
     config.close();
   }
*/

/* write a function that will implement the thread which will keep listening from the clients */
unsigned int __stdcall receiveThread(void*)
   {
     WSADATA wsa;
     SOCKET s;
     struct sockaddr_in server, si_other;
     int slen, recv_len;

     char packet[PACKETLEN];

     /* Initializing the socket library */
     printf("Receive:Initialising Winsock...\n");

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
       {
         printf("Receive:Failed. Error Code : %d\n", WSAGetLastError());
         return 1;
       }
    printf("Receive:Initialised.\n");

    /* create UDP socket*/
    if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
       { 
          printf("Receive:Could not create socket : %d\n", WSAGetLastError());
       }
      printf("Receive:Socket created.\n");

    /* Initialize UDP socket */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = my_port;

   /* Bind the socket with structure */
   if (::bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
      {
         printf("Receive:Bind failed with error code : %d\n", WSAGetLastError());
         exit(EXIT_FAILURE);
      }
    puts("Receive:Bind done\n");
    slen = sizeof(si_other);

   /* keep listening for data */
   while (1)
      {
        printf("\nReceive:Waiting for data...\n");
        fflush(stdout);

        /* clear the buffer by filling null, it might have previously received data */
        memset(packet, '\0', PACKETLEN);

        / * If iMode==0, blocking mode is enabled.*/
        u_long iMode = 1;
        ioctlsocket(s, FIONBIO, &iMode);

        /* try to receive some data, for a non blocking call */
        recv_len = recvfrom(s, (char *)packet, PACKETLEN, 0, (struct sockaddr *) &si_other, &slen);

        for (int counter = 0; counter<30; counter++)
             {
                routing_table_entry_timer_new[counter] = GetTickCount();
              }

        /* check for stale entries and also decrement the age */ 
        mtx.lock();
        for (int counter = 0; counter < 30; counter++)
           { 
             if (routing_table[counter][0] == my_id)
             continue;
             /* to find an entry */
             else if (routing_table[counter][0] != -1)
                   {
                     if ((routing_table_entry_timer_new[counter] - routing_table_entry_timer_old[counter]) > timeout)
                       {
                         /* decrement age by 1 */
                         --routing_table[counter][4];
                           routing_table_entry_timer_old[counter] = GetTickCount();
                           ::printf("\n age decremented");
                       }
                    /* entry is expired */
                     if (routing_table[counter][4] <= 0)
                         routing_table[counter][0] = -1;
                         break;
                    }
           }
        mtx.unlock();

/* {
      ::printf("Receive:recvfrom() failed with error code : %d\n", WSAGetLastError());
      getchar();
      exit(EXIT_FAILURE);
   }
*/

  mtx.lock();
  if (packet[1] == 1) //LSA packet
    {
      ::printf("Receive:Received LSA packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
      printf("\nSourceRouter:%d, LinkID1:%d, LinkID2:%d, Cost:%d, Sequence:%d, Age:%d\n", packet[2], packet[3],packet[4], packet[5], packet[6], packet[7]);
      int flag_found = 0;
      /* update router_matrix with current cost */
      char entry[2] = { packet[2], packet[3] };
      /* redundant info from me to neighbor which I have */
      if (entry[0] == my_id) 
         { 
           / * some neighbor is sending me info about myself, I will ignore */ 
           mtx.unlock();
           continue;
         }

      for (int counter = 0; counter < 30; counter++)
        {
           /* case 1: entry exists with same or different costs */ 
           int val = ::memcmp(&routing_table[counter][0], (void *)entry, 2);
           ::printf("\n%d%d%d", routing_table[counter][0], routing_table[counter][1], routing_table[counter][2]); 
           ::printf("\n%d%d%d", entry[0], entry[1], entry[2]); 
           cout << "\nval is:" << val << endl;

           if (val == 0)
            {
                /* entry found */
                if (routing_table[counter][3] < packet[5])
                   {
                   / * received an updated info about already existing link with higher sequence number */
                   packet[6] = packet[6] - 1;//reduce the age
                   memcpy(&routing_table[counter][0], &packet[2], 5);//update the entry
                   routing_table_entry_timer_old[counter] = GetTickCount();
                   }
              else if (routing_table[counter][3] > packet[5]);
              else 
                   {
                     / * entry exists but with same sequence number */
                    if ((packet[6] - 1) <= 0)
                      routing_table[counter][0] = -1;
                    else if (routing_table[counter][4] > (packet[6] - 1))
                           {
                             /* printf("\nLess Age received, updating the entry...\n"); */
                                packet[6] = packet[6] - 1;
                                if (packet[6]<=0)
                                    routing_table[counter][0] = -1;
                                else
                                    memcpy(&routing_table[counter][0], &packet[2], 5);//update the entry
                           }
                   }
                flag_found = 1;
                break;
            }
        }

    if (!(flag_found == 1))
       {
       / *case 2: if not found, then insert */
       /* find an empty slot */
       for (int counter = 0; counter < 30; counter++)
          {
            if (routing_table[counter][0] == (char)-1)
                {
                  /* to find an empty slot */
                  ::memcpy(&routing_table[counter][0], &packet[2], 5);//copy entry to the empty slot in routing table
                  routing_table_entry_timer_old[counter] = GetTickCount();
                  break;
                }
           }
       }
     flag_found = 0;
   }
   for (int counter = 0; counter < 30; counter++)
       {
         if (routing_table[counter][0] != (char)-1)
            {
              /*to find an entry */
              if (routing_table[counter][4] <= 0)
              /*entry is expired*/
                  routing_table[counter][0] = -1;
                  break;
            }
       }
/* for (int r = 0, c = 0; r < 30; r++)
      {
        if (routing_table[r][0] != -1)
           ::printf("\nLinkID1:%d, LinkID2:%d, Cost:%d, Sequence:%d, Age:%d\n", routing_table[r][0], routing_table[r][1],routing_table[r][2], routing_table[r][3], routing_table[r][4]);
      } */
    mtx.unlock();
    /* Sleep(1000); */
    }
  closesocket(s);
  WSACleanup();
  return 0;
}



/* write a function that will implement thread to send messages */
unsigned int __stdcall sendThread(void*)
     {
        struct sockaddr_in si_other;
        int s, slen = sizeof(si_other);
        WSADATA wsa;
        char packet[PACKETLEN];

        /* Initialise winsock */
        printf("Send:Initialising Winsock...\n");
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
           {
             printf("Send:Failed. Error Code : %d\n", WSAGetLastError());
             exit(EXIT_FAILURE);
           }
         printf("Send:Initialised.\n");

        /* create socket */
        if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
           {
             printf("Send:socket() failed with error code : %d\n", WSAGetLastError());
             exit(EXIT_FAILURE);
           }
        /* 01110110 in protocol number represents SLSP packet, constant for each packet */
        memset(packet, '\0', PACKETLEN);

        / * start communication , create the packet you want to send */
        packet[0] = 118;

        /* Now send LSA in loop */
        while (1)
           {
            / *make the packet you want to send, LSA type */
            packet[1] = 1;

            /*create packets for each routing table entry and send */
            mtx.lock();
            ::printf("\nSend Matrix::\n");
            for (int counter2 = 0; counter2 < 10; counter2++)
                {
                 ::printf("%d,", send_matrix[counter2]);
                }

            for (int c = 0; c < 10; c++)//for each router
                {
                  if (send_matrix[c] > 0)
                     {
                      /* setup address structure for each receiver, i.e. change port */
                      memset((char *)&si_other, 0, sizeof(si_other));
                      si_other.sin_family = AF_INET;
                      si_other.sin_port = router_listen_ports[c];
                      si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);

                       /* increment the sequence number */
                       for (int counter = 0; counter < 30; counter++)
                           {
                              if (routing_table[counter][0] == my_id)
                              {
                                /*routing table entry exists, -1 represents there is no entry*/
                                if (routing_table[counter][3]>=126)
                                    routing_table[counter][3]=0;
                                else
                                    routing_table[counter][3]++;
                              }
                            }
                        for (int counter = 0; counter < 30; counter++)
                            {
                              if (routing_table[counter][0] != (char)-1 && routing_table[counter][4] > 0)
                                 {
                                   /*routing table entry exists, -1 represents there is no entry */
                                   packet[2] = routing_table[counter][0];
                                   packet[3] = routing_table[counter][1];
                                   packet[4] = routing_table[counter][2];
                                   packet[5] = routing_table[counter][3];
                                   packet[6] = routing_table[counter][4];
                                   if (sendto(s, (const char*)packet, PACKETLEN, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
                                      {
                                          printf("Send:sendto() failed with error code : %d\n", WSAGetLastError());
                                          exit(EXIT_FAILURE);
                                      }
                                   /* cout << endl << "Send:LSA packet send to:" << SERVER << ", At port:" << si_other.sin_port;*/
                                 }
                            }
                     }
                 mtx.unlock();
                 Sleep(10000);
                 /*each router sending its table to neighbors after 10secs*/
               }
     closesocket(s);
     WSACleanup();
     return 0;
   }

  unsigned int __stdcall printThread(void*)
     {
         int choice;
         while (1)
             {
                 printf("\nEnter a choice\n1. Print Routing Table\n2. Enter new routing matrix row for this router\n");
                 scanf_s("%d", &choice);
                 switch (choice)
                     {
                         case 1:
                                 mtx.lock();
                                 for (int r = 0, c = 0; r < 30; r++)
                                     {
                                         if (routing_table[r][0] != -1)
                                             {
                                                 ::printf("\nLinkID1:%u, LinkID2:%u, Cost:%u, Sequence:%u, Age:%u\n", routing_table[r][0], routing_table[r][1],routing_table[r][2], routing_table[r][3], routing_table[r][4]);
                                                 ::printf("\nTimer difference is:%lu", (routing_table_entry_timer_new[r] - routing_table_entry_timer_old[r]));
                                             }
                                         if (routing_table[r][4] <= 0)
                                            routing_table[r][0] = -1;
                                     }
                                 ::printf("\nSend Matrix is: ");
                                 for (int r = 0; r < 10; r++)
                                     ::printf("%d ", send_matrix[r]);

                                 mtx.unlock();
                                 break;
                         case 2:

                                 int input[10];
 								 printf("\nEnter the values for this router, press enter after entering one value\n");
                                 scanf_s("%d%d%d%d%d%d%d%d%d%d", &input[0], &input[1], &input[2], &input[3],&input[4], &input[5], &input[6], &input[7], &input[8], &input[9]);
                                 mtx.lock();
                                 for (int c = 0; c < 9; c++)
                                     {
                                         send_matrix_new[c] = (char)input[c];
                                     }
                                 for (int c = 0, entry = 0; c < 10; c++)
                                     {
                                         /* insert initial info in routing table */
                                         int flag_found = 0;

                                         if (send_matrix_new[c] != send_matrix[c])
                                             {
                                                /*if true then someone updated the config file */
                                                /* change the routing table based on the new costs */
                                         if (send_matrix_new[c] == 0)
                                             { 
                                                / * link went down */
                                                char entry[2] = { my_id, c };
                                                for (int counter = 0; counter < 30; counter++)
                                                     {
                                                         /*case 1: entry exists with same or different costs*/
                                                         int val = compareMem((void *)(routing_table + counter), (void *)entry, 2);
                                                         if (val == 0)
                                                             {
                                                                  /* entry found */
                                                                  routing_table[counter][2] = -1;
                                                                  routing_table[counter][4] = 60;
                                                                  flag_found = 1;
                                                                  break;
                                                              }
                                                     }
                                             }
                                        else 
                                           {
                                             char entry[2] = { my_id, c };
                                             for (int counter = 0; counter < 30; counter++)
                                                 {
                                                     /* case 1: entry exists with same or different costs */
                                                     int val = compareMem((void *)(routing_table + counter), (void *)entry, 2);
                                                     if (val == 0)
                                                        {
                                                          /* entry found */
                                                          routing_table[counter][2] = send_matrix_new[c];
                                                          routing_table[counter][4] = 60;
                                                          flag_found = 1;
                                                          break;
                                                        }
                                                 }
                                             if (!(flag_found == 1))
                                                 {
                                                     char entry[5] = { my_id, c, send_matrix_new[c], 1, 60 };
                                                     /* find an empty slot */
                                                     for (int counter = 0; counter < 30; counter++)
                                                         {
                                                             if (routing_table[counter][0] == (char)-1)
                                                                  {
                                                                     ::memcpy(&routing_table[counter][0], entry, 5);
                                                                     break;
                                                                  }
                                                         }
                                                 }
                                                   flag_found = 0;
                                            }
                                        }

                                    }
                                    ::memcpy(send_matrix, send_matrix_new, sizeof(send_matrix_new));
                                    ::memcpy(&router_matrix[my_id][0], send_matrix_new, sizeof(send_matrix_new));
                                    mtx.unlock();
                                    break;
                    default:
                             printf("\nWrong Input\n");
                   }
        }
 return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
      HANDLE listen, send, print;
      initializeRouter();
      send = (HANDLE)_beginthreadex(0, 0, &sendThread, (void*)0, 0, 0);
      listen = (HANDLE)_beginthreadex(0, 0, &receiveThread, (void*)0, 0, 0);
      print = (HANDLE)_beginthreadex(0, 0, &printThread, (void*)0, 0, 0);
      WaitForSingleObject(send, INFINITE);
      WaitForSingleObject(listen, INFINITE);
      CloseHandle(listen);
      CloseHandle(send);
      getchar();
      return 0;
}