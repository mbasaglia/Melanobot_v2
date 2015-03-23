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
#define BOOST_TEST_MODULE Test_String

#include <boost/test/unit_test.hpp>

#include "string/string_functions.hpp"
#include "string/trie.hpp"

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

    BOOST_CHECK( string::replace(foxy,{{"fox","dog"}, {"dog","fox"}}) == "the quick brown dog jumps over the lazy fox" );
    std::string template_string = "%animol the quick brown %animal_2 %action over the lazy %animal_";
    Properties replace{{"animal","dog"},{"action","jumps"},{"animal_2","fox"}};
    BOOST_CHECK( string::replace(template_string,replace,"%") == "%animol "+foxy+'_' );

}

