/*
  AuthClient Library
   NetPIE Project
   http://netpie.io
*/

#include "AuthClient.h"
#include <Arduino.h>

AuthClient::AuthClient(Client& iclient) {
    client = &iclient;
}

AuthClient::~AuthClient() {

}

void AuthClient::init(char* appid, char* scope, unsigned long bts) {
    this->appid = appid;
    this->scope = scope;
	this->bootts = bts;
}

bool AuthClient::connect() {
    if (client->connect(GEARAUTHHOST,GEARAUTHPORT)) {
        return true;
    }
    else {
        return false;
    }
}

void AuthClient::stop() {
    client->stop();
}
/**
 * [AuthClient::writeout write string stored in sram or flash memory to client or memory buffer
 * @param dst     destination string address
 * @param src     source string
 * @param newline append new line character?
 * @param progmem true:copy from prog memory, false:copy from sram
 */
void AuthClient::writeout(char* dst, char* src, bool newline, bool progmem) {
    unsigned char len;
    char *dp;
    uint8_t ch, buff[MAXHEADERLINESIZE];

    if (!client) return;
    len = 0;
    if (dst != NULL) dp = dst;
    if (src) {
        if (progmem) {
            while ((ch = pgm_read_byte(src++))) {
                buff[len++] = ch;
                if (dst != NULL) {
                    *dp = ch;
                    dp++;
                }
            }
            if (dst != NULL) {
                *dp = '\0';
            }
        }    
        else {
            len = strlen(src);
            strcpy((char *)buff, src);
            if (dst != NULL) strcpy(dst, src);
        }
    }
    if (newline) {
        buff[len++] = '\r';
        buff[len++] = '\n';
    }
    /* if dst == null then write to client */
    if (dst == NULL)
        client->write(buff,len);
}

void AuthClient::write_P(const char str[]) {
  writeout(NULL, (char *)str, false, true);
}

void AuthClient::writeln_P(const char str[]) {
  writeout(NULL, (char *)str, true, true);
}

void AuthClient::write(char* str) {
  writeout(NULL, str, false, false);
}

void AuthClient::writeln(char* str) {
  writeout(NULL, str, true, false);
}

void AuthClient::strcpy_P(char *dst, const char src[]) {
    writeout(dst, (char *)src, false, true);
}

void AuthClient::strcat_P(char *dst, const char src[]) {
    unsigned char len = strlen(dst);
    writeout(dst+len, (char *)src, false, true);
}

bool AuthClient::readln(char *buffer, size_t buflen) {
	size_t pos = 0;
	while (true) {
		while (true) {
			uint8_t byte = client->read();
			if (byte == '\n') {
				// EOF found.
				if (pos < buflen) {
					if (pos > 0 && buffer[pos - 1] == '\r')
					pos--;
					buffer[pos] = '\0';
				}
				else {
					buffer[buflen - 1] = '\0';
				}
				return true;
			}
			if (pos < buflen)
			buffer[pos++] = byte;
		}
	}
	return false;
}

bool processTok(char* key, char* buff, char **p) {
    char *r;
    char len = strlen(key);

    if (strncmp(*p,key,len)==0) {
        *p += len;
        r = buff;
        while (**p!='\0' && **p!='&' && **p!='\r' && **p!='\n')
            *r++ = *(*p)++;

        *r = '\0';
        if (**p=='&') *p++;
        return true;
    }
    else return false;
}

char* AuthClient::append(char* buffer, char* mstr, char stopper) {
    strcat(buffer,mstr);
    if (stopper) {
        int len = strlen(buffer);
        buffer[len] = stopper;
        buffer[len+1] = '\0';
    }
    return buffer;
}

char* AuthClient::append_P(char* buffer, char* pstr, char stopper) {
    char ch;
    int len = strlen(buffer);
    while ((ch = pgm_read_byte(pstr++))) buffer[len++] = ch;
    if (stopper) {
        buffer[len] = stopper;
        buffer[len+1] = '\0';
    }
    else buffer[len] = '\0';
    return buffer;
}

void AuthClient::addParam(char* buff, char* key, char* value, bool first) {
    if (!first) strcat(buff,"%26");
    strcat(buff,key);
    strcat(buff,"%3D");
    encode(strtail(buff),value);
}

