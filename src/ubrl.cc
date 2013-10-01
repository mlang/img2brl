#include "ubrl.h"
#include <Magick++/Blob.h>
#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
#include <boost/spirit/include/qi_char_.hpp>
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/spirit/include/qi_kleene.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_optional.hpp>
#include <boost/spirit/include/qi_parse_attr.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_sequence.hpp>
#include <boost/spirit/include/qi_uint.hpp>

ubrl::ubrl(Magick::Image const &image)
{
  Magick::Blob blob;
  Magick::Image(image).write(&blob, "ubrl");

  using namespace boost::spirit::qi;
  char const *begin = static_cast<char const *>(blob.data());
  char const *const end = begin + blob.length();
  char_type char_;
  eoi_type eoi;
  eol_type eol;
  lit_type lit;
  uint_type uint_;

  boost::optional<std::string> title;
  boost::optional<std::size_t> x, y;
  rule<char const *, std::string()> label = lit("Title: ") > *~char_('\n') > eol;
  rule<char const *, std::size_t()> x_offset = lit("X: ") > uint_ > eol;
  rule<char const *, std::size_t()> y_offset = lit("Y: ") > uint_ > eol;
  rule<char const *, std::size_t()> width = lit("Width: ") > uint_ > eol;
  rule<char const *, std::size_t()> height = lit("Height: ") > uint_ > eol;
  if (not
  parse( begin, end
	 , -label >> -x_offset >> -y_offset >> width >> height >> eol >> *char_ > eoi
	 , title, x, y, w, h, data))
    throw std::runtime_error("ubrl parse failed"+std::string(begin, end));
}
