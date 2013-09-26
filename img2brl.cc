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
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <stdexcept>

#include <cgicc/Cgicc.h>
#include <cgicc/CgiUtils.h>
#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPContentHeader.h>
#include <cgicc/XHTMLDoctype.h>
#include <curl/curl.h>
#include <Magick++/Include.h>
#include <boost/config.hpp>
#include <boost/locale.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/version.hpp>

#include <sys/utsname.h>

#include "config.h"
#include "accept_language.h"
#include "ubrl.h"

using namespace boost::locale;
using namespace cgicc;

static code git_clone{"git&nbsp;<span lang=\"en\">clone</span>&nbsp;"
                      "http://img2brl.delysid.org"};

static int
curl_append_to_string(char *data, size_t size, size_t nmemb, std::string *buffer)
{
  if (not buffer) return 0;

  buffer->append(data, size*nmemb);

  return size * nmemb;
}

using namespace std;

enum class output_mode { html, json, text };

static cgicc::a
link_to_lang(cgicc::Cgicc const &cgi, std::string const &lang, std::string const &name) {
  std::map<std::string, std::string> params;
  for (auto element: cgi.getElements())
    if (element.getName() != "img" and element.getName() != "submit")
      params.insert({element.getName(), element.getValue()});
  params["lang"] = lang;
  std::stringstream href;
  href << "/";
  for (auto param = params.begin(); param != params.end(); ++param)
    href << (param == params.begin()? '?': '&')
         << cgicc::form_urlencode(param->first)
         << '='
         << cgicc::form_urlencode(param->second);
  cgicc::a link(name);
  link.set("href", href.str())
      .set("hreflang", lang)
      .set("lang", lang)
      .set("rel", "alternate");
  return link;
}

static std::map<std::string, std::tuple<boost::locale::message, std::string>> const languages {
  { "de", std::make_tuple(translate("German"), "Deutsch") },
  { "en", std::make_tuple(translate("English"), "English") },
  { "fr", std::make_tuple(translate("French"), "fran&ccedil;ais") },
  { "hu", std::make_tuple(translate("Hungarian"), "magyar") },
  { "it", std::make_tuple(translate("Italian"), "Italiano") }
};

static void
print_languages(cgicc::Cgicc const &cgi, std::string const &current_lang) {
  std::cout << cgicc::div().set("id", "languages") << std::endl;
  for (auto lang: languages)
    if (lang.first != current_lang)
      std::cout << link_to_lang(cgi, lang.first, std::get<1>(lang.second)) << std::endl;
  std::cout << cgicc::div();
}

static void
print_alternate(cgicc::Cgicc const &cgi, std::string const &current_lang) {
  std::map<std::string, std::string> params;
  for (auto element: cgi.getElements())
    if (element.getName() != "img" and element.getName() != "submit")
      params.insert({element.getName(), element.getValue()});
  for (auto lang: languages)
    if (lang.first != current_lang) {
      params["lang"] = lang.first;
      std::stringstream href;
      href << "/";
      for (auto param = params.begin(); param != params.end(); ++param)
        href << (param == params.begin()? '?': '&')
             << cgicc::form_urlencode(param->first)
             << '='
             << cgicc::form_urlencode(param->second);
      std::cout << cgicc::link().set("rel", "alternate")
                                .set("title", std::get<0>(lang.second))
                                .set("href", href.str())
                                .set("hreflang", lang.first) << std::endl;;
    }
}

