/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#define BOOST_TEST_MODULE Test_String

#include <boost/test/unit_test.hpp>

#include <unordered_map>

#include "melanolib/string/stringutils.hpp"
#include "melanolib/string/trie.hpp"
#include "melanolib/string/language.hpp"
#include "melanolib/string/quickstream.hpp"

using namespace melanolib;

BOOST_AUTO_TEST_CASE( test_trie_insert )
{
    string::Trie trie;
    BOOST_CHECK ( !trie.contains_prefix("hello") );

    trie.insert("hello");
    BOOST_CHECK ( trie.contains_prefix("hello") );
    BOOST_CHECK ( trie.contains_prefix("hell") );
    BOOST_CHECK ( trie.contains("hello") );
    BOOST_CHECK ( !trie.contains("hell") );

    trie.insert("hell");
    BOOST_CHECK ( trie.contains_prefix("hello") );
    BOOST_CHECK ( trie.contains_prefix("hell") );
    BOOST_CHECK ( trie.contains("hello") );
    BOOST_CHECK ( trie.contains("hell") );
}

BOOST_AUTO_TEST_CASE( test_trie_erase )
{
    string::Trie trie;

    trie.insert("hello");
    trie.insert("he");
    BOOST_CHECK ( trie.contains_prefix("hello") );
    BOOST_CHECK ( trie.contains_prefix("hell") );

    trie.erase("hello");
    BOOST_CHECK ( !trie.contains_prefix("hel") );
    BOOST_CHECK ( trie.contains("he") );
}

BOOST_AUTO_TEST_CASE( test_trie_initializer )
{
    string::Trie trie{"pony","princess"};

    BOOST_CHECK ( trie.contains_prefix("prince") );
    BOOST_CHECK ( !trie.contains("prince") );
    BOOST_CHECK ( trie.contains("pony") );
}

BOOST_AUTO_TEST_CASE( test_trie_prepend )
{
    string::Trie trie{"pony","princess"};

    trie.prepend(' ');
    BOOST_CHECK ( trie.contains_prefix(" prince") );
    BOOST_CHECK ( trie.contains(" pony") );
    BOOST_CHECK ( !trie.contains("pony") );
    BOOST_CHECK ( !trie.contains_prefix("prince") );

    trie.prepend("little");
    BOOST_CHECK ( !trie.contains(" pony") );
    BOOST_CHECK ( trie.contains("little pony") );

    trie.prepend("");
    BOOST_CHECK ( trie.contains("little pony") );
}

BOOST_AUTO_TEST_CASE( test_trie_data )
{
    string::Trie trie{"pony","princess"};
    trie.root().data();
    BOOST_CHECK ( std::is_void<decltype(trie.root().data())>::value );

    string::BasicTrie<std::string> string_trie;
    string_trie.insert("pony","little");
    BOOST_CHECK ( string_trie.find("pony").data() == "little" );

    auto made_trie_assoc = string::make_trie(std::unordered_map<std::string,int>{ {"foo",5}, {"bar",6} });
    BOOST_CHECK ( made_trie_assoc.find("foo").data() == 5 );

    auto made_trie = string::make_trie(std::vector<std::string>{"foo","bar"});
    BOOST_CHECK ( made_trie.contains("foo") );
}

BOOST_AUTO_TEST_CASE( test_trie_iterator )
{
    string::Trie trie{"pretty","pony","princess","priceless"};
    auto iter = trie.root();
    BOOST_CHECK ( iter.root() );
    BOOST_CHECK ( iter.can_move_down('p') );
    BOOST_CHECK ( !iter.can_move_down('q') );
    BOOST_CHECK ( iter.valid() );
    BOOST_CHECK ( iter.depth() == 0 );
    iter.move_down('p');
    iter.move_down('r');
    BOOST_CHECK ( iter.can_move_down('e') );
    BOOST_CHECK ( iter.can_move_down('i') );
    BOOST_CHECK ( !iter.can_move_down('o') );
    BOOST_CHECK ( iter.valid() );
    BOOST_CHECK ( !iter.root() );
    BOOST_CHECK ( iter.depth() == 2 );
    iter.move_up();
    BOOST_CHECK ( iter.depth() == 1 );
    iter.move_down('o');
    BOOST_CHECK ( iter.can_move_down('n') );
    BOOST_CHECK ( iter.valid() );
    BOOST_CHECK ( !iter.root() );
    BOOST_CHECK ( iter.depth() == 2 );
    iter.move_down('n');
    iter.move_down('y');
    BOOST_CHECK ( !iter.can_move_down('.') );
    BOOST_CHECK ( iter.valid() );
    BOOST_CHECK ( !iter.root() );
    BOOST_CHECK ( iter.depth() == 4 );
    iter.move_down('.');
    BOOST_CHECK ( !iter.valid() );
    BOOST_CHECK ( iter.depth() == 0 );

    BOOST_CHECK ( string::StringTrie::iterator().data() == "" );

}

