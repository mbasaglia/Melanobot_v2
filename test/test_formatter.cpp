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
#define BOOST_TEST_MODULE Test_Formatter

#include <boost/test/unit_test.hpp>

#include <map>

#include "string/string.hpp"
#include "string/logger.hpp"
#include "xonotic/formatter.hpp"
#include "irc/irc_formatter.hpp"
#include "fun/rainbow.hpp"
#include "string/encoding.hpp"
#include "string/replacements.hpp"

using namespace string;

BOOST_AUTO_TEST_CASE( test_registry )
{
    Logger::instance().set_log_verbosity("sys",-1);
    Formatter::registry();
    BOOST_CHECK( Formatter::formatter("config")->name() == "config" );
    BOOST_CHECK( Formatter::formatter("utf8")->name() == "utf8" );
    BOOST_CHECK( Formatter::formatter("ascii")->name() == "ascii" );
    BOOST_CHECK( Formatter::formatter("ansi-ascii")->name() == "ansi-ascii" );
    BOOST_CHECK( Formatter::formatter("ansi-utf8")->name() == "ansi-utf8" );
    BOOST_CHECK( Formatter::formatter("foobar")->name() == "utf8" );
}

template<class T>
    const T* cast(FormattedString::const_reference item)
{
    if ( item.has_type<T>() )
        return &item.reference<T>();
    return nullptr;
}

BOOST_AUTO_TEST_CASE( test_utf8 )
{
    FormatterUtf8 fmt;
    std::string utf8 = u8"Foo bar è$ç";
    auto decoded = fmt.decode(utf8);
    BOOST_CHECK( decoded.size() == 4 );
    BOOST_CHECK( cast<string::AsciiString>(decoded[0]));
    BOOST_CHECK( cast<Unicode>(decoded[1]));
    BOOST_CHECK( cast<string::AsciiString>(decoded[2]));
    BOOST_CHECK( cast<Unicode>(decoded[3]));
    BOOST_CHECK( decoded.encode(fmt) == utf8 );
    auto invalid_utf8 = utf8;
    invalid_utf8.push_back(uint8_t(0b1110'0000)); // '));
    BOOST_CHECK( fmt.decode(invalid_utf8).encode(fmt) == utf8 );

    BOOST_CHECK( decoded.encode(*Formatter::formatter(fmt.name())) == utf8 );

    BOOST_CHECK( fmt.to_string("hello world") == "hello world" );
    BOOST_CHECK( fmt.to_string('x') == "x" );
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)) == "ç" );
    BOOST_CHECK( fmt.to_string(color::red).empty() );
    BOOST_CHECK( fmt.to_string(FormatFlags::BOLD).empty() );
    BOOST_CHECK( fmt.to_string(ClearFormatting()).empty() );
}

BOOST_AUTO_TEST_CASE( test_ascii )
{
    string::FormatterAscii fmt;
    std::string utf8 = u8"Foo bar è$ç";
#ifdef HAS_ICONV
    BOOST_CHECK( string::FormatterUtf8().decode(utf8).encode(fmt) == "Foo bar e$c" );
#else
    BOOST_CHECK( string::FormatterUtf8().decode(utf8).encode(fmt) == "Foo bar ?$?" );
#endif

    BOOST_CHECK( fmt.decode("foobarè").size() == 1 );

    BOOST_CHECK( fmt.to_string("hello world") == "hello world" );
    BOOST_CHECK( fmt.to_string('x') == "x" );
#ifdef HAS_ICONV
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)) == "c" );
#else
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)).empty() );
#endif
    BOOST_CHECK( fmt.to_string(color::red).empty() );
    BOOST_CHECK( fmt.to_string(FormatFlags::BOLD).empty() );
    BOOST_CHECK( fmt.to_string(ClearFormatting()).empty() );
}

