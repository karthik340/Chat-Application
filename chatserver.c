#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
  char cmd_id;
  unsigned short int length_of_data;
  char username_length;
  char *username;
  char password_length;
  char *password;
} login_packet;

typedef struct {
  char cmd_id;
  char response;
} response_packet;

typedef struct {
  char cmd_id;
  unsigned short int total_msglength;
  char source_length;
  char *sourceName;
  char destination_length;
  char *destName;
  unsigned short int message_length;
  char *message;
} message_packet;

typedef struct {
  char cmd_id;
  unsigned short int username_length;
  char *username;
} general_packet;

typedef struct {
  char fd;
  char *username;
} online_users;

typedef struct {
  unsigned char *username;
  unsigned char *password;
} database;

online_users users1[20];
database database1[] = {{"john", "admin@123"},
                        {"bob", "shock@123"},
                        {"rock", "west@123"},
                        {"don", "east@123"}};
int db_index = 4;
int onlineusers_index = -1;

void error1(char *msg) {
  fprintf(stderr, "%s: %s \n", msg, strerror(errno));
  exit(1);
}

int open_listener_socket() {
  int s = socket(PF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    error1("cant open socket");
  }
  return s;
}

void bind_to_port(int socket, int port) {
  struct sockaddr_in name;
  name.sin_family = PF_INET;
  name.sin_port = (in_port_t)htons(port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  int reuse = 1;
  if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                 sizeof(int)) == -1) {
    error1("cant set reuse option on socket");
  }
  int c = bind(socket, (struct sockaddr *)&name, sizeof(name));
  if (c == -1) {
    error1("cant bind to socket");
  }
}

unsigned char *serialize_short(char *buffer, unsigned short int value) {
  buffer[0] = value >> 8;
  buffer[1] = value;
  return buffer + 2;
}

unsigned short int deserialize_short(unsigned char *buffer, int low) {
  unsigned short int a = 0;
  unsigned short int b = buffer[low];
  unsigned short int c = buffer[low + 1];
  a = (b << 8) | c;
  return a;
}

int catch_signal(int sig, void (*handler)(int)) {
  struct sigaction action;
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  return sigaction(sig, &action, NULL);
}

int listener_d;

void handle_shutdown(int sig) {
  if (listener_d)
    close(listener_d);

  fprintf(stderr, "BYE! \n");
  exit(0);
}

void broadcast_addusername(unsigned short int connect_d,
                           char *name) // say add username to clients
{
  unsigned char buf[20] = {0};
  buf[0] = 3;
  unsigned short int namelength = strlen(name);
  serialize_short(buf + 1, namelength);
  memcpy(buf + 3, name, namelength);
  for (int i = 0; i <= onlineusers_index; i++) {
    if (users1[i].fd != connect_d) // make sure that we are not sending to same
                                   // user to add himself
    {
      printf("adding to %s\n", users1[i].username);
      for (int s = 0; s < namelength + 3; s++) {
        printf("byte %d \n", buf[s]);
      }
      send(users1[i].fd, buf, namelength + 3, 0);
      sleep(1);
    }
  }
}

void broadcast_removeusername(unsigned short int connect_d,
                              char *name) // say remove username to clients
{
  unsigned char buf[20] = {0};
  buf[0] = 5;
  unsigned short int namelength = strlen(name);
  serialize_short(buf + 1, namelength);
  memcpy(buf + 3, name, namelength);
  for (int i = 0; i <= onlineusers_index; i++) {
    if (users1[i].fd != connect_d) // make sure that we are not sending to same
                                   // user to remove himself
    {
      printf("remove to %s\n", users1[i].username);
      for (int s = 0; s < namelength + 3; s++) {
        printf("byte %d \n", buf[s]);
      }
      send(users1[i].fd, buf, namelength + 3, 0);
      sleep(1);
    }
  }
}

void add_existing_users(
    connect_d) // add_existing_users to currently logged person
{
  unsigned char buf[20] = {0};

  for (int i = 0; i <= onlineusers_index; i++) {
    memset(buf, 0, sizeof(buf));
    if (users1[i].fd != connect_d) // make sure that we are not sending to same
                                   // user to add himself
    {
      buf[0] = 3;
      char *username = users1[i].username;
      unsigned short int namelength = strlen(username);
      serialize_short(buf + 1, namelength);
      memcpy(buf + 3, username, namelength);
      printf("adding to himself\n");
      for (int s = 0; s < namelength + 3; s++) {
        printf("byte %d \n", buf[s]);
      }
      printf("sending to %d \n", connect_d);
      send(connect_d, buf, namelength + 3, 0);
      sleep(1);
    }
  }
}

void remove_username_from_onlineusers(
    char *name) // remove_username_from_onlineusers
{
  for (int i = 0; i <= onlineusers_index; i++) {
    if (strcmp(users1[i].username, name) == 0) {
      printf("removing user %s\n", name);
      for (int j = i; j <= onlineusers_index - 1; j++) {
        users1[j].username = users1[j + 1].username;
        users1[j].fd = users1[j + 1].fd;
      }
      break;
    }
  }
  onlineusers_index--;
}