BOOST_AUTO_TEST_CASE( test_implode )
{
    using Container = std::vector<std::string>;
    BOOST_CHECK ( string::implode(" ",Container{"hello","world"}) == "hello world" );
    BOOST_CHECK ( string::implode(" ",Container{"hello"}) == "hello" );
    BOOST_CHECK ( string::implode(" ",Container{}).empty() );
}

BOOST_AUTO_TEST_CASE( test_starts_with )
{
    BOOST_CHECK ( string::starts_with("princess","prince") );
    BOOST_CHECK ( !string::starts_with("prince","princess") );
    BOOST_CHECK ( string::starts_with("pony","") );
    BOOST_CHECK ( !string::starts_with("pony","my") );
    BOOST_CHECK ( string::starts_with("racecar","racecar") );
}

BOOST_AUTO_TEST_CASE( test_ends_with )
{
    BOOST_CHECK ( string::ends_with("princess","cess") );
    BOOST_CHECK ( !string::ends_with("cess","princess") );
    BOOST_CHECK ( string::ends_with("pony","") );
    BOOST_CHECK ( !string::ends_with("pony","my") );
    BOOST_CHECK ( string::ends_with("racecar","racecar") );
}

BOOST_AUTO_TEST_CASE( test_strtolower_strtoupper )
{
    BOOST_CHECK ( string::strtolower("pony") == "pony" );
    BOOST_CHECK ( string::strtolower("Pony") == "pony" );
    BOOST_CHECK ( string::strtolower("[PONY]") == "[pony]" );

    BOOST_CHECK ( string::strtoupper("PONY") == "PONY" );
    BOOST_CHECK ( string::strtoupper("Pony") == "PONY" );
    BOOST_CHECK ( string::strtoupper("[pony]") == "[PONY]" );
}

BOOST_AUTO_TEST_CASE( test_elide )
{
    std::string long_text = "Lorem ipsum dolor \n   sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
    BOOST_CHECK ( string::elide(long_text,long_text.size()) == long_text );
    BOOST_CHECK ( string::elide(long_text,3) == "..." );
    BOOST_CHECK ( string::elide(long_text,11+3) == "Lorem ipsum..." );
    BOOST_CHECK ( string::elide(long_text,12+3) == "Lorem ipsum..." );
    BOOST_CHECK ( string::elide(long_text,14+3) == "Lorem ipsum..." );
    BOOST_CHECK ( string::elide(long_text,17+3) == "Lorem ipsum dolor..." );
    BOOST_CHECK ( string::elide(long_text,20+3) == "Lorem ipsum dolor..." );
}

BOOST_AUTO_TEST_CASE( test_misc )
{
    BOOST_CHECK(string::collapse_spaces("Hello  world\n\t  !") == "Hello world !");
    BOOST_CHECK(string::collapse_spaces("Hello world!") == "Hello world!");
    BOOST_CHECK(string::add_slashes("Hello world!","wo!") == R"(Hell\o \w\orld\!)");
    BOOST_CHECK(string::add_slashes("Hello world!","") == "Hello world!");
    BOOST_CHECK(string::regex_escape("^([a-z]+)[0-9]?$") == R"(\^\(\[a-z\]\+\)\[0-9\]\?\$)");
}