BOOST_AUTO_TEST_CASE( test_config )
{
    FormatterConfig fmt;
    std::string utf8 = u8"Foo bar è#ç";
    auto decoded = fmt.decode(utf8);
    BOOST_CHECK( decoded.size() == 4 );
    BOOST_CHECK( cast<string::AsciiString>(decoded[0]));
    BOOST_CHECK( cast<Unicode>(decoded[1]));
    BOOST_CHECK( cast<string::AsciiString>(decoded[2]));
    BOOST_CHECK( cast<Unicode>(decoded[3]));
    BOOST_CHECK( decoded.encode(fmt) == utf8 );

    std::string formatted = "Hello $(1)World $(-bu)test$(-)#1$(green)green$(x00f)blue§$$(1)";
    decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 13 );
    // "Hello "
    BOOST_CHECK( cast<string::AsciiString>(decoded[0]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[0]) == "Hello " );
    // red
    BOOST_CHECK( cast<color::Color12>(decoded[1]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[1]) == color::red );
    // "World "
    BOOST_CHECK( cast<string::AsciiString>(decoded[2]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[2]) == "World " );
    // bold+underline
    BOOST_CHECK( cast<FormatFlags>(decoded[3]) );
    BOOST_CHECK( *cast<FormatFlags>(decoded[3]) == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<string::AsciiString>(decoded[4]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[4]) == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<string::AsciiString>(decoded[6]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[6]) == "#1" );
    // green
    BOOST_CHECK( cast<color::Color12>(decoded[7]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[7]) == color::green );
    // "green"
    BOOST_CHECK( cast<string::AsciiString>(decoded[8]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[8]) == "green" );
    // blue
    BOOST_CHECK( cast<color::Color12>(decoded[9]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[9]) == color::blue );
    // "blue"
    BOOST_CHECK( cast<string::AsciiString>(decoded[10]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[10]) == "blue" );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[11]) );
    BOOST_CHECK( cast<Unicode>(decoded[11])->utf8() == u8"§" );
    BOOST_CHECK( cast<Unicode>(decoded[11])->point() == 0x00A7 );
    // "$(1)"
    BOOST_CHECK( cast<string::AsciiString>(decoded[12]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[12]) == "$(1)" );


    BOOST_CHECK( decoded.encode(fmt) == "Hello $(1)World $(-bu)test$(-)#1$(2)green$(4)blue§$$(1)" );

    BOOST_CHECK( fmt.to_string("hello world$") == "hello world$$" );
    BOOST_CHECK( fmt.to_string('x') == "x" );
    BOOST_CHECK( fmt.to_string('$') == "$$" );
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)) == "ç" );
    BOOST_CHECK( fmt.to_string(color::red) == "$(1)" );
    BOOST_CHECK( fmt.to_string(color::nocolor) == "$(nocolor)" );
    BOOST_CHECK( fmt.to_string(FormatFlags::BOLD) == "$(-b)" );
    BOOST_CHECK( fmt.to_string(ClearFormatting()) == "$(-)" );
}

