# About
This is a simple CLI interface for the W3 HTML and CSS validation services. It is not approved by or affiliated with the W3. I wrote it to experiment with C and to make submitting locally stored HTML and CSS files for validation more ergonomic. Please use it responsibly.

## Building
This project depends on [libcurl](https://curl.se/libcurl/). To build, just run:
```bash
$ cc -o val val.c -lcurl
```

## Usage
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
Validate a CSS file named example.css and output the response as plain text.
```bash
$ val -c -f 6 < example.css
```