BOOST_AUTO_TEST_CASE( test_replace )
{
    std::string foxy = "the quick brown fox jumps over the lazy dog";
    BOOST_CHECK( string::replace(foxy,"","foo") == foxy );
    BOOST_CHECK( string::replace(foxy,"the","a") == "a quick brown fox jumps over a lazy dog" );
    BOOST_CHECK( string::replace(foxy," ","") == "thequickbrownfoxjumpsoverthelazydog" );

    BOOST_CHECK( string::replace(foxy, {{"fox","dog"}, {"dog","fox"}}, "") == "the quick brown dog jumps over the lazy fox" );
    std::string template_string = "%animol the quick brown %animal_2 %action over the lazy %animal_";
    std::unordered_map<std::string, std::string> replace{{"animal","dog"},{"action","jumps"},{"animal_2","fox"}};
    BOOST_CHECK( string::replace(template_string,replace,"%") == "%animol "+foxy+'_' );
    template_string += "%anim";
    BOOST_CHECK( string::replace(template_string,replace,"%") == "%animol "+foxy+"_%anim" );

    BOOST_CHECK( string::replace(foxy,std::unordered_map<std::string,std::string>()) == foxy );

}

BOOST_AUTO_TEST_CASE( test_wildcard )
{
    BOOST_CHECK( !string::simple_wildcard("foobar","fu*") );
    BOOST_CHECK( string::simple_wildcard("foobar","foobar") );
    BOOST_CHECK( string::simple_wildcard("foobar","foo*") );
    BOOST_CHECK( string::simple_wildcard("foobar","*") );
    BOOST_CHECK( string::simple_wildcard("foobar","*bar") );
    BOOST_CHECK( string::simple_wildcard("foobar","f*r") );
    BOOST_CHECK( !string::simple_wildcard("foo*","foobar") );

    std::vector<std::string> c;
    BOOST_CHECK( !string::simple_wildcard(c,"m*y") );
    c.push_back("pony");
    BOOST_CHECK( !string::simple_wildcard(c,"m*y") );
    c.push_back("money");
    BOOST_CHECK( string::simple_wildcard(c,"m*y") );
}

BOOST_AUTO_TEST_CASE( test_split )
{
    using v = std::vector<std::string>;
    v hw {"hello","world"};
    v h_w {"hello","","world"};
    v hwb {"hello","world!"};

    BOOST_CHECK( string::regex_split("hello, world!","[, !]") == hw );
    BOOST_CHECK( string::regex_split("hello, world!",std::regex("[, !]")) == hw );
    BOOST_CHECK( string::regex_split("hello, world!","[, !]",false) == h_w );

    BOOST_CHECK( string::comma_split("hello, world!") == hwb );
    BOOST_CHECK( string::comma_split("hello,,  world",false) == h_w );
    BOOST_CHECK( string::comma_split("") == v{} );

    BOOST_CHECK( string::char_split("hello:world",':') == hw );
    BOOST_CHECK( string::char_split("hello::world",':') == hw );
    BOOST_CHECK( string::char_split("hello::world",':',false) == h_w );
    BOOST_CHECK( string::char_split("",':') == v{} );
    BOOST_CHECK( string::char_split("foo:",':') == v{"foo"} );
    BOOST_CHECK( string::char_split(":foo",':') == v{"foo"} );
}

BOOST_AUTO_TEST_CASE( test_similarity )
{
    BOOST_CHECK( string::similarity("foo","bar") == 0 );
    BOOST_CHECK( string::similarity("hello","hello") != 0 );
    BOOST_CHECK( string::similarity("hello","he") != 0 );
    BOOST_CHECK( string::similarity("hello","hello") > string::similarity("hello","he") );
    BOOST_CHECK( string::similarity("princess","priceless") == string::similarity("priceless","princess") );
    BOOST_CHECK( string::similarity("foo","foobar") > string::similarity("foo","fboaor") );
}

BOOST_AUTO_TEST_CASE( test_to_uint )
{
    BOOST_CHECK( string::to_uint("f00bar") == 0 );
    BOOST_CHECK( string::to_uint("f00bar",16) == 0xF00BA );
    BOOST_CHECK( string::to_uint("f00bar",10,123) == 123 );
    BOOST_CHECK( string::to_uint("10",8) == 8 );
    BOOST_CHECK( string::to_uint("10",9) == 9 );
    BOOST_CHECK( string::to_uint("10",10) == 10 );
    BOOST_CHECK( string::to_uint("10",11) == 11 );
    BOOST_CHECK( string::to_uint("10",12) == 12 );
}

BOOST_AUTO_TEST_CASE( test_icase_equal )
{
    BOOST_CHECK( string::icase_equal("foo","foo") );
    BOOST_CHECK( string::icase_equal("foo","FOO") );
    BOOST_CHECK( !string::icase_equal("foo","fo") );
    BOOST_CHECK( string::icase_equal("foo_-'","FOO_-'") );
    BOOST_CHECK( string::icase_equal("","") );
    BOOST_CHECK( !string::icase_equal("foo","") );
}

