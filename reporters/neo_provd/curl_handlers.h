

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <curl/curl.h>

/* Curl Stuff */
typedef struct{
  size_t size;
  char* data;
} url_data;

struct data {
  char trace_ascii; /* 1 or 0 */ 
};

//#define DEBUG 1

CURL       *curl;
CURLcode   res_curl;

size_t  curl_write_data(void * ptr, size_t size, 
			       size_t nmemb, url_data *data);
void    dump           (const char *text, FILE *stream, 
			       unsigned char *ptr, size_t size,
			       char nohex);
int     my_trace       (CURL *handle, curl_infotype type,
			       unsigned char *data, size_t size,
			       void *userp);
char * post(const char * url, const char *postfields);

