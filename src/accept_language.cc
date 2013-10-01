#include "accept_language.h"
#define BOOST_SPIRIT_NO_PREDEFINED_TERMINALS
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi_alternative.hpp>
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

using namespace boost::spirit::qi;

namespace {
  alpha_type alpha;
  eoi_type eoi;
  eps_type eps;
  lit_type lit;
  real_parser<float, ureal_policies<float>> qvalue;
  repeat_type repeat;
  space_type space;
  string_type string;
  rule<std::string::const_iterator, std::vector<std::string>(), space_type> range
  = +alpha % '-' | repeat(1)[string("*")];
  rule<std::string::const_iterator, float(), space_type> q
  = lit(";") > "q" > "=" > qvalue;
}

accept_language::accept_language(std::string const &input)
{
  if (not input.empty())
    phrase_parse( input.begin(), input.end()
                , eps > (range >> -q) % ',' > eoi, space, entries
                );
}
