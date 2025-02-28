#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <curl/curl.h>

#define HTML_URL "https://validator.w3.org/nu/?out=json"
#define HTML_MIME "Content-Type: text/html; charset=utf-8"
/* User agent taken from https://github.com/validator/validator/wiki/Service-%C2%BB-Input-%C2%BB-POST-body*/
#define USER_AGENT "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.101 Safari/537.36"

size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
  return fread(buffer, size, nitems, (FILE *)userdata);
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  return fwrite(ptr, size, nmemb, (FILE *)userdata);
}

int main(int argc, char *argv[])
{
  FILE *in = stdin;
  FILE *out = stdout;

  int opt;
  while ((opt = getopt(argc, argv, "i:o:")) != -1)
  {
    switch (opt)
    {
    case 'i':
      in = fopen(optarg, "r");
      break;
    case 'o':
      out = fopen(optarg, "w");
      break;

    default:
      fprintf(stderr, "Usage: %s [-i in_path] [-o out_path]\n", argv[0]);
      return 1;
    }
  }

  CURL *curl = curl_easy_init();
  if (!curl)
    /* TODO: handle error */
    return 1;

  struct curl_slist *list = NULL;
  list = curl_slist_append(list, HTML_MIME);
  list = curl_slist_append(list, USER_AGENT);

  curl_easy_setopt(curl, CURLOPT_URL, HTML_URL);
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
  curl_easy_setopt(curl, CURLOPT_READDATA, (void *)in);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)out);

  curl_easy_perform(curl);

  curl_slist_free_all(list);
  curl_easy_cleanup(curl);

  fclose(in);
  fclose(out);

  return 0;
}