BOOST_AUTO_TEST_CASE( test_to_string )
{
    BOOST_CHECK( string::to_string(1) == "1" );
    BOOST_CHECK( string::to_string(1,2) == "01" );
    BOOST_CHECK( string::to_string(1,3) == "001" );
    BOOST_CHECK( string::to_string(1,-1) == "1" );
    BOOST_CHECK( string::to_string(1l,2) == "01" );
    BOOST_CHECK( string::to_string(1u) == "1" );
}

BOOST_AUTO_TEST_CASE( test_is_one_of )
{
    BOOST_CHECK( string::is_one_of("foo",{"foo","bar"}) );
    BOOST_CHECK( !string::is_one_of("foo",{"fu","bar"}) );
    BOOST_CHECK( !string::is_one_of("",{"foo","bar"}) );
    BOOST_CHECK( !string::is_one_of("foo",{}) );
}

BOOST_AUTO_TEST_CASE( test_English )
{
    using string::english;

    // genitive
    BOOST_CHECK( english.genitive("Melano") == "Melano's" );
    BOOST_CHECK( english.genitive("Melanosuchus") == "Melanosuchus'" );

    // imperate
    BOOST_CHECK( english.imperate("") == "" );
    BOOST_CHECK( english.imperate("can") == "can" );
    BOOST_CHECK( english.imperate("be") == "is" );
    BOOST_CHECK( english.imperate("don't") == "doesn't" );

    BOOST_CHECK( english.imperate("try") == "tries" );
    BOOST_CHECK( english.imperate("say") == "says" );

    BOOST_CHECK( english.imperate("go") == "goes" );
    BOOST_CHECK( english.imperate("push") == "pushes" );
    BOOST_CHECK( english.imperate("sit") == "sits" );

    // ordinal_suffix
    BOOST_CHECK( english.ordinal_suffix(0) == "" );

    BOOST_CHECK( english.ordinal_suffix(1) == "st" );
    BOOST_CHECK( english.ordinal_suffix(21) == "st" );
    BOOST_CHECK( english.ordinal_suffix(121) == "st" );
    BOOST_CHECK( english.ordinal_suffix(11) == "th" );
    BOOST_CHECK( english.ordinal_suffix(111) == "th" );

    BOOST_CHECK( english.ordinal_suffix(2) == "nd" );
    BOOST_CHECK( english.ordinal_suffix(22) == "nd" );
    BOOST_CHECK( english.ordinal_suffix(122) == "nd" );
    BOOST_CHECK( english.ordinal_suffix(12) == "th" );
    BOOST_CHECK( english.ordinal_suffix(112) == "th" );

    BOOST_CHECK( english.ordinal_suffix(3) == "rd" );
    BOOST_CHECK( english.ordinal_suffix(23) == "rd" );
    BOOST_CHECK( english.ordinal_suffix(123) == "rd" );
    BOOST_CHECK( english.ordinal_suffix(13) == "th" );
    BOOST_CHECK( english.ordinal_suffix(113) == "th" );

    BOOST_CHECK( english.ordinal_suffix(5) == "th" );

    // pronoun_to3rd
    std::string you = "Melanobot";
    std::string me = "Melanosuchus";
    BOOST_CHECK( english.pronoun_to3rd("I'm here",me,you) == "Melanosuchus is here" );
    BOOST_CHECK( english.pronoun_to3rd("are you here?",me,you) == "is Melanobot here?" );
    BOOST_CHECK( english.pronoun_to3rd("my bot",me,you) == "Melanosuchus' bot" );

}

BOOST_AUTO_TEST_CASE( test_Inflector )
{
    string::Inflector infl = {
        {std::regex("foo"), "bar"}
    };
    BOOST_CHECK( infl.inflect_all("foobar") == "barbar" );
    BOOST_CHECK( infl.inflect_all("fubar") == "fubar" );
    BOOST_CHECK( infl.inflect_all("foobarfooo") == "barbarbaro" );

    string::Inflector infl2 ({
        {"foo", "bar"}
    }, true);

    BOOST_CHECK( infl2.inflect_all("foo bar") == "bar bar" );
    BOOST_CHECK( infl2.inflect_all("fu bar") == "fu bar" );
    BOOST_CHECK( infl2.inflect_all("foobarfooo") == "foobarfooo" );
}

