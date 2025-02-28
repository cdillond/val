#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#define HTML_JSON_URL "https://validator.w3.org/nu/?out=json"
#define HTML_XML_URL "https://validator.w3.org/nu/?out=xml"
#define HTML_CONTENT_TYPE "Content-Type: text/html; charset=utf-8"

#define CSS_URL "https://jigsaw.w3.org/css-validator/validator?"
#define CSS_QUERY "text="
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

enum CSS_OUTPUT_FORMAT
{
  TEXT_HTML,
  HTML,
  XHTML_XML,
  XHTML,
  SOAP_XML,
  SOAP12,
  TEXT_PLAIN,
  TEXT,
};

static const char *outputs[] = {
    "output=text/html",
    "output=html",
    "output=application/xhtml+xml",
    "output=xhtml",
    "output=application/soap+xml",
    "output=soap12",
    "output=text/plain",
    "output=text",
};

int validate_css(CURL *curl, FILE *in, FILE *out, enum CSS_OUTPUT_FORMAT format)
{

  int end = fseek(in, 0, SEEK_END);
  assert(end == 0);
  end = ftell(in);
  rewind(in);

  char *data = malloc(sizeof CSS_QUERY - 1 + end + 1);
  memcpy(data, CSS_QUERY, sizeof CSS_QUERY);
  size_t n = fread(data + sizeof CSS_QUERY - 1, sizeof *data, end, in);
  assert(n == (size_t)end);

  data[sizeof CSS_QUERY - 1 + end] = '\0';

  CURLU *url = curl_url();
  assert(url != NULL);
  CURLUcode rc = curl_url_set(url, CURLUPART_URL, CSS_URL, 0);
  assert(rc == 0);
  rc = curl_url_set(url, CURLUPART_QUERY, data, CURLU_APPENDQUERY | CURLU_URLENCODE);
  assert(rc == 0);
  rc = curl_url_set(url, CURLUPART_QUERY, outputs[format], CURLU_APPENDQUERY | CURLU_URLENCODE);
  assert(rc == 0);

  curl_easy_setopt(curl, CURLOPT_CURLU, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);

  CURLcode status = curl_easy_perform(curl);
  if (status)
    fprintf(stderr, "CURL error: response code %d\n", status);

  curl_url_cleanup(url);
  free(data);
  return status;
}

int main(int argc, char *argv[])
{
  FILE *in = stdin;
  FILE *out = stdout;
  const char *url = HTML_JSON_URL;
  const char *content_type = HTML_CONTENT_TYPE;
  int use_css = 0;
  int css_format = TEXT_PLAIN;
  extern char *optarg;
  int opt;
  while ((opt = getopt(argc, argv, "i:o:f:xc")) != -1)
  {
    switch (opt)
    {
    case 'i':
      in = fopen(optarg, "r");
      break;
    case 'o':
      out = fopen(optarg, "w");
      break;
    case 'f':
      css_format = atoi(optarg);
      break;
    case 'x':
      url = HTML_XML_URL;
      break;
    case 'c':
      use_css = 1;
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
    return 1;
  }

  if (use_css)
  {
    int status = validate_css(curl, stdin, stdout, css_format);

    curl_easy_cleanup(curl);
    return status;
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
