#include "ubrl.h"
#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
#include <boost/spirit/include/qi_char_.hpp>
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/spirit/include/qi_kleene.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_parse_attr.hpp>
#include <boost/spirit/include/qi_uint.hpp>

ubrl::ubrl(Magick::Image &image)
{
  Magick::Blob blob;
  image.write(&blob, "ubrl");
  char const *begin = static_cast<char const *>(blob.data());
  char const *const end = begin + blob.length();

  boost::spirit::qi::eoi_type eoi;
  boost::spirit::qi::eol_type eol;
  boost::spirit::qi::lit_type lit;
  boost::spirit::qi::uint_type uint_;
  boost::spirit::qi::char_type char_;
  boost::spirit::qi::
  parse( begin, end
       , lit("Width: ") > uint_ > eol
           > "Height: " > uint_ > eol
           > eol
           > *char_ > eoi
       , w, h, data);
}