BOOST_AUTO_TEST_CASE( test_ansi_ascii )
{
    FormatterAnsi fmt(false);
    std::string utf8 = u8"Foo bar è$ç";
#ifdef HAS_ICONV
    BOOST_CHECK( string::FormatterUtf8().decode(utf8).encode(fmt) == "Foo bar e$c" );
#else
    BOOST_CHECK( string::FormatterUtf8().decode(utf8).encode(fmt) == "Foo bar ?$?" );
#endif

    std::string formatted = "Hello \x1b[31mWorld \x1b[1;4;41mtest\x1b[0m#1\x1b[92mgreen\x1b[1;34mblue\x1b[39m$";
    auto decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 13 );
    // "Hello "
    BOOST_CHECK( cast<string::AsciiString>(decoded[0]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[0]) == "Hello " );
    // red
    BOOST_CHECK( cast<color::Color12>(decoded[1]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[1]) == color::dark_red );
    // "World "
    BOOST_CHECK( cast<string::AsciiString>(decoded[2]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[2]) == "World " );
    // bold+underline
    BOOST_CHECK( cast<FormatFlags>(decoded[3]) );
    BOOST_CHECK( *cast<FormatFlags>(decoded[3]) == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<string::AsciiString>(decoded[4]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[4]) == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<string::AsciiString>(decoded[6]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[6]) == "#1" );
    // green
    BOOST_CHECK( cast<color::Color12>(decoded[7]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[7]) == color::green );
    // "green"
    BOOST_CHECK( cast<string::AsciiString>(decoded[8]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[8]) == "green" );
    // blue
    BOOST_CHECK( cast<color::Color12>(decoded[9]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[9]) == color::blue );
    // "blue"
    BOOST_CHECK( cast<string::AsciiString>(decoded[10]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[10]) == "blue" );
    // nocolor
    BOOST_CHECK( cast<color::Color12>(decoded[11]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[11]) == color::nocolor );
    // "$"
    BOOST_CHECK( cast<string::AsciiString>(decoded[12]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[12]) == "$" );

    BOOST_CHECK( decoded.encode(fmt) == "Hello \x1b[31mWorld \x1b[1;4;23mtest\x1b[0m#1\x1b[92mgreen\x1b[94mblue\x1b[39m$" );

    BOOST_CHECK( fmt.to_string("hello world") == "hello world" );
    BOOST_CHECK( fmt.to_string('x') == "x" );
#ifdef HAS_ICONV
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)) == "c" );
#else
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)).empty() );
#endif
    BOOST_CHECK( fmt.to_string(color::dark_red) == "\x1b[31m" );
    BOOST_CHECK( fmt.to_string(color::nocolor) == "\x1b[39m" );
    auto bold = fmt.to_string(FormatFlags::BOLD);
    BOOST_CHECK( std::regex_match(bold, std::regex(R"(\x1b\[.*(\b1;.*m$)|(\b1m$))")) );
    BOOST_CHECK( fmt.to_string(ClearFormatting()) == "\x1b[0m" );
}

BOOST_AUTO_TEST_CASE( test_ansi_utf8 )
{
    FormatterAnsi fmt(true);

    std::string formatted = "Hello \x1b[31mWorld \x1b[1;4;41mtest\x1b[0m#1\x1b[92mgreen\x1b[1;34mblue\x1b[39m§";
    auto decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 13 );
    // "Hello "
    BOOST_CHECK( cast<string::AsciiString>(decoded[0]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[0]) == "Hello " );
    // red
    BOOST_CHECK( cast<color::Color12>(decoded[1]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[1]) == color::dark_red );
    // "World "
    BOOST_CHECK( cast<string::AsciiString>(decoded[2]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[2]) == "World " );
    // bold+underline
    BOOST_CHECK( cast<FormatFlags>(decoded[3]) );
    BOOST_CHECK( *cast<FormatFlags>(decoded[3]) == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<string::AsciiString>(decoded[4]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[4]) == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<string::AsciiString>(decoded[6]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[6]) == "#1" );
    // green
    BOOST_CHECK( cast<color::Color12>(decoded[7]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[7]) == color::green );
    // "green"
    BOOST_CHECK( cast<string::AsciiString>(decoded[8]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[8]) == "green" );
    // blue
    BOOST_CHECK( cast<color::Color12>(decoded[9]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[9]) == color::blue );
    // "blue"
    BOOST_CHECK( cast<string::AsciiString>(decoded[10]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[10]) == "blue" );
    // nocolor
    BOOST_CHECK( cast<color::Color12>(decoded[11]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[11]) == color::nocolor );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[12]) );
    BOOST_CHECK( cast<Unicode>(decoded[12])->utf8() == u8"§" );

    BOOST_CHECK( decoded.encode(fmt) == "Hello \x1b[31mWorld \x1b[1;4;23mtest\x1b[0m#1\x1b[92mgreen\x1b[94mblue\x1b[39m§" );

    BOOST_CHECK( fmt.to_string("hello world") == "hello world" );
    BOOST_CHECK( fmt.to_string('x') == "x" );
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)) == "ç" );
    BOOST_CHECK( fmt.to_string(color::dark_red) == "\x1b[31m" );
    BOOST_CHECK( fmt.to_string(color::nocolor) == "\x1b[39m" );
    auto bold = fmt.to_string(FormatFlags::BOLD);
    BOOST_CHECK( std::regex_match(bold, std::regex(R"(\x1b\[.*(\b1;.*m$)|(\b1m$))")) );
    BOOST_CHECK( fmt.to_string(ClearFormatting()) == "\x1b[0m" );
}

