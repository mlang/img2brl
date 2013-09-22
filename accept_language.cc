#include "accept_language.h"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;

BOOST_FUSION_ADAPT_STRUCT(
  accept_language::entry,
  (std::vector<std::string>, subtags)
  (boost::optional<float>, q)
)

namespace /* anonymous */ { // no external linkage
    using namespace qi;
    typedef std::string::const_iterator It;

    static const rule<It, std::vector<std::string>(), space_type> any
        = repeat(1)[ string("*") ]
        ;

    static const rule<It, std::vector<std::string>(), space_type> range
        = +alpha % '-'
        | any
        ;

    static const rule<It, float(), space_type> qvalue
        = lit(';') > 'q' > '=' > float_
        ;

    static const rule<It, accept_language::entry(), space_type> entry_
        = range >> -qvalue
        ;

}

accept_language::accept_language(std::string const& input)
{
    if (!phrase_parse(input.begin(), input.end(), -(entry_ % ',') > eoi, space, entries))
        throw std::runtime_error("invalid accept_language header"); // TODO rethink?
}

void test(std::string const& header)
{
    try
    {
        accept_language al(header);
        std::cout << "Success: '" << header << "'\n";

        for (auto& e : al.languages())
        {
            std::cout << e.subtags.size() << " subtags";
            if (e.q) std::cout << " with q=" << *e.q << '\n';
            else     std::cout << " with no q specified\n";
        }
    } catch(...)
    {
        std::cout << "Failed:  '" << header << "'\n";
        throw;
    }
}

int main()
{
    test(/*"Accept-Language:"*/ " da, en-gb;q=0.8, en;q=0.7");
    test(/*"Accept-Language:"*/ " *");
    test(/*"Accept-Language:"*/ " *;q=3.4");
    test(/*"Accept-Language:"*/ "");
    test(/*"Accept-Language:"*/ " ");
}
