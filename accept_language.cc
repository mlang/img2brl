#include "accept_language.h"
#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
#include <boost/spirit/include/qi_attr.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_eps.hpp>
#include <boost/spirit/include/qi_expect.hpp>
#include <boost/spirit/include/qi_list.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/qi_optional.hpp>
#include <boost/spirit/include/qi_parse_attr.hpp>
#include <boost/spirit/include/qi_plus.hpp>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_repeat.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_sequence.hpp>

BOOST_FUSION_ADAPT_STRUCT(
  accept_language::entry,
  (std::vector<std::string>, subtags)
  (boost::optional<float>, q)
)

accept_language::accept_language(std::string const &input)
{
  using namespace boost::spirit::qi;
  alpha_type alpha;
  attr_type attr;
  char_type char_;
  eoi_type eoi;
  eps_type eps;
  lit_type lit;
  real_parser<float, ureal_policies<float>> qvalue;
  repeat_type repeat;
  string_type string;
  rule<std::string::const_iterator, entry(), space_type> all
  = repeat(1)[string("*")] >> attr(boost::optional<float>());
  rule<std::string::const_iterator, entry(), space_type> entry_
  = (+alpha % '-' >> -(lit(";") > "q" > "=" > qvalue)) | all;
  if (not input.empty())
    phrase_parse( input.begin(), input.end()
                , eps > entry_ % ',' > eoi, space_type()
                , entries
                );
}
