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
#include <map>

#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPContentHeader.h>
#include <cgicc/XHTMLDoctype.h>
#include <curl/curl.h>
#include <Magick++/Blob.h>
#include <Magick++/Image.h>
#include <boost/config.hpp>
#include <boost/version.hpp>

#include <sys/utsname.h>

#include "config.h"
#include "ubrl.h"

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
       << cgicc::link().set("rel", "shortcut icon")
                .set("href", "favicon.png") << endl
       << cgicc::link().set("rel", "stylesheet").set("type", "text/css")
                .set("href", "img2brl.css")
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
       << checkbox(cgi, "trim", "trim_img")
       << label().set("for", "trim_img") << "trim" << label() << endl
       << checkbox(cgi, "normalize", "normalize_img")
       << label().set("for", "normalize_img") << "normalize" << label() << endl
       << checkbox(cgi, "negate", "negate_img")
       << label().set("for", "negate_img") << "negate" << label() << endl
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

       << cgicc::div().set("class", "center") << endl
       << input().set("type", "submit")
                 .set("name", "submit")
                 .set("value", "Translate to Braille") << endl
       << cgicc::div() << endl
       << form() << endl;
}

static void
manipulate(cgicc::Cgicc const &cgi, Magick::Image &image) {
  if (cgi.queryCheckbox("trim")) image.trim();
  if (cgi.queryCheckbox("normalize")) image.normalize();
  if (cgi.queryCheckbox("negate")) {
//  image.threshold(50.0);
    image.negate(true);
  }
  if (cgi.queryCheckbox("resize")) {
    const_form_iterator cols = cgi.getElement("cols");
    if (cols != cgi.getElements().end()) {
      std::string cols_str(cols->getValue());
      if (not cols_str.empty()) {
        char *end = NULL;
        long int width = strtol(cols_str.c_str(), &end, 10) * 2;
        if (end and *end == 0) {
          Magick::Geometry g(width, 0);
          g.less(false); g.greater(true);
          image.resize(g);
        }
      }
    }
  }
}

typedef std::chrono::steady_clock clock_type;

static void
print_xhtml_footer(clock_type::time_point const &start)
{
  cout << cgicc::div().set("class", "center") << endl;
  clock_type::time_point end = clock_type::now();
  cout << "Total time for request was "
       << span().set("class", "timing").set("id", "microseconds")
       << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
       << span() << " us"
       << " ("
       << span().set("class", "timing").set("id", "seconds")
       << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count()
       << span() << " s)";
  cout << cgicc::div() << endl;

  cout << body() << endl
       << html() << endl;
}

static void
print_image( std::string const &mode
           , cgicc::Cgicc const &cgi
           , std::string const &data
           , std::map<string, string> const &attrs
           )
{
  try {
    Magick::Blob blob(data.data(), data.length());
    Magick::Image image(blob);
    manipulate(cgi, image);
    ubrl tactile(image);
    if (mode == "html") {
      cout << pre().set("id", "result") << endl;
      for (std::pair<string, string> e: attrs) {
	cout << e.first << ": " << e.second << endl;
      }
      cout << "Format: " << image.format() << endl;
      if (not image.label().empty())
	cout << "Label: " << image.label() << endl;
      cout << "Width: " << tactile.width() << endl
	   << "Height: " << tactile.height() << endl << endl;
    } else if (mode == "json") {
      cout << "{\"src\":{";
      for (std::pair<string, string> e: attrs) {
	cout << "\"" << e.first << "\":\"" << e.second << "\",";
      }
      cout << "\"format\":\"" << image.format() << "\",";
      if (not image.label().empty())
	cout << "\"label\":\"" << image.label() << "\",";
      if (not image.comment().empty())
	cout << "\"comment\":\"" << image.comment() << "\",";
      cout << "\"width\":" << image.baseColumns() << ",";
      cout << "\"height\":" << image.baseRows() << "},";
      cout << "\"width\":" << tactile.width() << ",\"height\":" << tactile.height() << ','
	   << endl
	   << " \"braille\":\"";
    }

    cout << tactile.string();

    if (mode == "html") cout << pre() << endl;
    else if (mode == "json") cout << "\"}";
  } catch (Magick::ErrorMissingDelegate const &missing_delegate_exception) {
    if (mode == "html") {
      cout << h1("Error: Image format not supported") << endl;
      cout << p(missing_delegate_exception.what()) << endl;
    } else if (mode == "json") {
      cout << "{\"exception\":\"Magick::ErrorMissingDelegate\","
	   << "\"message\":\"" << missing_delegate_exception.what() << "\"}";
    } else {
      cout << "Unsupported image format: "
	   << missing_delegate_exception.what() << endl;
    }
  }
}