BOOST_AUTO_TEST_CASE( test_irc )
{
    irc::FormatterIrc fmt;

    std::string formatted = "Hello \x03""04,05World \x02\x1ftest\x0f#1\x03""09green\x03""12blue§\x03";
    auto decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 13 );
    // "Hello "
    BOOST_CHECK( cast<string::AsciiString>(decoded[0]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[0]) == "Hello " );
    // red
    BOOST_CHECK( cast<color::Color12>(decoded[1]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[1]) == color::red );
    // "World "
    BOOST_CHECK( cast<string::AsciiString>(decoded[2]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[2]) == "World " );
    // bold+underline
    BOOST_CHECK( cast<FormatFlags>(decoded[3]) );
    BOOST_CHECK( *cast<FormatFlags>(decoded[3]) == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<string::AsciiString>(decoded[4]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[4]) == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<string::AsciiString>(decoded[6]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[6]) == "#1" );
    // green
    BOOST_CHECK( cast<color::Color12>(decoded[7]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[7]) == color::green );
    // "green"
    BOOST_CHECK( cast<string::AsciiString>(decoded[8]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[8]) == "green" );
    // blue
    BOOST_CHECK( cast<color::Color12>(decoded[9]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[9]) == color::blue );
    // "blue"
    BOOST_CHECK( cast<string::AsciiString>(decoded[10]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[10]) == "blue" );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[11]) );
    BOOST_CHECK( cast<Unicode>(decoded[11])->utf8() == u8"§" );
    BOOST_CHECK( cast<Unicode>(decoded[11])->point() == 0x00A7 );
    // invalid color
    BOOST_CHECK( cast<color::Color12>(decoded[12]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[12]) == color::nocolor );

    BOOST_CHECK( decoded.encode(fmt) == "Hello \x03""04World \x02\x1ftest\x0f#1\x03""09green\x03""12blue§\xf" );

    BOOST_CHECK( fmt.to_string("hello world") == "hello world" );
    BOOST_CHECK( fmt.to_string('x') == "x" );
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)) == "ç" );
    BOOST_CHECK( fmt.to_string(color::red) == "\x03""04" );
    BOOST_CHECK( fmt.to_string(color::nocolor) == "\xf" );
    BOOST_CHECK( fmt.to_string(FormatFlags::BOLD) == "\x02" );
    BOOST_CHECK( fmt.to_string(ClearFormatting()) == "\xf" );
}

