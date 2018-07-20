/**
	Handle multiple socket connections with select and fd_set on Linux
*/

#include <iostream>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#include <chrono>
#include <vector>
#include <unordered_map>
#include <future>

using namespace std;

#define TRUE   1
#define FALSE  0
#define PORT 8888

struct DATA
{
  string event_data;
  int sock_disc;
};

//PROACTOR
vector<future<DATA>> v;
future_status status;

string str;
int sfd;

unordered_map<string, std::function<void()>> handlers{};

void addHandler(std::string event, std::function<void()> callback) {
  handlers.emplace(std::move(event), std::move(callback));
}

DATA event_handler(std::string ipstr, int sfd)   //Event handler
{
  int j;
  if (ipstr == "one")
  {
    j = 5;
  }
  else if (ipstr == "two")
  {
    j = 10;
  }
  DATA d;
  char n[30];
  string tmp;
  d.sock_disc = sfd;
  cout << "SFD = " << sfd << endl;
  for (int i=0; i<j; i++)
  {
    tmp = ipstr;
    tmp += "=>";
    cout << ipstr << " => " << i << endl;
    tmp += to_string(i);
    tmp += "\n";
    strcpy(n, tmp.c_str());
    send(sfd, n, strlen(n) , 0);
    sleep(1);
  }
  cout << ipstr << " => DONE" << endl;
  ipstr += "=>DONE\n";
  d.event_data = ipstr;
  return d;
}

void completion_dispatcher()   //Completion dispatcher
{
  cout << "completion_dispatcher" << endl;
  while(1)
  {
    for (int i=0; i<v.size(); i++)
    {
      status = v[i].wait_for(std::chrono::seconds(1));
      if (status == future_status::ready)
      {
        DATA temp;
        temp=v[i].get();
        char dt[10];
        strcpy(dt, temp.event_data.c_str());
        send(temp.sock_disc, dt, strlen(dt) , 0);
        v.erase(v.begin() + i);
      }
    }
  }
}

int cnt = 0;

void event_dispatcher()   //Initiator / Event dispatcher
{
  cout << "Event Handler" << endl;
  while(1)
  {
    getline(cin, str);
    cout << "EVENT => " << str << endl;
    try 
    {
      handlers.at(str)();
      cnt++;
    } 
    catch (const std::out_of_range& e) 
    {
      std::cout << "no handler for " << str << '\n';
    }
  }
}

void event_dispatcher_tcp(string inp, int fd)   //Initiator / Event dispatcher
{
  inp.erase(inp.find_last_not_of(" \n\r\t")+1);
  cout << "Event Handler" << endl;
  cout << "EVENT=>" << inp << " / FD=>" << fd << endl;
  str = inp;
  sfd = fd;
  try 
  {
    handlers.at(inp)();
    cnt++;
  } 
  catch (const std::out_of_range& e) 
  {
    std::cout << "no handler for " << inp << '\n';
  }
}

int main()
{
  //PROACTOR
  addHandler("one", [](){
    std::cout << "one handler called!" << '\n';
        //DO SOMETHING
    v.push_back(async(launch::async, event_handler, str, sfd));
  });
  addHandler("two", [](){
    std::cout << "two handler called!" << '\n';
        //DO SOMETHING
    v.push_back(async(launch::async, event_handler, str, sfd));
  });

  cout << "V size = " << v.size() << endl;
  auto f = async(launch::async, event_dispatcher);
  auto fh = async(launch::async, completion_dispatcher);
  cout << "READY" << endl;

//***************************************************************

  int opt = TRUE;
  int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
  int max_sd;
  struct sockaddr_in address;

  char buffer[1025];  //data buffer of 1K

  //set of socket descriptors
  fd_set readfds;

  //a message
  char message[30] = "Proactor Server!\r\n";

  //initialise all client_socket[] to 0 so not checked
  for (i = 0; i < max_clients; i++) 
  {
    client_socket[i] = 0;
  }

  //create a master socket
  if((master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  //set master socket to allow multiple connections , this is just a good habit, it will work without this
  if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
  {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  //type of socket created
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  //bind the socket to localhost port 8888
  if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  cout << "Listener on port" << PORT << endl;

  //try to specify maximum of 3 pending connections for the master socket
  if (listen(master_socket, 3) < 0)
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  //accept the incoming connection
  addrlen = sizeof(address);
  cout << "Waiting for connections ..." << endl;
  
  while(TRUE) 
  {
      //clear the socket set
    FD_ZERO(&readfds);

      //add master socket to set
    FD_SET(master_socket, &readfds);
    max_sd = master_socket;

      //add child sockets to set
    for ( i = 0 ; i < max_clients ; i++) 
    {
          //socket descriptor
      sd = client_socket[i];

		//if valid socket descriptor then add to read list
      if(sd > 0)
      {
        FD_SET( sd , &readfds);
      }

            //highest file descriptor number, need it for the select function
      if(sd > max_sd)
      {
        max_sd = sd;
      }
    }

      //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
    activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

    if ((activity < 0) && (errno!=EINTR)) 
    {
      printf("select error");
    }

      //If something happened on the master socket , then its an incoming connection
    if (FD_ISSET(master_socket, &readfds)) 
    {
      if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
      {
        perror("accept");
        exit(EXIT_FAILURE);
      }

          //inform user of socket number - used in send and receive commands
      cout << "New connection , socket fd is " << new_socket << " , ip is : " << inet_ntoa(address.sin_addr) << " , port : " << ntohs(address.sin_port) << endl;;

          //send new connection greeting message
      if(send(new_socket, message, strlen(message), 0) != strlen(message) ) 
      {
        perror("send");
      }

      cout << "Welcome message sent successfully" << endl;

          //add new socket to array of sockets
      for (i = 0; i < max_clients; i++) 
      {
              //if position is empty
        if( client_socket[i] == 0 )
        {
          client_socket[i] = new_socket;
          cout << "Adding to list of sockets as " << i << endl;
          break;
        }
      }
    }

      //else its some IO operation on some other socket :)
    for (i = 0; i < max_clients; i++) 
    {
      sd = client_socket[i];

      if (FD_ISSET(sd , &readfds)) 
      {
              //Check if it was for closing , and also read the incoming message
        if ((valread = read(sd , buffer, 1024)) == 0)
        {
                  //Somebody disconnected , get his details and print
          getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
          cout << "Host disconnected , ip : " << inet_ntoa(address.sin_addr) << " , port : " << ntohs(address.sin_port) << endl;

                  //Close the socket and mark as 0 in list for reuse
          close( sd );
          client_socket[i] = 0;
        }

              //Echo back the message that came in
        else
        {
                  //set the string terminating NULL byte on the end of the data read
          buffer[valread] = '\0';
          event_dispatcher_tcp((string)buffer, sd);
          // send(sd , buffer , strlen(buffer) , 0 );
        }
      }
    }
  }
  return 0;
} 