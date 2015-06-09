/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015 Mattia Basaglia
 * \section License
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define BOOST_TEST_MODULE Test_Formatter

#include <boost/test/unit_test.hpp>

#include "string/json.hpp"

using namespace string;

BOOST_AUTO_TEST_CASE( test_array )
{
    JsonParser parser;
    parser.throws(false);
    PropertyTree tree;

    tree = parser.parse_string("[1, 2, 3]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "1" );
    BOOST_CHECK( tree.get<std::string>("1") == "2" );
    BOOST_CHECK( tree.get<std::string>("2") == "3" );

    // not valid json, but it shouldn't have issues
    tree = parser.parse_string("[4, 5, 6,]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "4" );
    BOOST_CHECK( tree.get<std::string>("1") == "5" );
    BOOST_CHECK( tree.get<std::string>("2") == "6" );

    // errors leave the parser in a consistent state
    tree = parser.parse_string("[7, 8, 9");
    BOOST_CHECK( parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "7" );
    BOOST_CHECK( tree.get<std::string>("1") == "8" );
    BOOST_CHECK( tree.get<std::string>("2") == "9" );

    tree = parser.parse_string("[]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( !tree.get_optional<std::string>("0") );

    tree = parser.parse_string("[[0,1],[2]]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0.0") == "0" );
    BOOST_CHECK( tree.get<std::string>("0.1") == "1" );
    BOOST_CHECK( tree.get<std::string>("1.0") == "2" );

}

BOOST_AUTO_TEST_CASE( test_object )
{
    JsonParser parser;
    parser.throws(false);
    PropertyTree tree;

    tree = parser.parse_string("{}");
    BOOST_CHECK( !parser.error() );

    tree = parser.parse_string(R"({foo: "bar"})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo") == "bar" );

    tree = parser.parse_string(R"({"foo": "bar"})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo") == "bar" );

    tree = parser.parse_string(R"({foo: "bar", hello: "world"})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo") == "bar" );
    BOOST_CHECK( tree.get<std::string>("hello") == "world" );

    tree = parser.parse_string(R"({foo: "bar", hello: "world",})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo") == "bar" );
    BOOST_CHECK( tree.get<std::string>("hello") == "world" );

    tree = parser.parse_string(R"({foo: {hello: "world"}})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo.hello") == "world" );

    tree = parser.parse_string(R"({foo: {hello: "bar")");
    BOOST_CHECK( parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo.hello") == "bar" );
}

BOOST_AUTO_TEST_CASE( test_values )
{
    JsonParser parser;
    parser.throws(false);
    PropertyTree tree;

    tree = parser.parse_string("[123]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<int>("0") == 123 );

    tree = parser.parse_string("[12.3]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( int(tree.get<double>("0")*10) == 123 );

    tree = parser.parse_string("[true, false]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<bool>("0") == true );
    BOOST_CHECK( tree.get<bool>("1") == false );

    tree = parser.parse_string("[null]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( !tree.get_optional<std::string>("0") );

    tree = parser.parse_string("[foo]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "foo" );
}

BOOST_AUTO_TEST_CASE( test_string )
{
    JsonParser parser;
    parser.throws(false);
    PropertyTree tree;

    tree = parser.parse_string(R"(["123"])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"(["12\"3"])"); // "
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "12\"3" );

    tree = parser.parse_string(R"(["\b\f\r\t\n\\\"\/"])"); // "
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "\b\f\r\t\n\\\"/" );

    tree = parser.parse_string(R"(["\u0020"])"); // "
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == " " );

    tree = parser.parse_string(R"(["\u00E6"])"); // "
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "æ" );
}

BOOST_AUTO_TEST_CASE( test_comments )
{
    JsonParser parser;
    parser.throws(false);
    PropertyTree tree;

    tree = parser.parse_string(R"([   123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"( [
        "123"])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"([// hello
        123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"([/*hello*/123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"([ /*hello
    world*/
    123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"([/**/123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );
}

