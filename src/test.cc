#include <algorithm>

#define BOOST_TEST_MODULE img2brl_test
#include <boost/test/included/unit_test.hpp>

#include "accept_language.h"

BOOST_AUTO_TEST_CASE(accept_language_1) {
  BOOST_REQUIRE(accept_language("").languages().empty());
}

BOOST_AUTO_TEST_CASE(accept_language_2) {
  accept_language accept("*");

  BOOST_REQUIRE_EQUAL(accept.languages().size(), 1);
  BOOST_CHECK(std::none_of(accept.languages().begin(), accept.languages().end(),
                           [](accept_language::entry const &language) -> bool {
                             return language.q;
                           }));
  BOOST_REQUIRE_EQUAL(accept.languages()[0].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[0], "*");
};

BOOST_AUTO_TEST_CASE(accept_language_3) {
  accept_language accept("*; q=0.1");

  BOOST_REQUIRE_EQUAL(accept.languages().size(), 1);
  BOOST_REQUIRE(std::all_of(accept.languages().begin(), accept.languages().end(),
                            [](accept_language::entry const &language) -> bool {
                              return language.q;
                            }));
  BOOST_REQUIRE_EQUAL(accept.languages()[0].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[0], "*");
  BOOST_CHECK_CLOSE(*accept.languages()[0].q, 0.1, 0.0001);
};

BOOST_AUTO_TEST_CASE(accept_language_4) {
  accept_language accept("de-at, de, en");

  BOOST_REQUIRE_EQUAL(accept.languages().size(), 3);
  BOOST_CHECK(std::none_of(accept.languages().begin(), accept.languages().end(),
                           [](accept_language::entry const &language) -> bool {
                             return language.q;
                           }));
  BOOST_REQUIRE_EQUAL(accept.languages()[0].subtags.size(), 2);
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[0], "de");
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[1], "at");
  BOOST_REQUIRE_EQUAL(accept.languages()[1].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[1].subtags[0], "de");
  BOOST_REQUIRE_EQUAL(accept.languages()[2].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[2].subtags[0], "en");
  BOOST_CHECK(accept.accepts_language("de"));
  BOOST_CHECK(accept.accepts_language("en"));
  BOOST_CHECK(not accept.accepts_language("fr"));
}

BOOST_AUTO_TEST_CASE(accept_language_5) {
  accept_language accept("de-at;q=1.0, de;q=0.9, en;q=0.7, es;q=0.1");
  float epsilon = 0.00001;

  BOOST_REQUIRE_EQUAL(accept.languages().size(), 4);
  BOOST_CHECK(std::all_of(accept.languages().begin(), accept.languages().end(),
                          [](accept_language::entry const &language) -> bool {
                            return language.q;
                          }));
  BOOST_REQUIRE_EQUAL(accept.languages()[0].subtags.size(), 2);
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[0], "de");
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[1], "at");
  BOOST_CHECK_CLOSE(*accept.languages()[0].q, 1.0, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[1].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[1].subtags[0], "de");
  BOOST_CHECK_CLOSE(*accept.languages()[1].q, 0.9, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[2].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[2].subtags[0], "en");
  BOOST_CHECK_CLOSE(*accept.languages()[2].q, 0.7, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[3].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[3].subtags[0], "es");
  BOOST_CHECK_CLOSE(*accept.languages()[3].q, 0.1, epsilon);
}

BOOST_AUTO_TEST_CASE(accept_language_6) {
  accept_language accept("de-AT; q=1.0");
  float epsilon = 0.00001;

  BOOST_REQUIRE_EQUAL(accept.languages().size(), 1);
  BOOST_CHECK(std::all_of(accept.languages().begin(), accept.languages().end(),
                          [](accept_language::entry const &language) -> bool {
                            return language.q;
                          }));
  BOOST_REQUIRE_EQUAL(accept.languages()[0].subtags.size(), 2);
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[0], "de");
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[1], "AT");
  BOOST_CHECK_CLOSE(*accept.languages()[0].q, 1.0, epsilon);
}

BOOST_AUTO_TEST_CASE(accept_language_7) {
  accept_language accept("de-at;q=1.0, de;q=0.9, en;q=0.7, es;q=0.1, fr, en-GB");
  float epsilon = 0.00001;

  BOOST_REQUIRE_EQUAL(accept.languages().size(), 6);
  BOOST_CHECK(std::any_of(accept.languages().begin(), accept.languages().end(),
                          [](accept_language::entry const &language) -> bool {
                            return language.q;
                          }));
  BOOST_REQUIRE_EQUAL(accept.languages()[0].subtags.size(), 2);
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[0], "de");
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[1], "at");
  BOOST_CHECK_CLOSE(*accept.languages()[0].q, 1.0, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[1].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[1].subtags[0], "de");
  BOOST_CHECK_CLOSE(*accept.languages()[1].q, 0.9, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[2].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[2].subtags[0], "en");
  BOOST_CHECK_CLOSE(*accept.languages()[2].q, 0.7, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[3].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[3].subtags[0], "es");
  BOOST_CHECK_CLOSE(*accept.languages()[3].q, 0.1, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[4].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[4].subtags[0], "fr");
  BOOST_CHECK(not accept.languages()[4].q);
  BOOST_REQUIRE_EQUAL(accept.languages()[5].subtags.size(), 2);
  BOOST_CHECK_EQUAL(accept.languages()[5].subtags[0], "en");
  BOOST_CHECK_EQUAL(accept.languages()[5].subtags[1], "GB");
  BOOST_CHECK(not accept.languages()[4].q);
  accept.normalize();
  BOOST_REQUIRE_EQUAL(accept.languages()[0].subtags.size(), 2);
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[0], "de");
  BOOST_CHECK_EQUAL(accept.languages()[0].subtags[1], "at");
  BOOST_CHECK_CLOSE(*accept.languages()[0].q, 1.0, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[1].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[1].subtags[0], "fr");
  BOOST_CHECK(not accept.languages()[1].q);
  BOOST_REQUIRE_EQUAL(accept.languages()[2].subtags.size(), 2);
  BOOST_CHECK_EQUAL(accept.languages()[2].subtags[0], "en");
  BOOST_CHECK_EQUAL(accept.languages()[2].subtags[1], "GB");
  BOOST_CHECK(not accept.languages()[2].q);
  BOOST_REQUIRE_EQUAL(accept.languages()[3].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[3].subtags[0], "de");
  BOOST_CHECK_CLOSE(*accept.languages()[3].q, 0.9, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[4].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[4].subtags[0], "en");
  BOOST_CHECK_CLOSE(*accept.languages()[4].q, 0.7, epsilon);
  BOOST_REQUIRE_EQUAL(accept.languages()[5].subtags.size(), 1);
  BOOST_CHECK_EQUAL(accept.languages()[5].subtags[0], "es");
  BOOST_CHECK_CLOSE(*accept.languages()[5].q, 0.1, epsilon);
}

