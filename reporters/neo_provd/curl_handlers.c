#include "curl_handlers.h"

size_t curl_write_data(void * ptr, size_t size, size_t nmemb, url_data *data)
{
  size_t index = data->size;
  size_t n = (size * nmemb);
  char* tmp;

  data->size += (size * nmemb);

#ifdef DEBUG
  fprintf(stderr, "data at %p size=%ld nmemb=%ld\n", ptr, size, nmemb);
#endif
  tmp = realloc(data->data, data->size + 1); /* +1 for '\0' */

  if(tmp) {
    data->data = tmp;
  } else {
    if(data->data) {
      free(data->data);
    }
    fprintf(stderr, "Failed to allocate memory.\n");
    return 0;
  }

  memcpy((data->data + index), ptr, n);
  data->data[data->size] = '\0';

  return size * nmemb;
}

void dump(const char *text, FILE *stream, unsigned char *ptr, size_t size,char nohex)
{
  size_t i;
  size_t c;
 
  unsigned int width=0x10;
 
  if(nohex)
    /* without the hex output, we can fit more on screen */ 
    width = 0x40;
 
  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);
 
  for(i=0; i<size; i+= width) {
 
    fprintf(stream, "%4.4lx: ", (long)i);
 
    if(!nohex) {
      /* hex not disabled, show it */ 
      for(c = 0; c < width; c++)
        if(i+c < size)
          fprintf(stream, "%02x ", ptr[i+c]);
        else
          fputs("   ", stream);
    }
 
    for(c = 0; (c < width) && (i+c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */ 
      if (nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
        i+=(c+2-width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */ 
      if (nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
        i+=(c+3-width);
        break;
      }
    }
    fputc('\n', stream); /* newline */ 
  }
  fflush(stream);
}
 

int my_trace(CURL *handle, curl_infotype type,
		    unsigned char *data, size_t size,
		    void *userp)
{
  const char *text;
 
  (void)userp;
  (void)handle; /* prevent compiler warning */ 
 
  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */ 
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  }
 
  dump(text, stderr, data, size, 1);
  return 0;
}
 
char * post(const char * url, const char *postfields){

  struct curl_slist *headers = NULL;

  /* Initialize url_data response */
  url_data response;
  response.size = 0;
  response.data = malloc(4096); /* reasonable size initial buffer */
  if(response.data == NULL){
    fprintf(stderr, "post: Failed to allocate memory.\n");
    return NULL;
  }
  response.data[0] = '\0';

  /* curl configuration */
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER,     headers); 

  curl_easy_setopt(curl, CURLOPT_URL,            url);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,  "POST");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  &curl_write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     postfields);

#if DEBUG
  struct data config;
  curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
  curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

 
  /* Perform the request, res will get the return code */ 
  res_curl = curl_easy_perform(curl);

  /* Check for errors */ 
  if(res_curl != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
	    curl_easy_strerror(res_curl));


  return response.data;

}