// thread
void *do_stuff(void *a) {
  long connect_d = (long)a;
  int length;
  unsigned char buf[255] = {0};
  unsigned char *ptr;

  while (1) {
    memset(buf, 0, sizeof(buf));
    int bytes = recv(connect_d, buf, 255, 0);
    if (bytes == 0) {
      break;
    }
    printf("%d bytes received\n", bytes);
    for (int i = 0; i < bytes; i++) {
      printf("%d \n", buf[i]);
    }
    response_packet r1;
    if (1 == buf[0]) // check if it is login packet
    {
      unsigned char name_length;
      unsigned char pass_length;
      memcpy(&name_length, buf + 3, 1);
      memcpy(&pass_length, buf + 4 + name_length, 1);
      unsigned char *name = malloc(20);
      ;
      unsigned char password[20] = {0};
      memcpy(name, buf + 4, *(buf + 3));
      memcpy(password, buf + 4 + name_length + 1,
             *(buf + 4 + *(buf + 3))); // extract username and password
      name[name_length] = '\0';
      password[pass_length] = '\0';
      int flag = 0;
      memset(buf, 0, sizeof(buf));
      buf[0] = 1;
      printf("%s %s", name, password);

      int already_loggedin = 0;
      for (int i = 0; i <= onlineusers_index;
           i++) // checking whether user logged in already
      {
        if (strcmp(users1[i].username, name) == 0) {
          already_loggedin = 1;
          buf[1] = 1;
        }
      }

      if (!already_loggedin) {
        for (int i = 0; i < db_index; i++) {
          if (strcmp(name, database1[i].username) == 0 &&
              strcmp(password, database1[i].password) ==
                  0) // checking for valid login
          {
            flag = 1;
            i = db_index;
            buf[1] = 0;
          }
        }
        if (flag == 0)
          buf[1] = 1;
        else {
          onlineusers_index++;
          users1[onlineusers_index].fd =
              connect_d; // storing in online_users if valid login
          users1[onlineusers_index].username = name;
        }
      }
      printf("\n %d %d \n", buf[0], buf[1]);
      for (int i = 0; i <= onlineusers_index; i++) {
        printf("%d \n %s\n", users1[i].fd, users1[i].username);
      }
      send(connect_d, buf, 2, 0); // sending response for login
      sleep(1);
      if (0 == buf[1]) {
        broadcast_addusername(connect_d,
                              name);   // saying add username to clients
        add_existing_users(connect_d); // add existing users to logged in user
      }

    } else if (2 == buf[0]) // check if it  is message packet
    {
      char destination_name[20] = {0};
      unsigned char source_length;
      unsigned char destination_length;
      source_length = *(buf + 3);
      destination_length = *(buf + 4 + source_length);
      memcpy(destination_name, buf + 5 + source_length, destination_length);
      destination_name[destination_length] = '\0';
      int fd = 0;
      printf("destination = %s\n", destination_name);
      for (int i = 0; i <= onlineusers_index; i++) {
        if (strcmp(destination_name, users1[i].username) == 0) {
          printf("sending message to %s", destination_name);
          send(users1[i].fd, buf, bytes, 0);
          sleep(1);
          break;
        }
      }
      printf("fd=%d\n", fd);
    } else if (5 == buf[0]) // remove loggedout user
    {
      unsigned short int namelength = deserialize_short(buf, 1);
      unsigned char name[20];
      memcpy(name, buf + 3, namelength);
      name[namelength] = '\0';
      broadcast_removeusername(connect_d, name);
      remove_username_from_onlineusers(name);
      break;
    }
  }

  printf("closing man\n");
  close(connect_d);
}

int main() {
  if (catch_signal(SIGINT, handle_shutdown) == -1) {
    error1("cant set interrupt handler");
  }

  listener_d = open_listener_socket();
  bind_to_port(listener_d, 30000);
  if (listen(listener_d, 10) == -1) {
    error1("cant listen");
  }
  printf("listener socket = %d \n", listener_d);
  struct sockaddr_storage client_addr;
  unsigned int address_size = sizeof(client_addr);
  pthread_t threads[20];
  int t = -1;

  while (1) {
    puts("waiting for connection \n");
    int connect_d =
        accept(listener_d, (struct sockaddr *)&client_addr, &address_size);
    if (connect_d == -1) {
      error1("cant open secondary connection");
    }
    t = t + 1;
    printf("connected %d\n", connect_d);
    if (pthread_create(&threads[t], NULL, do_stuff, (void *)connect_d) == -1) {
      error1("cant create thread");
    }
  }

  void *result;
  for (int i = 0; i <= t; i++) {
    pthread_join(threads[t], &result);
  }

  close(listener_d);
}