static void
print_header(cgicc::Cgicc const &cgi, output_mode mode, std::string const &title, std::string const &lang)
{
  static char const *text_html_utf8 = "text/html; charset=UTF-8";
  switch (mode) {
    case output_mode::html:
      cout << "Content-Type: " << text_html_utf8 << endl
           << "Content-Language: " << lang << endl
           << "Vary: " << "Accept-Language" << endl
           << endl
           << XHTMLDoctype(XHTMLDoctype::eStrict) << endl
           << html().set("xmlns", "http://www.w3.org/1999/xhtml")
                    .set("lang", lang).set("dir", "ltr") << endl
           << head() << endl
           << cgicc::title() << title << cgicc::title() << endl
           << meta().set("http-equiv", "Content-Type")
                    .set("content", text_html_utf8) << endl
           << cgicc::link().set("rel", "shortcut icon")
                           .set("href", "favicon.png") << endl
           << cgicc::link().set("rel", "stylesheet").set("type", "text/css")
                           .set("href", "img2brl.css") << endl
           << cgicc::link().set("rel", "author")
                           .set("href", "http://delysid.org/") << endl;
      print_alternate(cgi, lang);
      cout << head() << endl
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
    std::cout << cgicc::dl().set("id", "supported-image-formats").set("lang", "en") << std::endl;
    for (std::size_t i = 0; i < formats; ++i) {
      if (info[i]->stealth == MagickCore::MagickFalse and
          info[i]->decoder and info[i]->magick) {
        std::cout << cgicc::dt(cgicc::abbr(info[i]->name))
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
        , std::string const &name, std::string const &id, std::string const &title
        )
{
  cgicc::input input;
  input.set("type", "checkbox").set("name", name).set("id", id);
  if (not title.empty()) input.set("title", title);
  if (cgi.queryCheckbox(name)) input.set("checked", "checked");
  return input;
}

static void
print_form(cgicc::Cgicc const &cgi)
{
  cgicc::const_file_iterator file(cgi.getFile("img"));
  cgicc::const_form_iterator lang(cgi.getElement("lang"));
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
  url_input.set("type", "url");
  url_input.set("name", "url");
  if (url != cgi.getElements().end()) url_input.set("value", url->getValue());

  std::string columns("88");
  if (cgi.getElement("cols") != cgi.getElements().end())
    columns = cgi.getElement("cols")->getValue();

  input columns_input;
  columns_input.set("type", "number")
               .set("name", "cols")
               .set("title", translate("Amount of characters a line of the "
                                       "braille output is not allowed to exceed."))
               .set("id", "cols_img")
               .set("size", "3")
               .set("value", columns);

  std::string action("/");
  if (lang != cgi.getElements().end()) action += "?lang=" + lang->getValue();

  cout << form().set("method", "post")
                .set("action", action)
                .set("enctype", "multipart/form-data") << endl
       << cgicc::div()
       << label(translate("Send an image file: ")).set("for", img_file) << endl
       << file_input << endl
       << cgicc::div() << endl
       << cgicc::div() << translate("or") << cgicc::div() << endl
       << cgicc::div()
       << label(translate("Enter URL to image: ")).set("for", img_url) << endl
       << url_input << endl
       << cgicc::div() << endl

       << cgicc::fieldset() << cgicc::legend(translate("Options:")) << endl
       << checkbox(cgi, "trim", "trim_img",
                   translate("Trims edges that are the background color from "
                             "the image."))
       << endl
       << label(translate("trim edges")).set("for", "trim_img") << endl
       << checkbox(cgi, "normalize", "normalize_img",
                   translate("Enhances the contrast of a color image by mapping "
                             "the darkest 2 percent of all pixels to black and "
                             "the brightest 1 percent to white.")) << endl
       << label(translate("increase contrast")).set("for", "normalize_img") << endl
       << checkbox(cgi, "negate", "negate_img",
                   translate("Negates the colors in the reference image."))
       << endl
       << label(translate("invert")).set("for", "negate_img") << endl
       << checkbox(cgi, "resize", "resize_img",
                   translate("Scales image such that it uses at most the "
                             "specified amount of characters horizontally."))
       << endl
       << format(translate("{1} max {2} {3}"))
          % label(translate("resize to")).set("for", "resize_img")
          % columns_input
          % label(translate("columns")).set("for", "cols_img")
       << cgicc::fieldset() << endl

       << script().set("type", "application/javascript")
       << "document.getElementById('cols_img').disabled = !document.getElementById('resize_img').checked;" << endl
       << "document.getElementById('resize_img').onchange = function() {" << endl
       << "  document.getElementById('cols_img').disabled = !this.checked;" << endl
       << "};" << endl
       << script() << endl

       << cgicc::div().set("class", "right topmargin") << endl
       << input().set("type", "submit")
                 .set("name", "submit")
                 .set("value", translate("Translate to Braille")) << endl
       << cgicc::div() << endl
       << form() << endl;
}

typedef std::chrono::steady_clock clock_type;

static void
print_footer(output_mode mode, clock_type::time_point const &start)
{
  if (mode == output_mode::html) {
    cout << body() << endl
         << html() << endl;
  } else if (mode == output_mode::json) {
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

int main()
{
  clock_type::time_point start_time = clock_type::now();
  boost::locale::generator locale_gen;
  locale_gen.add_messages_path("./locale");
  locale_gen.add_messages_domain("img2brl");

  output_mode mode{output_mode::html};
  std::string html_lang = "en";
  Cgicc cgi;

  if (char *value = std::getenv("HTTP_ACCEPT_LANGUAGE")) {
    std::stringstream msg;
    msg << "Accept-Language: " << value;
    try {
      accept_language spec(value);
      spec.normalize();
      msg << " -> " << spec;
      std::string selected_lang{spec.best_match({"en", "de", "fr", "hu", "it"}, "en")};
      locale::global(locale_gen(selected_lang+".UTF-8"));
      html_lang = selected_lang;
    } catch (std::runtime_error const &e) {
      msg << e.what() << endl;
    }
    msg << endl;
    cerr << msg.str();
  }
  if (cgi.getElement("lang") != cgi.getElements().end()) {
    std::set<std::string> const available_languages{"en", "de", "fr", "hu", "it"};
    if (available_languages.find(cgi("lang")) != available_languages.end()) {
      locale::global(locale_gen(cgi("lang")+".UTF-8"));
      html_lang = cgi("lang");
    }
  }

  cout.imbue(locale());

  try {
    if (cgi.getElement("mode") != cgi.getElements().end()) {
      std::map<std::string, output_mode> const modes = {
        { "html", output_mode::html },
        { "json", output_mode::json },
        { "text", output_mode::text }
      };
      try {
        mode = modes.at(cgi("mode"));
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
        curl_easy_cleanup(curl);
      }
      curl_global_cleanup();
    }

    print_header(cgi, mode, translate("Tactile Image Viewer"), html_lang);

    if (cgi("show") == "formats") {
      if (mode == output_mode::html) {
        cout << h1(translate("Supported image formats")) << endl;
        print_supported_image_formats();
      }
    }

    if (not data.get_data().empty()) {
      try {
        Magick::Blob blob(data.get_data().data(), data.get_data().length());
        Magick::Image image(blob);
        if (cgi.queryCheckbox("trim")) image.trim();
	if (cgi.queryCheckbox("normalize")) image.normalize();
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
	  switch (data.get_type()) {
	  case source::file: cout << "Filename: "; break;
	  case source::url: cout << "Url: "; break;
	  }
	  cout << data.get_identifier() << endl;
	  cout << "Content type: " << data.get_content_type() << endl;
	  cout << "Format: " << image.format() << endl;
	  if (not image.label().empty())
	    cout << "Label: " << image.label() << endl;
	  cout << "Width: " << tactile.width() << endl
	       << "Height: " << tactile.height() << endl << endl;
	} else if (mode == output_mode::json) {
	  cout << '"' << "src" << '"' << ':'
	       << '{';
	  cout << '"';
	  switch (data.get_type()) {
	  case source::file: cout << "filename";
	  case source::url: cout << "url";
	  }
	  cout << '"'
	       << ':'
	       << '"' << data.get_identifier() << '"'
	       << ','
	       << '"' << "content-type" << '"'
	       << ':'
	       << '"' << data.get_content_type() << '"'
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

	clock_type::duration duration = clock_type::now() - start_time;
	cout << cgicc::div().set("class", "center").set("id", "footer") << endl
	     << format(translate("Processing time was {3} {4} ({1} {2})"))
	        % span((format("{1}")
			% std::chrono::duration_cast<std::chrono::microseconds>
  			  (duration).count()
			).str()).set("class", "timing").set("id", "microseconds")
	        % translate("microseconds")
	        % span((format("{1,p=2}")
			% std::chrono::duration_cast<std::chrono::duration<double>>
			(duration).count()
			).str()).set("class", "timing").set("id", "seconds")
    	        % translate("seconds")
	     << cgicc::div() << endl;
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
    } else {
      if (mode == output_mode::html) {
        std::string href("?show=formats");
        if (not cgi("lang").empty()) href += "&lang="+cgi("lang");
        a unicode_braille(translate("Unicode braille"));
        unicode_braille.set("href",
                            translate("http://en.wikipedia.org/wiki/Unicode_braille"));
        cout << h1(translate("img2brl &mdash; Convert images to Braille")) << endl
             << p() << format(translate("Translate images from various {1} to {2}.")) 
                       % a(translate("formats")).set("class", "internal").set("href", href)
                       % unicode_braille
             << p() << endl;
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
           << translate("Install Firefox Add-on")
           << a() << endl
           << cgicc::div() << endl;

      a github_link("github.com/mlang/img2brl");
      github_link.set("href", "https://github.com/mlang/img2brl");
      github_link.set("hreflang", "en");
      cout << cgicc::div().set("class", "center") << endl
           << format(translate("There is an {1}.")) % a{"API"}.set("href", "https://github.com/mlang/img2brl/#api").set("hreflang", "en")
           << endl
           << format(translate("Source code? {1} or {2}."))
              % git_clone % github_link
           << cgicc::div() << endl;

      print_languages(cgi, html_lang);

      struct utsname info;
      if (uname(&info) != -1)
	cout << cgicc::div().set("class", "center").set("id", "powered-by") << endl
	     << (format(translate("Powered by {1}, {2}, {3}, {4}, {5} and {6} running on {7} ({8})."))
		 % BOOST_COMPILER
		 % (format("GNU&nbsp;cgicc&nbsp;{1}&nbsp;{2}") % translate("version") % cgi.getVersion())
		 % (format("libcurl&nbsp;{1}&nbsp;{2}.{3}.{4}")
		    % translate("version")
		    % LIBCURL_VERSION_MAJOR
		    % LIBCURL_VERSION_MINOR
		    % LIBCURL_VERSION_PATCH)
		 % (format("Magick++&nbsp;{1}&nbsp;{2}")
		    % translate("version")
		    % MAGICKPP_VERSION)
		 % (format("Boost&nbsp;{1}&nbsp;{2}.{3}.{4}")
		    % translate("version")
		    % (BOOST_VERSION / 100000)
		    % (BOOST_VERSION / 100 % 1000)
		    % (BOOST_VERSION % 100))
		 % (format("{1}&nbsp;{2}&nbsp;{3}")
		    % info.sysname % translate("version") % info.release)
		 % info.nodename
		 % cgi.getHost())
	     << cgicc::div() << endl;
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
    print_header(cgi, mode, "Error while fetching URL", html_lang);

    if (mode == output_mode::html) {
      cout << h1("An error occured while fetching URL") << endl
           << p("Please try again with a different URL.") << endl;

      print_form(cgi);
    }

    print_footer(mode, start_time);
  } catch (exception const &e) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }
}
