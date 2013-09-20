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
#include <map>
#include <string>
#include <stdexcept>
#include <iostream>

#include <cgicc/Cgicc.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPContentHeader.h>
#include <cgicc/XHTMLDoctype.h>
#include <curl/curl.h>
#include <Magick++/Include.h>
#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>
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

enum class output_mode { html, json, text };

static void
print_header(output_mode mode, std::string const &title)
{
  static char const *text_html_utf8 = "text/html; charset=UTF-8";
  switch (mode) {
    case output_mode::html:
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
                           .set("href", "img2brl.css") << endl
           << head() << endl
           << body() << endl;
      break;

    case output_mode::json:
      cout << HTTPContentHeader("application/json; charset=UTF-8") << '{';
      break;

    case output_mode::text:
      cout << HTTPContentHeader("text/plain; charset=UTF-8");
      break;
  }
}

static void
print_supported_image_formats()
{
  std::size_t formats;
  MagickCore::ExceptionInfo *exception = MagickCore::AcquireExceptionInfo();
  if (MagickCore::MagickInfo const **info = MagickCore::GetMagickInfoList("*", &formats, exception)) {
    std::cout << cgicc::dl().set("id", "supported-image-formats") << std::endl;
    for (std::size_t i = 0; i < formats; ++i) {
      if (info[i]->stealth == MagickCore::MagickFalse and
          info[i]->decoder and info[i]->magick) {
        std::cout << cgicc::dt(info[i]->name)
                  << cgicc::dd(info[i]->description) << std::endl;
      }
    }
    std::cout << cgicc::dl() << std::endl;
    MagickCore::RelinquishMagickMemory(info);
  }
  MagickCore::DestroyExceptionInfo(exception);
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

static void
print_form(cgicc::Cgicc const &cgi)
{
  cgicc::const_file_iterator file(cgi.getFile("img"));
  cgicc::const_form_iterator url(cgi.getElement("url"));

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

typedef std::chrono::steady_clock clock_type;

static void
print_footer(output_mode mode, clock_type::time_point const &start)
{
  clock_type::duration duration = clock_type::now() - start;
  if (mode == output_mode::html) {
    cout << cgicc::div().set("class", "center").set("id", "footer") << endl;
    cout << "Total time for request was "
         << span().set("class", "timing").set("id", "microseconds")
         << std::chrono::duration_cast<std::chrono::microseconds>(duration).count()
         << span() << " us"
         << " ("
         << span().set("class", "timing").set("id", "seconds")
         << std::chrono::duration_cast<std::chrono::duration<double>>(duration).count()
         << span() << " s)"
         << cgicc::div() << endl
	 << body() << endl
         << html() << endl;
  } else if (mode == output_mode::json) {
    cout << ','
	 << '"' << "runtime" << '"'
	 << ':'
	 << '{'
	 << '"' << "seconds" << '"'
	 << ':'
         << std::chrono::duration_cast<std::chrono::duration<double>>(duration).count()
         << '}';

    cout << '}';
  }
}

class source
{
public:
  enum type { unknown, url, file };
private:
  enum type type;
  std::string identifier;
  std::string content_type;
  std::string data;
public:
  source(): type{unknown} {}
  source( enum type type
        , std::string const &identifier
        , std::string const &content_type
        , std::string const &data
        )
  : type{type}, identifier{identifier}, content_type{content_type}, data{data}
  {}
public:
  enum type get_type() const { return type; };
  std::string const &get_identifier() const { return identifier; }
  std::string const &get_content_type() const { return content_type; }
  std::string const &get_data() const { return data; }
};

class http_error : public std::runtime_error
{
public:
  long code;
  http_error(long code): std::runtime_error("HTTP error"), code{code} {}
};

static void
print_image( output_mode mode
           , cgicc::Cgicc const &cgi
           , source const &src
           )
{
  try {
    Magick::Blob blob(src.get_data().data(), src.get_data().length());
    Magick::Image image(blob);
    if (cgi.queryCheckbox("trim"))
      image.trim();
    if (cgi.queryCheckbox("normalize"))
      image.normalize();
    if (cgi.queryCheckbox("negate")) {
      //  image.threshold(50.0);
      image.negate(true);
    }
    if (cgi.queryCheckbox("resize")) {
      const_form_iterator cols = cgi.getElement("cols");
      if (cols != cgi.getElements().end()) {
	try {
	  Magick::Geometry geometry( boost::lexical_cast<std::size_t>
				     (cols->getValue()) * 2
				   , 0
				   );
	  if (geometry.width()) {
	    geometry.less(false);
	    geometry.greater(true);
	    image.resize(geometry);
	  }
	} catch (boost::bad_lexical_cast const &e) {
	}
      }
    }

    ubrl tactile(image);

    if (mode == output_mode::html) {
      cout << pre().set("id", "result") << endl;
      switch (src.get_type()) {
        case source::file: cout << "Filename: "; break;
        case source::url: cout << "Url: "; break;
      }
      cout << src.get_identifier() << endl;
      cout << "Content type: " << src.get_content_type() << endl;
      cout << "Format: " << image.format() << endl;
      if (not image.label().empty())
        cout << "Label: " << image.label() << endl;
      cout << "Width: " << tactile.width() << endl
           << "Height: " << tactile.height() << endl << endl;
    } else if (mode == output_mode::json) {
      cout << '"' << "src" << '"' << ':'
           << '{';
      cout << '"';
      switch (src.get_type()) {
      case source::file: cout << "filename";
      case source::url: cout << "url";
      }
      cout << '"'
           << ':'
           << '"' << src.get_identifier() << '"'
           << ','
           << '"' << "content-type" << '"'
           << ':'
           << '"' << src.get_content_type() << '"'
           << ','
           << '"' << "format" << '"'
           << ':' << '"' << image.format() << '"'
           << ',';
      if (not image.label().empty())
        cout << '"' << "label" << '"' << ':' << '"' << image.label() << '"'
             << ',';
      if (not image.comment().empty())
        cout << '"' << "comment" << '"'
             << ':' << '"' << image.comment() << '"'
             << ',';
      cout << '"' << "width" << '"' << ':' << image.baseColumns()
           << ","
           << '"' << "height" << '"' << ':' << image.baseRows()
           << '}'

           << ',';
      cout << '"' << "width" << '"' << ':' << tactile.width()
           << ','
           << '"' << "height" << '"' << ':' << tactile.height()
           << ','
           << '"' << "braille" << '"' << ':' << '"';
    }

    cout << tactile.string();

    switch (mode) {
      case output_mode::html: cout << pre() << endl; break;
      case output_mode::json: cout << '"'; break;
      default: break;
    }
  } catch (Magick::ErrorMissingDelegate const &missing_delegate_exception) {
    switch (mode) {
      case output_mode::html:
        cout << h1("Error: Image format not supported") << endl
             << p(missing_delegate_exception.what()) << endl;
        break;
      case output_mode::json:
        cout << '"' << "exception" << '"'
             << ':'
             << '"' << "Magick::ErrorMissingDelegate" << '"'
             << ','
             << '"' << "message" << '"'
             << ':'
             << '"' << missing_delegate_exception.what() << '"';
        break;
      case output_mode::text:
        cout << "Unsupported image format: "
             << missing_delegate_exception.what() << endl;
    }
  }
}

int main()
{
  clock_type::time_point start_time = clock_type::now();
  output_mode mode(output_mode::html);
  Cgicc cgi;

  cerr << "Accept-Language: " << std::getenv("ACCEPT_LANGUAGE") << std::endl;
  try {
    cerr << nounitbuf;
    if (cgi.getElement("mode") != cgi.getElements().end()) {
      std::map<std::string, output_mode> const modes = {
        { "html", output_mode::html },
        { "json", output_mode::json },
        { "text", output_mode::text }
      };
      try {
        mode = modes.at(cgi.getElement("mode")->getValue());
      } catch (std::out_of_range const &e) {
        cerr << "Invalid mode '" << cgi.getElement("mode")->getValue()
             << "' specified, falling back to html." << endl;
      }
    }

    const_file_iterator file = cgi.getFile("img");
    const_form_iterator url = cgi.getElement("url");
    source data;

    if (file != cgi.getFiles().end() and not file->getData().empty()) {
      data = source(source::file, file->getFilename(), file->getDataType(), file->getData());
    } else if (url != cgi.getElements().end() and not url->getValue().empty()) {
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
//                    if (curl_easy_setopt(curl, CURLOPT_FAILONERROR, true) == CURLE_OK) {
                      if (curl_easy_perform(curl) == CURLE_OK) {
                        long http_response_code;
                        if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                                              &http_response_code) == CURLE_OK) {
                          if (http_response_code == 200 and not buffer.empty()) {
                            char *content_type;
                            if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE,
                                                  &content_type) == CURLE_OK) {
                              data = source(source::url, url->getValue(), content_type, buffer);
                            } else {
                              cerr << error_buffer << endl;
                            }
                          } else {
                            throw http_error(http_response_code);
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

    print_header(mode, "Tactile Image Viewer");

    if (cgi.getElement("show") != cgi.getElements().end() and cgi.getElement("show")->getValue() == "formats") {
      if (mode == output_mode::html) {
        cout << h1("Supported image formats") << endl;
        print_supported_image_formats();
      }
    }

    if (not data.get_data().empty()) {
      print_image(mode, cgi, data);
    } else {
      if (mode == output_mode::html) {
        a formats("formats");
        formats.set("href", "?show=formats");
        a unicode_braille("Unicode braille");
        unicode_braille.set("href",
                            "http://en.wikipedia.org/wiki/Unicode_braille");
        cout << h1("img2brl &mdash; Convert images to Braille") << endl
             << p() << "Translate images from various " << formats
                    << " to " << unicode_braille << '.' << p() << endl;
      }
    }

    if (mode == output_mode::html) {
      cout << hr() << endl;
      print_form(cgi);

      cout << hr() << endl;

      cout << script().set("type", "application/javascript")
           << "function install (aEvent) {" << endl
           << "  for (var a = aEvent.target; a.href === undefined;)" << endl
           << "    a = a.parentNode;" << endl
           << "  var params = {" << endl
           << "    'img2brl': { URL: aEvent.target.href," << endl
           << "                 IconURL: 'favicon.png'," << endl
           << "                 toString: function () { return this.URL; }" << endl
           << "               }" << endl
           << "  };" << endl
           << "  InstallTrigger.install(params);" << endl
           << "  return false;" << endl
           << "}" << endl
           << script() << endl
           << cgicc::div() << endl
           << a().set("href", "img2brl.xpi")
                 .set("onclick", "return install(event);")
           << "Install Firefox extension"
           << a() << endl
           << cgicc::div() << endl;

      code git_clone("git clone http://img2brl.delysid.org");
      a api_link("API");
      api_link.set("href", "https://github.com/mlang/img2brl/#api");
      a github_link("github.com/mlang/img2brl");
      github_link.set("href", "https://github.com/mlang/img2brl");
      cout << cgicc::div().set("class", "center") << endl
           << "There is an " << api_link << ". "
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
      if (uname(&info) != -1) {
        cout << ' ' << "and" << ' '
             << info.sysname << "&nbsp;" << "version" << "&nbsp;"
             << info.release << ' ' << "running on" << ' '
             << info.nodename << ' ' << '(' << cgi.getHost() << ')';
      }
      cout << '.' << cgicc::div() << endl;
    }

    print_footer(mode, start_time);

    return EXIT_SUCCESS;
  } catch (http_error const &e) {
    // See http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
    std::map<long, std::string> messages = {
      { 400, "Bad Request" },
      { 401, "Unauthorized" },
      { 402, "Payment Required" },
      { 403, "Forbidden" },
      { 404, "Not Found" },
      { 405, "Method Not Allowed" }
    };
    cout << "Status: " << e.code << ' ' << messages.at(e.code) << endl;
    print_header(mode, "Error while fetching URL");

    if (mode == output_mode::html) {
      cout << h1("An error occured while fetching URL") << endl
           << p("Please try again with a different URL.") << endl;

      print_form(cgi);
    }

    print_footer(mode, start_time);
  } catch (exception const &e) {
    cerr << "C++ exception: " << e.what() << endl;
    return EXIT_FAILURE;
  }
}
