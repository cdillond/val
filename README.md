# About
This is a simple CLI interface for the W3 HTML and CSS validation services. It is not approved by or affiliated with the W3. I wrote it to experiment with C and to make submitting locally stored HTML and CSS files for validation more ergonomic. Please use it responsibly.

## Building
This application depends on [libcurl](https://curl.se/libcurl/). To build val, make sure libcurl is installed and then run:
```bash
$ cc -o val val.c -lcurl
```

## Usage
val accepts the following options.
- `-i`: input file name [arg required, default behavior: read from `stdin`]
- `-o`: output file name [arg required, default behavior: write to `stdout`]
- `-c`: validate CSS input [default behavior: validate HTML input]
- `-x`: format HTML validation response as XML [must not be used in combination with `-c` or `-f`, default behavior: format HTML validation response as JSON]
- `-f`: format CSS validation response according to the provided arg (see below) [must not be used in combination with `-x`, default behavior: `text/plain`]
  - 0 = `text/plain`
  - 1 = `text`
  - 2 = `text/html`
  - 3 = `html`
  - 4 = `application/xhtml+xml`
  - 5 = `xhtml`
  - 6 = `application/soap+xml`
  - 7 = `soap12`

## Examples
Validate an HTML file named example.html using the default options.
```bash
$ val < example.html
```
Validate an HTML file named example.html and pipe the output to jq for a nicely formatted response.
```bash
$ val -i example.html | jq
```
Validate an HTML file named example.html and save the output, formatted as xml, to out.xml.
```bash
$ val -i example.html -x -o out.xml
```
Validate a CSS file named example.css and output the response in a soap12 format.
```bash
$ val -c -f 7 < example.css
```