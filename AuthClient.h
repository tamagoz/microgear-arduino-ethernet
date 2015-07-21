#ifndef AUTHCLIENT_H
#define AUTHCLIENT_H

#include "Client.h"
#include "SHA1.h"

#define GEARAUTHHOST "gearauth.netpie.io"
#define GEARAUTHPORT 8080
#define VERIFIER "1234"
#define TOKENSIZE               16
#define TOKENSECRETSIZE         32
#define MAXHEADERLINESIZE       350
#define MAXPAYLOADLINESIZE      350
#define NONCESIZE               16
#define MAXSIGNATUREBASELENGTH  512
#define _REQUESTTOKEN           1
#define _ACCESSTOKEN            2

class AuthClient {
    public:
        AuthClient(Client&);
        virtual ~AuthClient();

        void init(char*, char*,unsigned long);
        bool connect();
        void stop();
        void write_P(const char[]);
        void writeln_P(const char[]);
        void write(char*);
        void writeln(char*);
        void strcpy_P(char*,const char[]);
        void strcat_P(char*,const char[]);
        bool readln(char*, size_t);
        int getGearToken(char, char*, char*, char*, char*, char*, char*, char*, char*);
    protected:
    private:
        Client* client;
        char* appid;
        char* gearid;
        char* scope;
        void writeout(char*, char*, bool, bool);
        char* append(char*, char*, char);
        char* append_P(char*, char*, char);
        char* encode(char*,char);
        char* encode(char*,char*);
        char* strtail(char*);
        void addParam(char*, char*, char*, bool);
		unsigned long bootts;
};
#endif // HTTPCLIENT_H