/*
 *  Copyright (C) 2013 Mario Lang <mlang@delysid.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Affero General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU LAffero General Public
 *  License along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA. 
 */

#include <chrono>
#include <string>
#include <stdexcept>
#include <iostream>

#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPContentHeader.h>
#include <cgicc/XHTMLDoctype.h>
#include <curl/curl.h>
#include <Magick++/Blob.h>
#include <Magick++/Image.h>

#include <sys/utsname.h>

#include "config.h"

static int
curl_append_to_string(char *data, size_t size, size_t nmemb, std::string *buffer)
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

static cgicc::input
checkbox( cgicc::Cgicc const &cgi
        , std::string const &name, std::string const &id
        )
{
  cgicc::input input;
  input.set("type", "checkbox").set("name", name).set("id", id);
  if (cgi.queryCheckbox(name)) input.set("checked", "checked");
  return input;
}

class labeled_checkbox
{
  cgicc::Cgicc const &cgi;
  std::string name;
public:
  labeled_checkbox(cgicc::Cgicc const &cgi, std::string const &name)
  : cgi{cgi}, name{name} {}
  friend std::ostream &
  operator<<(std::ostream &stream, labeled_checkbox const &cb)
  {
    std::string id(cb.name);
    id.append("_id");
    stream << checkbox(cb.cgi, cb.name, id)
           << label().set("for", id) << cb.name << label();
    return stream;
  }
};

