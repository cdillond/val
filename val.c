#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>

/* needed for HTML validation requests */
#define HTML_JSON_URL "https://validator.w3.org/nu/?out=json"
#define HTML_XML_URL "https://validator.w3.org/nu/?out=xml"
#define HTML_CONTENT_TYPE "Content-Type: text/html; charset=utf-8"
#define USER_AGENT "html validator client"

/* needed for CSS validation requests */
#define CSS_URL "https://jigsaw.w3.org/css-validator/validator?"
#define CSS_QUERY_PARAM "text="

enum CSS_OUTPUT_FORMAT
{
  TEXT_PLAIN,
  TEXT,
  TEXT_HTML,
  HTML,
  XHTML_XML,
  XHTML,
  SOAP_XML,
  SOAP12,
};

size_t read_callback(char *buf, size_t size, size_t n_memb, void *in_file);
size_t write_callback(char *buf, size_t size, size_t n_memb, void *out_file);
int validate_css(CURL *curl, FILE *in, FILE *out, enum CSS_OUTPUT_FORMAT format);

int main(int argc, char *argv[])
{
  /* default settings */
  int status = 1;
  FILE *in = stdin;
  FILE *out = stdout;
  const char *url = HTML_JSON_URL;
  const char *content_type = HTML_CONTENT_TYPE;
  int use_css = 0;
  enum CSS_OUTPUT_FORMAT css_format = TEXT_PLAIN;

  /* parse args */
  extern char *optarg;
  int opt;
  while ((opt = getopt(argc, argv, "i:o:f:xc")) != -1)
  {
    switch (opt)
    {
    case 'i':
      in = fopen(optarg, "r");
      if (in == NULL)
      {
        fprintf(stderr, "unable to open input file %s\n", optarg);
        perror(NULL);
        return status;
      }
      break;
    case 'o':
      out = fopen(optarg, "w");
      if (out == NULL)
      {
        fprintf(stderr, "unable to open output file %s\n", optarg);
        perror(NULL);
        return status;
      }
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
      fprintf(stderr, "Usage: %s [-i in_path] [-o out_path] [-x xml HTML response output] [-c use CSS mode] [-f CSS response format]\n", argv[0]);
      goto close_io;
    }
  }

  /* initialize CURL context */
  CURL *curl = curl_easy_init();
  if (!curl)
  {
    fputs("error: unable to initialize CURL context\n", stderr);
    goto close_io;
  }

  if (use_css)
    status = validate_css(curl, in, out, css_format);
  else
  {
    /* HTML validation; much simpler than what's required for CSS */
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

    status = curl_easy_perform(curl);
    curl_slist_free_all(list);
  }

  if (status)
    fprintf(stderr, "CURL error: response code %d\n", status);

  curl_easy_cleanup(curl);

close_io:
  fclose(in);
  fclose(out);

  return status;
}

size_t read_callback(char *buf, size_t size, size_t n_memb, void *in_file)
{
  return fread(buf, size, n_memb, (FILE *)in_file);
}

size_t write_callback(char *buf, size_t size, size_t n_memb, void *out_file)
{
  return fwrite(buf, size, n_memb, (FILE *)out_file);
}

CURLUcode CSS_build_URL(CURLU *url, const char *restrict format, const char *restrict query)
{

  CURLUcode rc = curl_url_set(url, CURLUPART_URL, CSS_URL, 0);
  if (rc != 0)
  {
    fprintf(stderr, "CURL URL error: code %d\n", rc);
    return rc;
  }

  rc = curl_url_set(url, CURLUPART_QUERY, query, CURLU_APPENDQUERY | CURLU_URLENCODE);
  if (rc != 0)
  {
    fprintf(stderr, "CURL URL error: code %d\n", rc);
    return rc;
  }

  rc = curl_url_set(url, CURLUPART_QUERY, format, CURLU_APPENDQUERY | CURLU_URLENCODE);
  if (rc != 0)
    fprintf(stderr, "CURL URL error: code %d\n", rc);

  return rc;
}

int CSS_http_get(CURL *curl, CURLU *url, FILE *out)
{
  /* begin constructing the HTTP request */
  curl_easy_setopt(curl, CURLOPT_CURLU, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, out);

  int status = curl_easy_perform(curl); /* treat this as a regular int */
  if (status)
    fprintf(stderr, "CURL error: response code %d\n", status);

  return status;
}

/* CSS format enum as a string; returned string must not be freed or written to */
static const char *CSS_format_str(enum CSS_OUTPUT_FORMAT format)
{
  static const char *formats[] = {
      "output=text/plain",
      "output=text",
      "output=text/html",
      "output=html",
      "output=application/xhtml+xml",
      "output=xhtml",
      "output=application/soap+xml",
      "output=soap12",
  };

  if (format < 0 || format >= (sizeof formats / sizeof *formats))
  {
    fprintf(stderr, "invalid CSS response format argument %d\n", format);
    return NULL;
  }

  return formats[format];
}

/* returns the len of the input stream or 0 on failure */
size_t CSS_data_len(FILE *in)
{
  if (fseek(in, 0, SEEK_END) == -1)
  {
    fputs("error: unable to seek input file\n", stderr);
    perror(NULL);
    return 0;
  }

  long offset = ftell(in);
  if (offset == -1)
  {
    fputs("error: unable to determine input file size\n", stderr);
    perror(NULL);
    return 0;
  }

  rewind(in);

  return (size_t)offset;
}

/* validat_css builds an HTTP request that passes the input file data via a URL encoded query
 and writes the response to out. The return value is 0 on success and nonzero on failure. */
int validate_css(CURL *curl, FILE *in, FILE *out, enum CSS_OUTPUT_FORMAT format)
{
  int status = 1;

  const char *format_str = CSS_format_str(format);
  if (format_str == NULL)
    return status;

  size_t data_len = CSS_data_len(in);
  if (data_len == 0)
    return status;

  /* allocate query buffer */
  size_t param_len = sizeof CSS_QUERY_PARAM - 1; /* do not include null terminator here */
  size_t buf_len = param_len + data_len;
  char *buf = malloc(buf_len); /* include space for a null terminator */

  /* write query to buf */
  memcpy(buf, CSS_QUERY_PARAM, param_len);
  size_t n = fread(buf + param_len, sizeof *buf, data_len, in);
  if (n != data_len)
  {
    fputs("error reading input file\n", stderr);
    perror(NULL);
    goto free_data;
  }
  buf[buf_len] = '\0';

  /* construct URL with query string*/
  CURLU *url = curl_url();
  CURLUcode rc = CSS_build_URL(url, format_str, buf);
  if (rc)
    goto cleanup_url;

  /* send GET request */
  status = CSS_http_get(curl, url, out);

cleanup_url:
  curl_url_cleanup(url);
free_data:
  free(buf);

  return status;
}