BOOST_AUTO_TEST_CASE( test_QuickStream )
{
    using string::QuickStream;

    // eof
    BOOST_CHECK( QuickStream().eof() );
    QuickStream qs("foo");
    BOOST_CHECK( !qs.eof() );
    BOOST_CHECK( qs );
    qs.ignore(3);
    BOOST_CHECK( qs.eof() );
    BOOST_CHECK( qs );
    qs.ignore();
    BOOST_CHECK( !qs );
    qs.clear();
    BOOST_CHECK( qs );
    qs.unget();
    BOOST_CHECK( !qs.eof() );

    // str
    BOOST_CHECK( qs.str() == "foo" );
    qs.str("");
    qs.ignore();
    BOOST_CHECK( qs.eof() );
    qs.str("bar");
    BOOST_CHECK( !qs.eof() );
    BOOST_CHECK( qs.str() == "bar" );

    // next
    BOOST_CHECK( qs.next() == 'b' );
    BOOST_CHECK( qs.next() == 'a' );
    BOOST_CHECK( qs.next() == 'r' );
    BOOST_CHECK( qs.eof() );
    BOOST_CHECK( qs.next() == std::char_traits<char>::eof() );
    BOOST_CHECK( qs.eof() );

    // unget/peek
    qs.unget();
    BOOST_CHECK( qs.eof() );
    BOOST_CHECK( qs.peek() == std::char_traits<char>::eof() );
    qs.unget();
    BOOST_CHECK( qs.peek() == 'r' );
    qs.unget();
    BOOST_CHECK( qs.peek() == 'a' );
    qs.unget();
    BOOST_CHECK( qs.peek() == 'b' );
    qs.unget();
    BOOST_CHECK( qs.peek() == 'b' );

    // ignore
    qs.str("The quick brown fox jumps over the lazy dog");
    qs.ignore();
    BOOST_CHECK( qs.peek() == 'h' );
    qs.ignore(5);
    BOOST_CHECK( qs.peek() == 'i' );
    qs.ignore(10,' ');
    BOOST_CHECK( qs.peek() == 'b' );
    qs.ignore(10,'.');
    BOOST_CHECK( qs.peek() == 'j' );

    // get_line
    BOOST_CHECK( qs.get_line(' ') == "jumps" );
    BOOST_CHECK( qs.get_line() == "over the lazy dog" );
    BOOST_CHECK( qs.eof() );

    // get_int
    qs.str("123foo");
    BOOST_CHECK( qs.get_int() == 123 );
    BOOST_CHECK( qs.peek() == 'f' );
    BOOST_CHECK( qs.get_int() == 0 );
    BOOST_CHECK( qs.peek() == 'f' );
    qs.set_pos(0);
    int i = 0;
    BOOST_CHECK( qs.get_int(i) );
    BOOST_CHECK( i == 123 );
    BOOST_CHECK( !qs.get_int(i) );
    BOOST_CHECK( i == 123 );

    // tell_pos/set_pos
    BOOST_CHECK( qs.tell_pos() == 3 );
    qs.set_pos(1);
    BOOST_CHECK( qs.tell_pos() == 1 );
    BOOST_CHECK( qs.peek() == '2' );

    // regex
    qs.set_pos(0);
    std::regex re("[0-9]+");
    BOOST_CHECK( qs.regex_match(re) );
    BOOST_CHECK( qs.get_regex(re) == "123" );
    BOOST_CHECK( qs.peek() == 'f' );
    BOOST_CHECK( !qs.regex_match(re) );
    BOOST_CHECK( qs.get_regex(re) == "" );
    qs.ignore(10);
    BOOST_CHECK( !qs.regex_match(re) );
    BOOST_CHECK( qs.get_regex(re) == "" );
    std::smatch match;
    BOOST_CHECK( !qs.regex_match(re,match) );
    qs.set_pos(0);
    std::regex re1("([0-9]+)([a-z]+)");
    BOOST_CHECK( qs.get_regex(re1,match) );
    BOOST_CHECK( match[1] == "123" );
    BOOST_CHECK( match[2] == "foo" );
    BOOST_CHECK( qs.eof() );
}