BOOST_AUTO_TEST_CASE( test_xonotic )
{
    xonotic::XonoticFormatter fmt;

    std::string formatted = "Hello ^1World ^^^2green^x00fblue^x00§\ue012";
    auto decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 9 );
    // "Hello "
    BOOST_CHECK( cast<string::AsciiString>(decoded[0]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[0]) == "Hello " );
    // red
    BOOST_CHECK( cast<color::Color12>(decoded[1]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[1]) == color::red );
    // "World "
    BOOST_CHECK( cast<string::AsciiString>(decoded[2]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[2]) == "World ^" );
    // green
    BOOST_CHECK( cast<color::Color12>(decoded[3]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[3]) == color::green );
    // "green"
    BOOST_CHECK( cast<string::AsciiString>(decoded[4]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[4]) == "green" );
    // blue
    BOOST_CHECK( cast<color::Color12>(decoded[5]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[5]) == color::blue );
    // "blue"
    BOOST_CHECK( cast<string::AsciiString>(decoded[6]) );
    BOOST_CHECK( *cast<string::AsciiString>(decoded[6]) == "blue^x00" );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[7]) );
    BOOST_CHECK( cast<Unicode>(decoded[7])->utf8() == u8"§" );
    //  smile
    BOOST_CHECK( cast<xonotic::QFont>(decoded[8]) );
    BOOST_CHECK( cast<xonotic::QFont>(decoded[8])->index() == 0x12 );
    BOOST_CHECK( cast<xonotic::QFont>(decoded[8])->alternative() == ":)" );
    BOOST_CHECK( cast<xonotic::QFont>(decoded[8])->unicode_point() == 0xe012 );

    BOOST_CHECK( decoded.encode(fmt) == "Hello ^1World ^^^2green^4blue^^x00§\ue012" );

    BOOST_CHECK( decoded.encode(FormatterAscii()) == "Hello World ^greenblue^x00?:)" );

    BOOST_CHECK( fmt.to_string("hello world^") == "hello world^^" );
    BOOST_CHECK( fmt.to_string('x') == "x" );
    BOOST_CHECK( fmt.to_string('^') == "^^" );
    BOOST_CHECK( fmt.to_string(Unicode("ç", 0x00E7)) == "ç" );
    BOOST_CHECK( fmt.to_string(color::red) == "^1" );
    BOOST_CHECK( fmt.to_string(color::nocolor) == "^7" );
    BOOST_CHECK( fmt.to_string(FormatFlags::BOLD) == "" );
    BOOST_CHECK( fmt.to_string(ClearFormatting()) == "^7" );

    // QFont
    xonotic::QFont qf(1000);
    BOOST_CHECK( qf.alternative() == "" );
}

BOOST_AUTO_TEST_CASE( test_rainbow  )
{
    fun::FormatterRainbow fmt;
    std::string utf8 = u8"Hello World§!!";
    auto decoded = fmt.decode(utf8);
    BOOST_CHECK( decoded.size() == 28 );
    for ( int i = 0; i < 11; i++ )
    {
        BOOST_CHECK( cast<color::Color12>(decoded[i*2]) );
        BOOST_CHECK( cast<char>(decoded[i*2+1]) );
        BOOST_CHECK( *cast<char>(decoded[i*2+1]) == utf8[i] );
    }
    BOOST_CHECK( cast<Unicode>(decoded[23]) );
    BOOST_CHECK( *cast<color::Color12>(decoded[0]) == color::red );
    BOOST_CHECK( decoded.encode(fmt) == utf8 );

    fun::FormatterRainbow fmt2(0.5,0.5,0.5);
    std::string str = "....";
    decoded = fmt2.decode(str);
    for ( unsigned i = 0; i < str.size(); i++ )
    {
        BOOST_CHECK( cast<color::Color12>(decoded[i*2]) );
        BOOST_CHECK( *cast<color::Color12>(decoded[i*2]) == color::Color12::hsv(0.5+i*0.25,0.5,0.5) );
    }

}