int AuthClient::getGearToken(char mode, char* token, char* tokensecret, char* endpoint, char* gearkey, char* gearsecret, char* scope, char* rtoken, char* rtokensecret) {
        const char* noncealpha = PSTR("0123456789abcdefghijklmnopqrstuvwxyz");

        char buff[MAXHEADERLINESIZE];
        char signbase[MAXSIGNATUREBASELENGTH];
        char nonce[NONCESIZE+1];

        const char* OAUTH_CALLBACK = PSTR("oauth_callback=");
        const char* OAUTH_CONSUMER_KEY = PSTR("oauth_consumer_key=");
        const char* OAUTH_NONCE = PSTR("oauth_nonce=");
        const char* OAUTH_SIGNATURE_METHOD_CONST = PSTR("oauth_signature_method=\"HMAC-SHA1\"");
        const char* OAUTH_TIMESTAMP = PSTR("oauth_timestamp=");
        const char* OAUTH_VERSION_CONST = PSTR("oauth_version=\"1.0\"");
        const char* OAUTH_SIGNATURE = PSTR("oauth_signature=");
        const char* OAUTH_TOKEN = PSTR("oauth_token=");
        const char* OAUTH_VERIFIER = PSTR("oauth_verifier=");

        *token = '\0';
        *tokensecret = '\0';
        *endpoint = '\0';

        //strcpy(signbase,"POST&http%3A%2F%2F");
        strcpy_P(signbase,PSTR("POST&http%3A%2F%2F"));
       sprintf(strtail(signbase),"%s%%3A%d",GEARAUTHHOST,GEARAUTHPORT);

        if (mode == _REQUESTTOKEN) {
            writeln_P(PSTR("POST /oauth/request_token HTTP/1.1"));
            //strcat(signbase,"%2Foauth%2Frequest_token&");
            strcat_P(signbase,PSTR("%2Foauth%2Frequest_token&"));
        }
        else {
            writeln_P(PSTR("POST /oauth/access_token HTTP/1.1"));
            //strcat(signbase,"%2Foauth%2Faccess_token&");
            strcat_P(signbase,PSTR("%2Foauth%2Faccess_token&"));
        }

        sprintf(buff,"Host: %s:%d",GEARAUTHHOST,GEARAUTHPORT);
        writeln(buff);

        write_P(PSTR("Authorization: OAuth "));

        //OAUTH_CALLBACK
		/* this header is too long -- have to break into smaller chunks to write */
        *buff = '\0';
        append_P(buff,(char *)OAUTH_CALLBACK,0);
		//strcat(buff,"\"appid%3D");
        strcat_P(buff,PSTR("\"appid%3D"));
		strcat(buff,appid);
        write(buff);

		*buff = '\0';
		//strcat(buff,"%26scope%3D");
        strcat_P(buff,PSTR("%26scope%3D"));
		strcat(buff,scope);
		//strcat(buff,"%26verifier%3D");
        strcat_P(buff,PSTR("%26verifier%3D"));
		strcat(buff,VERIFIER);
		strcat(buff,"\",");
        write(buff);

        *buff = '\0';
        append_P(buff,(char *)OAUTH_CALLBACK,0);
        encode(strtail(signbase),buff);

        /*add key value*/
        buff[0] = '\0';
        addParam(buff,"appid",appid,1);
        addParam(buff,"scope",scope,0);
        addParam(buff,"verifier",VERIFIER,0);
        encode(strtail(signbase),buff);

        //OAUTH_CONSUMER_KEY
        *buff = '\0';
        append_P(buff,(char *)OAUTH_CONSUMER_KEY,0);
        sprintf(strtail(buff),"\"%s\"",gearkey);
        strcat(signbase,"%26"); //&
        encode(strtail(signbase),buff);
        write(strcat(buff,","));

        //OAUTH_NONCE
        randomSeed(analogRead(0));
        for (char i=0;i<NONCESIZE;i++) {
            nonce[i] = pgm_read_byte(noncealpha+random(0,35));
        }
        nonce[NONCESIZE] = '\0';
        *buff = '\0';
        append_P(buff,(char *)OAUTH_NONCE,0);
        sprintf(strtail(buff),"\"%s\"",nonce);
        strcat(signbase,"%26");  //&
        encode(strtail(signbase),buff);
        write(strcat(buff,","));

        //OAUTH_SIGNATURE_METHOD
        *buff = '\0';
        append_P(buff,(char *)OAUTH_SIGNATURE_METHOD_CONST,0);
        strcat(signbase,"%26"); //&
        encode(strtail(signbase),buff);
        write(strcat(buff,","));

        //OAUTH_TIMESTAMP
        *buff = '\0';
        append_P(buff,(char *)OAUTH_TIMESTAMP,0);

		//By pass NTP
        //sprintf(strtail(buff),"\"%lu\"",(unsigned long)gettimestamp());
		sprintf(strtail(buff),"\"%lu\"",bootts+millis()/1000);

        strcat(signbase,"%26"); //&
        encode(strtail(signbase),buff);
        write(strcat(buff,","));

        if (rtoken) {
            //OAUTH_TOKEN
            *buff = '\0';
            append_P(buff,(char *)OAUTH_TOKEN,0);
            sprintf(strtail(buff),"\"%s\"",rtoken);
            strcat(signbase,"%26"); //&
            encode(strtail(signbase),buff);
            write(strcat(buff,","));

            //OAUTH_VERIFIER
            *buff = '\0';
            append_P(buff,(char *)OAUTH_VERIFIER,0);
            sprintf(strtail(buff),"\"%s\"",VERIFIER);
            strcat(signbase,"%26"); //&
            encode(strtail(signbase),buff);
            write(strcat(buff,","));
        }

        //OAUTH_VERSION
        *buff = '\0';
        append_P(buff,(char *)OAUTH_VERSION_CONST,0);
        strcat(signbase,"%26"); //&
        encode(strtail(signbase),buff);
        write(strcat(buff,","));

        // Calculate HMAC-SHA1(signbase) and include in the Authorization header
        //OAUTH_SIGNATURE
        char hkey[66];
        char signature[29];
       *buff = '\0';
        append_P(buff,(char *)OAUTH_SIGNATURE,'"');
        sprintf(hkey,"%s&%s",gearsecret,rtokensecret?rtokensecret:"");

        Sha1.initHmac((uint8_t*)hkey,strlen(hkey));
        Sha1.HmacBase64(signature, signbase);
        encode(strtail(buff),signature);
        strcat(buff,"\"");
        write(strcat(buff,"\r\n"));

        writeln_P(PSTR("Accept: */*"));
        writeln_P(PSTR("Connection: close"));
        writeln_P(PSTR("User-Agent: Arduigear"));
        writeln_P(PSTR("Content-length: 0"));
        writeln_P(PSTR("Content-Type: application/x-www-form-urlencoded"));
        writeln(NULL);

		delay(1000);

        int httpstatus = 0;
        char pline = 0;
        while(true) {
            if (readln(buff, (size_t)MAXHEADERLINESIZE)) {
                    if (pline<2) {
                        if (strncmp(buff,"HTTP",4)==0) {
                            buff[12] = '\0';
                            httpstatus = atoi(buff+9);
                        }
                    }
                    else {
                        char *p;
						char *s, *t;

						int last = 0;
                        p = buff;
						while (*p != '\0') {
							s = p;
							while (*s!='=' && *s!='\0' && *s!='&') s++;	/* seek for = */
							t= s;
							while (*t!='\0' && *t!='&') t++;	        /* seek for & */
							*s = '\0';
							if (*t == '\0')	last = 1;
							*t = '\0';
	
							if (strcmp(p,"oauth_token")==0) strcpy(token,s+1);
							else if (strcmp(p,"oauth_token_secret")==0) strcpy(tokensecret,s+1);
							else if (strcmp(p,"endpoint")==0) strcpy(endpoint,s+1);
							delay(400);

							if (!last) p = t+1; else break;
						}
                        return httpstatus;
                    }
                    if (strlen(buff)<6) pline++;
            }
            else {
                return 0;
            }
        }
        return httpstatus;
}

char* AuthClient::encode(char *buffer, char ch) {
    // ignore doublequotes that include in oauth header value
    if(ch == '"') return buffer;

    if ('0' <= ch && ch <= '9' || 'a' <= ch && ch <= 'z' || 'A' <= ch && ch <= 'Z' || ch == '-' || ch == '.' || ch == '_' || ch == '~') {
        *buffer++ = ch;
    }
    else {
        int val = ((int) ch) & 0xff;
        snprintf(buffer, 4, "%%%02X", val);
        buffer += 3;
    }
    *buffer = '\0';
    return buffer;
}

char* AuthClient::encode(char *buffer, char *data) {
    char* p = buffer;
    while (char ch = *data++) p = encode(p, ch);
    return buffer;
}

char* AuthClient::strtail(char* buff) {
    return buff + strlen(buff);
}