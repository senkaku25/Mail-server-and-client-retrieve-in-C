#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);

int main(int argc, char *argv[]) {
  
  if (argc != 2) {
    fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
    return 1;
  }
  
  run_server(argv[1], handle_client);
  
  return 0;
}

void handle_client(int fd) {
    ///CURRENT GOALS 1. LINK MAIL FROM TO DATA (?)? 2. TEST USER LIST 
   ///test
   int i= is_valid_user("john.doe@example.com", NULL);
    printf("%i",i);
     i= is_valid_user("john.do@example.com", NULL);
    printf("%i",i);
     i= is_valid_user("mary.smith@example.com", NULL);
    printf("%i",i);
    char msg[100] = "220 Welcome to local host, SMTP Server \n";
    write(fd, msg, strlen(msg));
   net_buffer_t netbuffer= nb_create(fd, MAX_LINE_LENGTH);
    char data[512];
    //define checks
    int helo=0;
     int FROMcheck=0;
    int RCPTcheck=0;
    while(nb_read_line(netbuffer, data)){
        printf("%s",data);
        
        //create users but empty
        
        user_list_t userlist=create_user_list();
        
        //unsupported commands
        if((strncmp(data,"EHLO",4)==0)||(strncmp(data,"RSET",4)==0)||(strncmp(data,"VRFY",4)==0)||(strncmp(data,"EXPN",4)==0)||(strncmp(data,"HELP",4)==0)){
            char msg[100] = "502 Unsupported Command \n";
            write(fd, msg, strlen(msg));
        }
        else if(strncmp(data,"QUIT",4)==0){
            char msg[100] = "221 local host Service closing transmission channel \n";
            write(fd, msg, strlen(msg));
            break;
        }
        ///NOOP Command
       else if(strncmp(data,"NOOP",4)==0){
            char msg[100] = "250 OK  \n";
            write(fd, msg, strlen(msg));
        }
        ///HELO command
       else if(strncmp(data,"HELO",4)==0){
           helo=1;
            char msg[100] = "250 HELO command recieved! Hello! This server supports HELO, NOOP, MAIL, DATA, QUIT \n";
            write(fd, msg, strlen(msg));
        }
        ///MAIL command
       else if(strncmp(data,"MAIL",4)==0){
           ///Error Check for HELO
           if(helo!=1){
               char msg[100] = "500 handshake with server! Send a HELO first \n";
               write(fd, msg, strlen(msg));
           }
            char output[512];
            if(strncmp(data,"MAIL FROM:<",11)==0){
                FROMcheck=1;
                int x=0;
                int y=0;
                // checks for < and >, takes data in between.
                for(int i=0; i<strlen(data);i++){
                    if (data[i]=='<'){
                        x=1;
                    }
                    if (data[i]=='>'){
                        x=0;
                        y=1;
                    }
                    if(x==1){
                        output[i-11]=data[i];
                    }
                }
                ///User did not put < or > .
                if(x!=0&&y!=1){
                    char msg[100] = "500 Wrong format! Format is MAIL FROM:<....> \n";
                    write(fd, msg, strlen(msg));
                }
              //better formatting
                else{
                char msg[100] = "250 MAIL command recieved from\n";
                write(fd, msg, strlen(msg));
                write(fd, output, strlen(output));
                 char msg2[100] = "\n";
                write(fd, msg2, strlen(msg));
                }
            }
                else{
                    char msg[100] = "500 Wrong format! Format is MAIL FROM:<....> \n";
                    write(fd, msg, strlen(msg));
                }
            }
        ///DATA Command
       else if(strncmp(data,"DATA",4)==0){
           if(strncmp(data,"DATA",4)>0){
               char msg[100] = "500 Too Long! Send DATA first \n";
               write(fd, msg, strlen(msg));
           }
           else if(RCPTcheck!=1){
               char msg[100] = "500 Specify RCPT! \n";
               write(fd, msg, strlen(msg));
           }
           else{
           static char template[] = "/tmp/myfileXXXXXX";
           char fname[4096];
           int fd2;
           char msg[100] = "354 End Data in .\r \n";
           write(fd, msg, strlen(msg));
           strcpy(fname, template);
           fd2 = mkstemp(fname);
           printf("Filename is %s\n", fname);
           
           while(strncmp(data,".",1)!=0){
               nb_read_line(netbuffer, data);
               write(fd2, data, strlen(data));
           }
           /*
           lseek(fd2, 0L, SEEK_SET);
           n = read(fd2, buf, sizeof(buf));         Read data back; NOT '\0' terminated!
           printf("Got back: %.*s", n, buf);    /* Print it out for verification */
           save_user_mail(fname, userlist);
           char msg2[100] = "250 OK \n";
           write(fd, msg2, strlen(msg));
           close(fd2);                /* Close file */
           unlink(fname);                /* Remove it */
           }
       }
        ///RCPT Function
        else if(strncmp(data,"RCPT",4)==0){
            ///If FROM is not used, first use from
            if(FROMcheck!=1){
                char msg2[100] = "500 Error! Specify FROM\n";
                write(fd, msg2, strlen(msg));
            }
            else{
            char output[512];
            if(strncmp(data,"RCPT TO:<",9)==0){
                int x=0;
                // checks for < and >, takes data in between.
                for(int i=0; i<strlen(data);i++){
                    if (data[i]=='<'){
                        x=1;
                    }
                    if (data[i]=='>'){
                        x=0;
                    }
                    if(x==1){
                        output[i-9]=data[i];
                    }
                }
                if(is_valid_user(output, NULL)!=0){
                    add_user_to_list(&userlist, output);
                    char msg[100] = "250 OK \n";
                    write(fd, msg, strlen(msg));
                    RCPTcheck=1;
                }else{
                char msg[100] = "550 No such user here \n";
                write(fd, msg, strlen(msg));
                write(fd, output, strlen(output));
                }
            }
            else{
                char msg[100] = "500 Wrong format! Format is RCPT FROM:<....> \n";
                write(fd, msg, strlen(msg));
            }
                
            }
            
        }
       else if(strlen(data)>MAX_LINE_LENGTH){
            char msg[100] = "500 Line too long! \n";
            write(fd, msg, strlen(msg));
        }
       else{
        char msg[100] = "500 Invalid Command! \n";
           write(fd, msg, strlen(msg));}
     //   read(fd,data,10);
   //nc -c 127.0.0.1 6000
  // TODO To be implemented
    }
}