BOOST_AUTO_TEST_CASE( test_FormattedString )
{
    // Ctors
    BOOST_CHECK( FormattedString().empty() );
    BOOST_CHECK( FormattedString("foobar").size() == 1 );
    {
        FormattedString foo("foo");
        foo << "bar";
        BOOST_CHECK( !foo.empty() );
        FormattedString s1 = std::move(foo);
        BOOST_CHECK( s1.size() == 2 );
        FormattedString s2;
        s2 = std::move(s1);
        BOOST_CHECK( s2.size() == 2 );
        s2 = FormattedString();
        BOOST_CHECK( s2.size() == 0 );
    }

    // Iterator/Element access
    FormattedString s;
    BOOST_CHECK( s.begin() == s.end() );
    s << "Foo" << color::black;
    BOOST_CHECK( s.end() - s.begin() == 2 );
    BOOST_CHECK( cast<string::AsciiString>(*s.begin()) );
    BOOST_CHECK( cast<color::Color12>(*(s.end()-1)) );
    BOOST_CHECK( cast<string::AsciiString>(s[0]) );
    BOOST_CHECK( cast<color::Color12>(s[1]) );
    {
        const FormattedString& s2 = s;
        BOOST_CHECK( s2.begin() == s2.cbegin() );
        BOOST_CHECK( s2.end() == s2.cend() );
        BOOST_CHECK( cast<string::AsciiString>(s2[0]) );
        BOOST_CHECK( cast<color::Color12>(s2[1]) );
    }

    // Insert
    s.push_back(Element("bar"));
    BOOST_CHECK( cast<string::AsciiString>(s[2]) );
    s.insert(s.begin()+2,Element(color::blue));
    BOOST_CHECK( cast<color::Color12>(s[2]) );
    s.insert(s.begin()+2,3,Element(FormatFlags()));
    for ( int i = 2; i < 5; i++ )
        BOOST_CHECK( cast<FormatFlags>(s[i]) );
    {
        FormattedString s2;
        s2 << "Hello" << "World";
        s.insert(s.begin()+3, s2.begin(), s2.end());
        BOOST_CHECK( cast<string::AsciiString>(s[3]) );
        BOOST_CHECK( cast<string::AsciiString>(s[4]) );
    }
    s.insert(s.begin(),{
        Element(string::AsciiString("bar")),
        Element(color::red)
    });
    BOOST_CHECK( cast<string::AsciiString>(s[0]) );
    BOOST_CHECK( cast<color::Color12>(s[1]) );

    // Erase
    s.erase(s.begin()+1);
    BOOST_CHECK( cast<string::AsciiString>(s[1]) );
    s.erase(s.begin()+1,s.end()-1);
    BOOST_CHECK( s.size() == 2 );

    // Assign
    s.assign(5, Element(color::Color12(color::green)));
    BOOST_CHECK( s.size() == 5 );
    {
        FormattedString s2;
        s2 << "Hello" << "World" << "!";
        s.assign(s2.begin(),s2.end());
        BOOST_CHECK( s.size() == 3 );
    }
    s.assign({
        Element(string::AsciiString("bar")),
        Element(color::Color12(color::red))
    });
    BOOST_CHECK( s.size() == 2 );

    // Append
    s.append("hello");
    BOOST_CHECK( cast<string::AsciiString>(s[2]) );
    s.append(std::string("hello"));
    s.pop_back();
    s.append(color::Color12(color::white));
    BOOST_CHECK( cast<color::Color12>(s[3]) );
    {
        FormattedString s2;
        s2 << "Hello" << "World";
        s.append(s2);
        BOOST_CHECK( s.size() == 6 );
    }

    // operator<<
    s.clear();
    s << "hello";
    BOOST_CHECK( cast<string::AsciiString>(s[0]) );
    s << std::string("hello");
    BOOST_CHECK( cast<string::AsciiString>(s[1]) );
    s << color::dark_magenta;
    BOOST_CHECK( cast<color::Color12>(s[2]) );
    s << FormatFlags();
    BOOST_CHECK( cast<FormatFlags>(s[3]) );
    s << FormatFlags::BOLD;
    BOOST_CHECK( cast<FormatFlags>(s[4]) );
    s << ClearFormatting();
    BOOST_CHECK( cast<ClearFormatting>(s[5]) );
    s << 'c';
    BOOST_CHECK( cast<char>(s[6]) );
    {
        FormattedString s2;
        s2 << "Hello" << "World";
        s << s2;
        BOOST_CHECK( s.size() == 9 );
    }
    s << 12.3;
    BOOST_CHECK( cast<double>(s[9]) );

    // implode
    std::vector<FormattedString> v;
    v.push_back(FormattedString() << "hello" << color::red << "world");
    v.push_back(FormattedString() << 123);
    v.push_back(FormattedString() << FormatFlags::BOLD << "foo");
    FormattedString separator;
    separator << ClearFormatting() << ", ";

    s = implode(separator, v);
    BOOST_CHECK( s.size() == 10 );

    BOOST_CHECK( cast<string::AsciiString>(s[0]) );
    BOOST_CHECK( cast<color::Color12>(s[1]) );
    BOOST_CHECK( cast<string::AsciiString>(s[2]) );

    BOOST_CHECK( cast<ClearFormatting>(s[3]) );
    BOOST_CHECK( cast<string::AsciiString>(s[4]) );

    BOOST_CHECK( cast<int>(s[5]) );

    BOOST_CHECK( cast<ClearFormatting>(s[6]) );
    BOOST_CHECK( cast<string::AsciiString>(s[7]) );

    BOOST_CHECK( cast<FormatFlags>(s[8]) );
    BOOST_CHECK( cast<string::AsciiString>(s[9]) );
}