static void
print_form( cgicc::Cgicc const &cgi
          , cgicc::const_file_iterator file, cgicc::const_form_iterator url
          )
{
  static char const *img_file = "img_file";
  static char const *img_url = "img_url";

  input file_input;
  file_input.set("id", img_file);
  file_input.set("type", "file");
  file_input.set("name", "img");
  file_input.set("accept", "image/*");
  if (file != cgi.getFiles().end()) file_input.set("value", file->getFilename());

  input url_input;
  url_input.set("id", img_url);
  url_input.set("type", "text");
  url_input.set("name", "url");
  if (url != cgi.getElements().end()) url_input.set("value", url->getValue());

  std::string columns("88");
  if (cgi.getElement("cols") != cgi.getElements().end())
    columns = cgi.getElement("cols")->getValue();

  input columns_input;
  columns_input.set("type", "text")
               .set("name", "cols")
               .set("id", "cols_img")
               .set("size", "4").set("value", columns);

  cout << form().set("method", "post")
                .set("action", cgi.getEnvironment().getScriptName())
                .set("enctype", "multipart/form-data") << endl
       << cgicc::div()
       << label().set("for", img_file) << "Send an image file: " << label() << endl
       << file_input << endl
       << cgicc::div() << endl
       << cgicc::div() << "or" << cgicc::div() << endl
       << cgicc::div()
       << label().set("for", img_url) << "URL to image: " << label() << endl
       << url_input << endl
       << cgicc::div() << endl

       << cgicc::div() << endl
       << checkbox(cgi, "trim", "trim_img") << label().set("for", "trim_img") << "trim" << label() << endl
       << checkbox(cgi, "normalize", "normalize_img") << label().set("for", "normalize_img") << "normalize" << label() << endl
       << checkbox(cgi, "negate", "negate_img") << label().set("for", "negate_img") << "negate" << label() << endl
       << checkbox(cgi, "resize", "resize_img")
       << label().set("for", "resize_img") << "resize to max" << label()
       << columns_input
       << ' ' << label().set("for", "cols_img") << "columns" << label()
       << cgicc::div() << endl

       << script().set("type", "application/javascript")
       << "document.getElementById('cols_img').disabled = !document.getElementById('resize_img').checked;" << endl
       << "document.getElementById('resize_img').onchange = function() {" << endl
       << "  document.getElementById('cols_img').disabled = !this.checked;" << endl
       << "};" << endl
       << script() << endl

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
      Magick::Blob blob(file->getData().data(), file->getData().length());
      Magick::Image image(blob);
      cout << pre() << endl
           << "Content Type: " << file->getDataType() << endl
           << "Format: " << image.format() << endl
           << "Filename: " << file->getFilename() << endl;
      if (cgi.queryCheckbox("trim")) image.trim();
      if (cgi.queryCheckbox("normalize")) image.normalize();
      if (cgi.queryCheckbox("negate")) {
//      image.threshold(50.0);
        image.negate(true);
      }
      if (cgi.queryCheckbox("resize")) {
        const_form_iterator cols = cgi.getElement("cols");
        if (cols != cgi.getElements().end()) {
          std::string cols_str(cols->getValue());
          if (not cols_str.empty()) {
            Magick::Geometry g(0, 0);
            char *end = NULL;
            long int c = strtol(cols_str.c_str(), &end, 10);
            if (*end == 0) {
              g.width(c*2);
              g.less(false); g.greater(true);
              image.resize(g);
            }
          }
        }
      }
      image.write("ubrl:-");
      cout << pre() << endl;
    }

    const_form_iterator url = cgi.getElement("url");
    if (url != cgi.getElements().end()) {
      curl_global_init(CURL_GLOBAL_DEFAULT);
      if (CURL *conn = curl_easy_init()) {
        char error_buffer[CURL_ERROR_SIZE];
        if (curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, error_buffer) == CURLE_OK) {
          if (curl_easy_setopt(conn, CURLOPT_URL, url->getValue().c_str()) == CURLE_OK) {
            if (curl_easy_setopt(conn, CURLOPT_USERAGENT, cgi.getEnvironment().getUserAgent().c_str()) == CURLE_OK) {
              if (curl_easy_setopt(conn, CURLOPT_FOLLOWLOCATION, 1L) == CURLE_OK) {
                if (curl_easy_setopt(conn, CURLOPT_MAXREDIRS, 3L) == CURLE_OK) {
                  if (curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, curl_append_to_string) == CURLE_OK) {
                    std::string buffer;
                    if (curl_easy_setopt(conn, CURLOPT_WRITEDATA, &buffer) == CURLE_OK) {
                      if (curl_easy_perform(conn) == CURLE_OK) {
                        long http_response_code;
                        if (curl_easy_getinfo(conn, CURLINFO_RESPONSE_CODE, &http_response_code) == CURLE_OK) {
                          if (http_response_code == 200 and not buffer.empty()) {
                            char *content_type;
                            if (curl_easy_getinfo(conn, CURLINFO_CONTENT_TYPE, &content_type) == CURLE_OK) {
                              Magick::Blob blob(buffer.data(), buffer.length());
                              Magick::Image image(blob);
                              cout << pre() << endl
                                   << "Content Type: " << content_type << endl
                                   << "Format: " << image.format() << endl
                                   << "URL: " << url->getValue() << endl;
                              if (cgi.queryCheckbox("trim")) image.trim();
                              if (cgi.queryCheckbox("normalize")) image.normalize();
                              if (cgi.queryCheckbox("negate")) {
//                              image.threshold(50.0);
                                image.negate(true);
                              }
                              if (cgi.queryCheckbox("resize")) {
                                const_form_iterator cols = cgi.getElement("cols");
                                if (cols != cgi.getElements().end()) {
                                  std::string cols_str(cols->getValue());
                                  if (not cols_str.empty()) {
                                    Magick::Geometry g(0, 0);
                                    char *end = NULL;
                                    long int c = strtol(cols_str.c_str(), &end, 10);
                                    if (*end == 0) {
                                      g.width(c*2);
                                      g.less(false); g.greater(true);
                                      image.resize(g);
                                    }
                                  }
                                }
                              }
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
            }
          }
        }
        curl_easy_cleanup(conn);
      }
    }

    print_form(cgi, file, url);

    cout << hr() << endl;

    cout << cgicc::div().set("style", "text-align: center") << endl
         << "Sourcecode? git clone http://img2brl.delysid.org/ or >"
         << a().set("href", "https://github.com/mlang/img2brl")
         << "github.com/mlang/img2brl"
         << a() << endl
         << cgicc::div() << endl;

    cout << script().set("type", "application/javascript")
         << "function install (aEvent) {" << endl
         << "  for (var a = aEvent.target; a.href === undefined;) a = a.parentNode;" << endl
         << "  var params = {" << endl
         << "    'img2brl': { URL: aEvent.target.href," << endl
         << "                 IconURL: aEvent.target.getAttribute('iconURL')," << endl
         << "                 Hash: aEvent.target.getAttribute('hash')," << endl
         << "                 toString: function () { return this.URL; }" << endl
         << "               }" << endl
         << "  };" << endl
         << "  InstallTrigger.install(params);" << endl
         << "  return false;" << endl
         << "}" << endl
         << script()  << endl
         << cgicc::div() << endl
         << a().set("href", "img2brl.xpi")
               .set("iconURL", "favicon.png")
               .set("hash", "sha512:"+std::string(IMG2BRL_XPI_SHA512))
               .set("onclick", "return install(event);")
         << "Install Firefox extension"
         << a() << endl
         << cgicc::div() << endl;
    cout << cgicc::div().set("style", "text-align: center") << endl
         << comment() << "Configured for " << cgi.getHost();  
    struct utsname info;
    if(uname(&info) != -1) {
      cout << ". Running on " << info.sysname;
      cout << ' ' << info.release << " (";
      cout << info.nodename << ").";
    }
    cout << comment() << endl;

    // Information on this query
    clock::time_point end = clock::now();
    cout << "Total time for request was "
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
