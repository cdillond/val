#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <curl/curl.h>

#define HTML_JSON_URL "https://validator.w3.org/nu/?out=json"
#define HTML_XML_URL "https://validator.w3.org/nu/?out=xml"
#define HTML_CONTENT_TYPE "Content-Type: text/html; charset=utf-8"

#define CSS_URL "TODO"
#define CSS_CONTENT_TYPE "Content-Type: text/css; charset=utf-8"

/* User agent taken from https://github.com/validator/validator/wiki/Service-%C2%BB-Input-%C2%BB-POST-body*/
#define USER_AGENT "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/41.0.2272.101 Safari/537.36"

size_t read_callback(char *buf, size_t size, size_t n_memb, void *in_file)
{
  return fread(buf, size, n_memb, (FILE *)in_file);
}

size_t write_callback(char *buf, size_t size, size_t n_memb, void *out_file)
{
  return fwrite(buf, size, n_memb, (FILE *)out_file);
}

int main(int argc, char *argv[])
{
  FILE *in = stdin;
  FILE *out = stdout;
  const char *url = HTML_JSON_URL;
  const char *content_type = HTML_CONTENT_TYPE;
  extern char *optarg;
  int opt;
  while ((opt = getopt(argc, argv, "i:o:xc")) != -1)
  {
    switch (opt)
    {
    case 'i':
      in = fopen(optarg, "r");
      break;
    case 'o':
      out = fopen(optarg, "w");
      break;
    case 'x':
      url = HTML_XML_URL;
      break;
    case 'c':
      url = CSS_URL;
      content_type = CSS_CONTENT_TYPE;
      break;
    default:
      fprintf(stderr, "Usage: %s [-i in_path] [-o out_path] [-x xml output]\n", argv[0]);
      return 1;
    }
  }

  CURL *curl = curl_easy_init();
  if (!curl)
  {
    fputs("error: unable to initialize CURL context\n", stderr);
    /* TODO: handle error */
    return 1;
  }

  struct curl_slist *list = NULL;
  list = curl_slist_append(list, content_type);
  list = curl_slist_append(list, USER_AGENT);

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
  curl_easy_setopt(curl, CURLOPT_READDATA, (void *)in);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)out);

  CURLcode status = curl_easy_perform(curl);
  if (status)
    fprintf(stderr, "CURL error: response code %d\n", status);

  curl_slist_free_all(list);
  curl_easy_cleanup(curl);

  fclose(in);
  fclose(out);

  return (int)status;
}