BOOST_AUTO_TEST_CASE( test_Misc )
{
    // FormatFlags
    FormatFlags fmt = FormatFlags::BOLD|FormatFlags::UNDERLINE;
    fmt &= ~FormatFlags::BOLD;
    BOOST_CHECK( fmt == FormatFlags::UNDERLINE );
    BOOST_CHECK( (fmt|FormatFlags::BOLD) ==
        (FormatFlags::BOLD|FormatFlags::UNDERLINE));
    BOOST_CHECK( fmt );
    BOOST_CHECK( fmt & FormatFlags::UNDERLINE );
    BOOST_CHECK( ~fmt & FormatFlags::BOLD );
}

BOOST_AUTO_TEST_CASE( test_Utf8Parser )
{
    for ( unsigned char c = 0; c < 128; c++ ) // C++ XD
    {
        BOOST_CHECK( Utf8Parser::to_ascii(c) == c );
        BOOST_CHECK( Utf8Parser::encode(c) == std::string(1,c) );
    }

#ifdef HAS_ICONV
    BOOST_CHECK( Utf8Parser::to_ascii("è") == 'e' );
    BOOST_CHECK( Utf8Parser::to_ascii("à") == 'a' );
    BOOST_CHECK( Utf8Parser::to_ascii("ç") == 'c' );
    BOOST_CHECK( Utf8Parser::to_ascii(0x00E7) == 'c' );
#endif

    BOOST_CHECK( Utf8Parser::encode(0x00A7) == u8"§" );
    BOOST_CHECK( Utf8Parser::encode(0x110E) == u8"ᄎ" );
    BOOST_CHECK( Utf8Parser::encode(0x26060) == u8"𦁠" );

}

BOOST_AUTO_TEST_CASE( test_Replacements )
{
    FormatterConfig cfg;
    FormatterAscii ascii;
    auto string = cfg.decode("$(red)$hellooo$hello,${hello}oo");
    std::map<std::string, std::string> replacements = {
        {"hello", "world"}
    };
    string.replace(replacements);
    replacements["hello"].append("!!");
    BOOST_CHECK( string.encode(ascii) == "world,worldoo" );

    BOOST_CHECK( string.replaced(replacements).encode(ascii) == "world!!,world!!oo" );

    std::map<std::string, FormattedString> replacements2 = {
        {"hello", "world"}
    };
    string.replace(replacements2);
    replacements2["hello"].append("!!");
    BOOST_CHECK( string.encode(ascii) == "world,worldoo" );

    BOOST_CHECK( string.replaced(replacements2).encode(ascii) == "world!!,world!!oo" );

    BOOST_CHECK( string.replaced("hellooo", "hi").encode(ascii) == "hiworld,worldoo" );

    BOOST_CHECK( cfg.decode("I'm $1").replaced("1", "the best").encode(ascii) == "I'm the best" );
}