int main()
{
  clock_type::time_point start = clock_type::now();
  try {
    Cgicc cgi;
    std::string mode(cgi.getElement("mode") != cgi.getElements().end()?
                     cgi.getElement("mode")->getValue(): "html");

    if (mode == "html") {
      print_xhtml_header("Tactile Image Viewer");
      cout << body() << endl;
    } else if (mode == "text") {
      cout << HTTPContentHeader("text/plain; charset=UTF-8");
    } else if (mode == "json") {
      cout << HTTPContentHeader("application/json; charset=UTF-8");
    }

    const_file_iterator file = cgi.getFile("img");
    if (file != cgi.getFiles().end()) {
      std::map<string, string> map;
      map["filename"] = file->getFilename();
      map["content-type"] = file->getDataType();

      print_image(mode, cgi, file->getData(), map);
    }

    const_form_iterator url = cgi.getElement("url");
    if (url != cgi.getElements().end()) {
      curl_global_init(CURL_GLOBAL_DEFAULT);
      if (CURL *curl = curl_easy_init()) {
        char error_buffer[CURL_ERROR_SIZE];
        if (curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,
                             error_buffer) == CURLE_OK) {
          if (curl_easy_setopt(curl, CURLOPT_URL,
                               url->getValue().c_str()) == CURLE_OK) {
            if (curl_easy_setopt(curl, CURLOPT_USERAGENT,
                                 cgi.getEnvironment().getUserAgent().c_str()) == CURLE_OK) {
              if (curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,
                                   1L) == CURLE_OK) {
                if (curl_easy_setopt(curl, CURLOPT_MAXREDIRS,
                                     3L) == CURLE_OK) {
                  if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                                       curl_append_to_string) == CURLE_OK) {
                    std::string buffer;
                    if (curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                                         &buffer) == CURLE_OK) {
                      if (curl_easy_perform(curl) == CURLE_OK) {
                        long http_response_code;
                        if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                                              &http_response_code) == CURLE_OK) {
                          if (http_response_code == 200 and not buffer.empty()) {
                            char *content_type;
                            if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE,
                                                  &content_type) == CURLE_OK) {
			      std::map<string, string> map;
			      map["url"] = url->getValue();
			      map["content-type"] = content_type;

			      print_image(mode, cgi, buffer, map);
                            } else {
                              cerr << error_buffer << endl;
                            }
                          } else {
                            cerr << "HTTP " << http_response_code << ' '
				 << url->getValue() << endl;
                          }
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
        curl_easy_cleanup(curl);
      }
    }

    if (mode == "html") {
      print_form(cgi, file, url);

      cout << hr() << endl;

#if defined(IMG2BRL_XPI_HASH)
      cout << script().set("type", "application/javascript")
           << "function install (aEvent) {" << endl
           << "  for (var a = aEvent.target; a.href === undefined;)" << endl
           << "    a = a.parentNode;" << endl
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
           << script() << endl
           << cgicc::div() << endl
           << a().set("href", "img2brl.xpi")
                 .set("iconURL", "favicon.png")
                 .set("hash", IMG2BRL_XPI_HASH)
                 .set("onclick", "return install(event);")
           << "Install Firefox extension"
           << a() << endl
           << cgicc::div() << endl;
#endif //defined(IMG2BRL_XPI_HASH)

      code git_clone("git clone http://img2brl.delysid.org");
      a github_link("github.com/mlang/img2brl");
      github_link.set("href", "https://github.com/mlang/img2brl");
      cout << cgicc::div().set("class", "center") << endl
           << "Source code? " << git_clone << " or " << github_link << endl
           << cgicc::div() << endl;

      cout << cgicc::div().set("class", "center").set("id", "powered-by")
           << "Powered by" << ' '
           << BOOST_COMPILER << ',' << ' '
           << "GNU&nbsp;cgicc" << "&nbsp;" << "version" << "&nbsp;"
           << cgi.getVersion() << ',' << ' '
           << "libcurl" << "&nbsp;" << "version" << "&nbsp;"
           << LIBCURL_VERSION_MAJOR << '.'
           << LIBCURL_VERSION_MINOR << '.'
           << LIBCURL_VERSION_PATCH << ',' << ' '
           << "Magick++" << "&nbsp;" << "version" << "&nbsp;"
           << MAGICKPP_VERSION << ',' << ' '
           << "Boost" << "&nbsp;" << "version" << "&nbsp;"
           << BOOST_VERSION / 100000 << '.'
           << BOOST_VERSION / 100 % 1000 << '.'
           << BOOST_VERSION % 100;
      struct utsname info;
      if(uname(&info) != -1) {
        cout << ' ' << "and" << ' '
             << info.sysname << "&nbsp;" << "version" << "&nbsp;"
             << info.release << ' ' << "running on" << ' '
             << info.nodename << ' ' << '(' << cgi.getHost() << ')';
      }
      cout << '.' << cgicc::div() << endl;

      print_xhtml_footer(start);
    }

    return EXIT_SUCCESS;
  } catch (exception const &e) {
    cerr << "C++ exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }
}
