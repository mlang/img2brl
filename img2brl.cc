/*
 *  Copyright (C) 2013 Mario Lang <mlang@delysid.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA 
 */

#include <chrono>
#include <new>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

#include <cgicc/CgiDefs.h>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/XHTMLDoctype.h>
#include <curl/curl.h>
#include <Magick++/Blob.h>
#include <Magick++/Image.h>

#include <sys/utsname.h>
#include <sys/time.h>

static int
write_to_buffer(char *data, size_t size, size_t nmemb, std::string *buffer)
{
  if (not buffer) return 0;

  buffer->append(data, size*nmemb);

  return size * nmemb;
}

using namespace std;
using namespace cgicc;

static void
print_xhtml_header(std::string const &title)
{
  static char const *text_html_utf8 = "text/html; charset=UTF-8";

  cout << HTTPContentHeader(text_html_utf8)
       << XHTMLDoctype(XHTMLDoctype::eStrict) << endl
       << html().set("xmlns", "http://www.w3.org/1999/xhtml")
                .set("lang", "en").set("dir", "ltr") << endl
       << head() << endl
       << cgicc::title() << title << cgicc::title() << endl
       << meta().set("http-equiv", "Content-Type")
                .set("content", text_html_utf8) << endl
       << link().set("rel", "shortcut icon")
                .set("href", "favicon.png") << endl
       << head() << endl;
}

static void print_form(Cgicc const &cgi)
{
  static char const *img_file = "img_file";
  static char const *img_url = "img_url";
  cout << form().set("method", "post")
                .set("action", cgi.getEnvironment().getScriptName())
                .set("enctype", "multipart/form-data") << endl
       << cgicc::div()
       << label().set("for", img_file) << "Send an image file: " << label() << endl
       << input().set("id", img_file)
                 .set("type", "file")
                 .set("name", "img")
                 .set("accept", "image/*") << endl
       << cgicc::div()
       << cgicc::div() << "or" << cgicc::div()
       << cgicc::div()
       << label().set("for", img_url) << "URL to image: " << label() << endl
       << input().set("id", img_url)
                 .set("type", "text")
                 .set("name", "url") << endl
       << cgicc::div()

       << cgicc::div().set("style", "text-align: center") << endl
       << input().set("type", "submit")
                 .set("name", "submit")
                 .set("value", "Translate to Braille") << endl
       << cgicc::div() << endl
       << form() << endl;
}

int main()
{
  typedef std::chrono::steady_clock clock;
  clock::time_point start = clock::now();
  try {
    Cgicc cgi;

    print_xhtml_header("Tactile Image Viewer");
    cout << body() << endl;

    const_file_iterator file = cgi.getFile("img");
    if (file != cgi.getFiles().end()) {
      cout << pre() << endl
           << "Data Type: " << file->getDataType() << endl
           << "Filename: " << file->getFilename() << endl;
      Magick::Blob blob(file->getData().data(), file->getData().length());
      Magick::Image image(blob);
      image.write("ubrl:-");
      cout << pre() << endl;
    }

    form_iterator url = cgi.getElement("url");
    if (url != cgi.getElements().end()) {
      curl_global_init(CURL_GLOBAL_DEFAULT);
      if (CURL *conn = curl_easy_init()) {
        char error_buffer[CURL_ERROR_SIZE];
        if (curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, error_buffer) == CURLE_OK) {
          if (curl_easy_setopt(conn, CURLOPT_URL, url->getValue().c_str()) == CURLE_OK) {
            if (curl_easy_setopt(conn, CURLOPT_FOLLOWLOCATION, 1L) == CURLE_OK) {
              if (curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, write_to_buffer) == CURLE_OK) {
                std::string buffer;
                if (curl_easy_setopt(conn, CURLOPT_WRITEDATA, &buffer) == CURLE_OK) {
                  if (curl_easy_perform(conn) == CURLE_OK) {
                    long http_response_code;
                    if (curl_easy_getinfo(conn, CURLINFO_RESPONSE_CODE, &http_response_code) == CURLE_OK) {
                      if (http_response_code == 200 and not buffer.empty()) {
                        cout << pre() << endl
                         //  << "Data Type: " << file->getDataType() << endl
                             << "URL: " << url->getValue() << endl;
                        Magick::Blob blob(buffer.data(), buffer.length());
                        Magick::Image image(blob);
                        image.write("ubrl:-");
                        cout << pre() << endl;
                      } else {
                        cerr << error_buffer << endl;
                      }
                    }
                  }
                }
              }
            }
          }
        }
        curl_easy_cleanup(conn);
      }
    }

    print_form(cgi);

    cout << hr() << endl;

    cout << cgicc::div().set("style", "text-align: center") << endl
         << a().set("href", "https://github.com/mlang/img2brl")
         << "Source at GitHub"
         << a() << endl
         << cgicc::div() << endl;

    cout << cgicc::div().set("style", "text-align: center") << endl
         << "Configured for " << cgi.getHost();  
    struct utsname info;
    if(uname(&info) != -1) {
      cout << ". Running on " << info.sysname;
      cout << ' ' << info.release << " (";
      cout << info.nodename << ")." << endl;
    }

    // Information on this query
    clock::time_point end = clock::now();
    cout << br() << "Total time for request = "
         << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
         << " us";
    cout << " (" << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() << " s)";
    cout << cgicc::div() << endl;

    cout << body() << html() << endl;

    return EXIT_SUCCESS;
  } catch (exception const &e) {
    // Reset all the HTML elements that might have been used to 
    // their initial state so we get valid output
    html::reset(); 	head::reset(); 		body::reset();
    title::reset(); 	h1::reset(); 		h4::reset();
    comment::reset(); 	pre::reset(); 		tr::reset(); 
    cgicc::div::reset(); 	p::reset(); 
    a::reset();		h2::reset(); 		colgroup::reset();

    print_xhtml_header("exception");    
    cout << body() << endl;
    
    cout << h1() << "GNU cgi" << span("cc", set("class","red"))
	 << " caught an exception" << h1() << endl; 
  
    cout << cgicc::div().set("align","center").set("class","notice") << endl;

    cout << h2(e.what()) << endl;

    // End of document
    cout << cgicc::div() << endl;
    cout << hr() << endl;
    cout << body() << html() << endl;
    
    return EXIT_SUCCESS;
  }
}