BOOST_AUTO_TEST_CASE( test_Filters )
{
    FormatterConfig cfg;
    FormatterAscii ascii;
    FilterRegistry::instance().register_filter("colorize",
        [](const std::vector<FormattedString>& args) -> FormattedString
        {
            if ( args.empty() )
                return {};

            if ( args.size() < 2 )
                return args[0];

            return FormattedString()
                << color::Color12::from_name(args[0].encode(FormatterAscii()))
                << args[1]
                << color::Color12();
        }
    );

    FormattedString string = cfg.decode("$(colorize red hello)");
    BOOST_CHECK( cast<FilterCall>(string[0]) );

    auto filtered = cast<FilterCall>(string[0])->filtered();
    BOOST_CHECK( filtered.size() == 3 );
    BOOST_CHECK( cast<color::Color12>(filtered[0]) );
    BOOST_CHECK( *cast<color::Color12>(filtered[0]) == color::red );
    BOOST_CHECK( cast<color::Color12>(filtered[2]) );
    BOOST_CHECK( !cast<color::Color12>(filtered[2])->is_valid() );

    string = cfg.decode("$(colorize red \"hello $world\")");
    string.replace("world", "pony");
    BOOST_CHECK( string.encode(ascii) == "hello pony" );

    string = cfg.decode("$(colorize red $world) yay");
    string.replace("world", "pony");
    BOOST_CHECK( string.encode(ascii) == "pony yay" );

    BOOST_CHECK( cfg.decode("$(fake pony)").encode(ascii) == "pony" );

    // built-ins

    BOOST_CHECK( cfg.decode("$(plural 1 pony)").encode(ascii) == "pony" );
    BOOST_CHECK( cfg.decode("$(plural 6 pony)").encode(ascii) == "ponies" );
    BOOST_CHECK( cfg.decode("$(plural $count pony)").replaced("count", "6").encode(ascii) == "ponies" );
    BOOST_CHECK( cfg.decode("$(plural pony)").encode(ascii) == "" );

    BOOST_CHECK( cfg.decode("$(ucfirst 'pony princess')").encode(ascii) == "Pony princess" );
    BOOST_CHECK( cfg.decode("$(ucfirst)").encode(ascii) == "" );

    auto conditional = cfg.decode("$(if $test cmp 'hello $world' 'nope')");
    conditional.replace("test", "cmp");
    conditional.replace("world", "pony");
    BOOST_CHECK( conditional.encode(ascii) == "hello pony" );
    conditional.replace("test", "fail");
    BOOST_CHECK( conditional.encode(ascii) == "nope" );

    conditional = cfg.decode("$(if $test cmp 'hello')");
    conditional.replace("test", "cmp");
    BOOST_CHECK( conditional.encode(ascii) == "hello" );
    conditional.replace("test", "fail");
    BOOST_CHECK( conditional.encode(ascii) == "" );
}

BOOST_AUTO_TEST_CASE( test_Padding )
{
    FormatterAscii ascii;
    BOOST_CHECK( Padding("hello", 7).to_string(ascii) == "  hello" );
    BOOST_CHECK( Padding("hello", 7, 0).to_string(ascii) == "hello  " );
    BOOST_CHECK( Padding("hello", 7, 0.5).to_string(ascii) == " hello " );
    BOOST_CHECK( Padding("hello", 7, 1, '.').to_string(ascii) == "..hello" );
    BOOST_CHECK( (FormattedString() << Padding("hello", 7, 0) << Padding("world", 7, 1)).encode(ascii)  == "hello    world" );
}

BOOST_AUTO_TEST_CASE( test_Element )
{
    BOOST_CHECK( Element("foo").has_type<std::string>() );
    BOOST_CHECK( Element((int) 1).has_type<int>() );
    BOOST_CHECK( Element((const int) 1).has_type<int>() );
    Element e("foo");
    BOOST_CHECK( Element(e).has_type<std::string>() );
    BOOST_CHECK( Element(std::move(e)).has_type<std::string>() );
    BOOST_CHECK( Element(std::string("foo")).has_type<std::string>() );
    BOOST_CHECK( Element(FormatFlags(FormatFlags::BOLD)).has_type<FormatFlags>() );
    BOOST_CHECK( Element(FormatFlags::BOLD).has_type<FormatFlags>() );

    BOOST_CHECK_THROW(Element(1).reference<double>(), melanobot::MelanobotError);
    e = Element(1);
    BOOST_CHECK_THROW(e.reference<double>(), melanobot::MelanobotError);
}
