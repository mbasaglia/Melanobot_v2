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
    return dynamic_cast<const T*>(item.operator->());
}

BOOST_AUTO_TEST_CASE( test_utf8 )
{
    FormatterUtf8 fmt;
    std::string utf8 = u8"Foo bar è$ç";
    auto decoded = fmt.decode(utf8);
    BOOST_CHECK( decoded.size() == 4 );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]));
    BOOST_CHECK( cast<Unicode>(decoded[1]));
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]));
    BOOST_CHECK( cast<Unicode>(decoded[3]));
    BOOST_CHECK( decoded.encode(fmt) == utf8 );
    auto invalid_utf8 = utf8;
    invalid_utf8.push_back(uint8_t(0b1110'0000)); // '));
    BOOST_CHECK( fmt.decode(invalid_utf8).encode(fmt) == utf8 );

    BOOST_CHECK( decoded.encode(*Formatter::formatter(fmt.name())) == utf8 );
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
}

BOOST_AUTO_TEST_CASE( test_config )
{
    FormatterConfig fmt;
    std::string utf8 = u8"Foo bar è#ç";
    auto decoded = fmt.decode(utf8);
    BOOST_CHECK( decoded.size() == 4 );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]));
    BOOST_CHECK( cast<Unicode>(decoded[1]));
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]));
    BOOST_CHECK( cast<Unicode>(decoded[3]));
    BOOST_CHECK( decoded.encode(fmt) == utf8 );

    std::string formatted = "Hello $(1)World $(-bu)test$(-)#1$(green)green$(x00f)blue§$$(1)";
    decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 13 );
    // "Hello "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0])->string() == "Hello " );
    // red
    BOOST_CHECK( cast<Color>(decoded[1]) );
    BOOST_CHECK( cast<Color>(decoded[1])->color() == color::red );
    // "World "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2])->string() == "World " );
    // bold+underline
    BOOST_CHECK( cast<Format>(decoded[3]) );
    BOOST_CHECK( cast<Format>(decoded[3])->flags() == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4])->string() == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6])->string() == "#1" );
    // green
    BOOST_CHECK( cast<Color>(decoded[7]) );
    BOOST_CHECK( cast<Color>(decoded[7])->color() == color::green );
    // "green"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8])->string() == "green" );
    // blue
    BOOST_CHECK( cast<Color>(decoded[9]) );
    BOOST_CHECK( cast<Color>(decoded[9])->color() == color::blue );
    // "blue"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10])->string() == "blue" );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[11]) );
    BOOST_CHECK( cast<Unicode>(decoded[11])->utf8() == u8"§" );
    BOOST_CHECK( cast<Unicode>(decoded[11])->point() == 0x00A7 );
    // "$(1)"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[12]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[12])->string() == "$(1)" );


    BOOST_CHECK( decoded.encode(fmt) == "Hello $(1)World $(-bu)test$(-)#1$(2)green$(4)blue§$$(1)" );
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
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0])->string() == "Hello " );
    // red
    BOOST_CHECK( cast<Color>(decoded[1]) );
    BOOST_CHECK( cast<Color>(decoded[1])->color() == color::dark_red );
    // "World "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2])->string() == "World " );
    // bold+underline
    BOOST_CHECK( cast<Format>(decoded[3]) );
    BOOST_CHECK( cast<Format>(decoded[3])->flags() == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4])->string() == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6])->string() == "#1" );
    // green
    BOOST_CHECK( cast<Color>(decoded[7]) );
    BOOST_CHECK( cast<Color>(decoded[7])->color() == color::green );
    // "green"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8])->string() == "green" );
    // blue
    BOOST_CHECK( cast<Color>(decoded[9]) );
    BOOST_CHECK( cast<Color>(decoded[9])->color() == color::blue );
    // "blue"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10])->string() == "blue" );
    // nocolor
    BOOST_CHECK( cast<Color>(decoded[11]) );
    BOOST_CHECK( cast<Color>(decoded[11])->color() == color::nocolor );
    // "$"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[12]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[12])->string() == "$" );

    BOOST_CHECK( decoded.encode(fmt) == "Hello \x1b[31mWorld \x1b[1;4;23mtest\x1b[0m#1\x1b[92mgreen\x1b[94mblue\x1b[39m$" );
}

BOOST_AUTO_TEST_CASE( test_ansi_utf8 )
{
    FormatterAnsi fmt(true);

    std::string formatted = "Hello \x1b[31mWorld \x1b[1;4;41mtest\x1b[0m#1\x1b[92mgreen\x1b[1;34mblue\x1b[39m§";
    auto decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 13 );
    // "Hello "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0])->string() == "Hello " );
    // red
    BOOST_CHECK( cast<Color>(decoded[1]) );
    BOOST_CHECK( cast<Color>(decoded[1])->color() == color::dark_red );
    // "World "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2])->string() == "World " );
    // bold+underline
    BOOST_CHECK( cast<Format>(decoded[3]) );
    BOOST_CHECK( cast<Format>(decoded[3])->flags() == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4])->string() == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6])->string() == "#1" );
    // green
    BOOST_CHECK( cast<Color>(decoded[7]) );
    BOOST_CHECK( cast<Color>(decoded[7])->color() == color::green );
    // "green"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8])->string() == "green" );
    // blue
    BOOST_CHECK( cast<Color>(decoded[9]) );
    BOOST_CHECK( cast<Color>(decoded[9])->color() == color::blue );
    // "blue"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10])->string() == "blue" );
    // nocolor
    BOOST_CHECK( cast<Color>(decoded[11]) );
    BOOST_CHECK( cast<Color>(decoded[11])->color() == color::nocolor );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[12]) );
    BOOST_CHECK( cast<Unicode>(decoded[12])->utf8() == u8"§" );

    BOOST_CHECK( decoded.encode(fmt) == "Hello \x1b[31mWorld \x1b[1;4;23mtest\x1b[0m#1\x1b[92mgreen\x1b[94mblue\x1b[39m§" );
}

BOOST_AUTO_TEST_CASE( test_irc )
{
    irc::FormatterIrc fmt;

    std::string formatted = "Hello \x03""04,05World \x02\x1ftest\x0f#1\x03""09green\x03""12blue§\x03";
    auto decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 13 );
    // "Hello "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0])->string() == "Hello " );
    // red
    BOOST_CHECK( cast<Color>(decoded[1]) );
    BOOST_CHECK( cast<Color>(decoded[1])->color() == color::red );
    // "World "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2])->string() == "World " );
    // bold+underline
    BOOST_CHECK( cast<Format>(decoded[3]) );
    BOOST_CHECK( cast<Format>(decoded[3])->flags() == (FormatFlags::BOLD|FormatFlags::UNDERLINE) );
    // "test"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4])->string() == "test" );
    // clear
    BOOST_CHECK( cast<ClearFormatting>(decoded[5]) );
    // "#1"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6])->string() == "#1" );
    // green
    BOOST_CHECK( cast<Color>(decoded[7]) );
    BOOST_CHECK( cast<Color>(decoded[7])->color() == color::green );
    // "green"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[8])->string() == "green" );
    // blue
    BOOST_CHECK( cast<Color>(decoded[9]) );
    BOOST_CHECK( cast<Color>(decoded[9])->color() == color::blue );
    // "blue"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[10])->string() == "blue" );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[11]) );
    BOOST_CHECK( cast<Unicode>(decoded[11])->utf8() == u8"§" );
    BOOST_CHECK( cast<Unicode>(decoded[11])->point() == 0x00A7 );
    // invalid color
    BOOST_CHECK( cast<Color>(decoded[12]) );
    BOOST_CHECK( cast<Color>(decoded[12])->color() == color::nocolor );

    BOOST_CHECK( decoded.encode(fmt) == "Hello \x03""04World \x02\x1ftest\x0f#1\x03""09green\x03""12blue§\xf" );
}

BOOST_AUTO_TEST_CASE( test_xonotic )
{
    xonotic::Formatter fmt;

    std::string formatted = "Hello ^1World ^^^2green^x00fblue^x00§\ue012";
    auto decoded = fmt.decode(formatted);
    BOOST_CHECK( decoded.size() == 9 );
    // "Hello "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[0])->string() == "Hello " );
    // red
    BOOST_CHECK( cast<Color>(decoded[1]) );
    BOOST_CHECK( cast<Color>(decoded[1])->color() == color::red );
    // "World "
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[2])->string() == "World ^" );
    // green
    BOOST_CHECK( cast<Color>(decoded[3]) );
    BOOST_CHECK( cast<Color>(decoded[3])->color() == color::green );
    // "green"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[4])->string() == "green" );
    // blue
    BOOST_CHECK( cast<Color>(decoded[5]) );
    BOOST_CHECK( cast<Color>(decoded[5])->color() == color::blue );
    // "blue"
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6]) );
    BOOST_CHECK( cast<AsciiSubstring>(decoded[6])->string() == "blue^x00" );
    // Unicode §
    BOOST_CHECK( cast<Unicode>(decoded[7]) );
    BOOST_CHECK( cast<Unicode>(decoded[7])->utf8() == u8"§" );
    // QFont smile
    BOOST_CHECK( cast<QFont>(decoded[8]) );
    BOOST_CHECK( cast<QFont>(decoded[8])->index() == 0x12 );
    BOOST_CHECK( cast<QFont>(decoded[8])->alternative() == ":)" );
    BOOST_CHECK( cast<QFont>(decoded[8])->unicode_point() == 0xe012 );

    BOOST_CHECK( decoded.encode(fmt) == "Hello ^1World ^^^2green^4blue^^x00§\ue012" );

    BOOST_CHECK( decoded.encode(FormatterAscii()) == "Hello World ^greenblue^x00?:)" );
}

BOOST_AUTO_TEST_CASE( test_rainbow  )
{
    fun::FormatterRainbow fmt;
    std::string utf8 = u8"Hello World§!!";
    auto decoded = fmt.decode(utf8);
    BOOST_CHECK( decoded.size() == 28 );
    for ( int i = 0; i < 11; i++ )
    {
        BOOST_CHECK( cast<Color>(decoded[i*2]) );
        BOOST_CHECK( cast<Character>(decoded[i*2+1]) );
        BOOST_CHECK( cast<Character>(decoded[i*2+1])->character() == utf8[i] );
    }
    BOOST_CHECK( cast<Unicode>(decoded[23]) );
    BOOST_CHECK( cast<Color>(decoded[0])->color() == color::red );
    BOOST_CHECK( decoded.encode(fmt) == utf8 );

    fun::FormatterRainbow fmt2(0.5,0.5,0.5);
    std::string str = "....";
    decoded = fmt2.decode(str);
    for ( unsigned i = 0; i < str.size(); i++ )
    {
        BOOST_CHECK( cast<Color>(decoded[i*2]) );
        BOOST_CHECK( cast<Color>(decoded[i*2])->color() == color::Color12::hsv(0.5+i*0.25,0.5,0.5) );
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
    BOOST_CHECK( cast<AsciiSubstring>(*s.begin()) );
    BOOST_CHECK( cast<Color>(*(s.end()-1)) );
    BOOST_CHECK( cast<AsciiSubstring>(s[0]) );
    BOOST_CHECK( cast<Color>(s[1]) );
    {
        const FormattedString& s2 = s;
        BOOST_CHECK( s2.begin() == s2.cbegin() );
        BOOST_CHECK( s2.end() == s2.cend() );
        BOOST_CHECK( cast<AsciiSubstring>(s2[0]) );
        BOOST_CHECK( cast<Color>(s2[1]) );
    }

    // Insert
    s.push_back(melanolib::New<AsciiSubstring>("bar"));
    BOOST_CHECK( cast<AsciiSubstring>(s[2]) );
    s.insert(s.begin()+2,melanolib::New<Color>(color::blue));
    BOOST_CHECK( cast<Color>(s[2]) );
    s.insert(s.begin()+2,3,melanolib::New<Format>(FormatFlags()));
    for ( int i = 2; i < 5; i++ )
        BOOST_CHECK( cast<Format>(s[i]) );
    {
        FormattedString s2;
        s2 << "Hello" << "World";
        s.insert(s.begin()+3,s2.begin(),s2.end());
        BOOST_CHECK( cast<AsciiSubstring>(s[3]) );
        BOOST_CHECK( cast<AsciiSubstring>(s[4]) );
    }
    s.insert(s.begin(),{
        melanolib::New<AsciiSubstring>("bar"),
        melanolib::New<Color>(color::red)
    });
    BOOST_CHECK( cast<AsciiSubstring>(s[0]) );
    BOOST_CHECK( cast<Color>(s[1]) );

    // Erase
    s.erase(s.begin()+1);
    BOOST_CHECK( cast<AsciiSubstring>(s[1]) );
    s.erase(s.begin()+1,s.end()-1);
    BOOST_CHECK( s.size() == 2 );

    // Assign
    s.assign(5,melanolib::New<Color>(color::green));
    BOOST_CHECK( s.size() == 5 );
    {
        FormattedString s2;
        s2 << "Hello" << "World" << "!";
        s.assign(s2.begin(),s2.end());
        BOOST_CHECK( s.size() == 3 );
    }
    s.assign({
        melanolib::New<AsciiSubstring>("bar"),
        melanolib::New<Color>(color::red)
    });
    BOOST_CHECK( s.size() == 2 );

    // Append
    s.append<AsciiSubstring>("hello");
    BOOST_CHECK( cast<AsciiSubstring>(s[2]) );
    s.append<AsciiSubstring>(std::string("hello"));
    s.pop_back();
    s.append(melanolib::New<Color>(color::white));
    BOOST_CHECK( cast<Color>(s[3]) );
    {
        FormattedString s2;
        s2 << "Hello" << "World";
        s.append(s2);
        BOOST_CHECK( s.size() == 6 );
    }

    // operator<<
    s.clear();
    s << "hello";
    BOOST_CHECK( cast<AsciiSubstring>(s[0]) );
    s << std::string("hello");
    BOOST_CHECK( cast<AsciiSubstring>(s[1]) );
    s << color::dark_magenta;
    BOOST_CHECK( cast<Color>(s[2]) );
    s << FormatFlags();
    BOOST_CHECK( cast<Format>(s[3]) );
    s << FormatFlags::BOLD;
    BOOST_CHECK( cast<Format>(s[4]) );
    s << ClearFormatting();
    BOOST_CHECK( cast<ClearFormatting>(s[5]) );
    s << 'c';
    BOOST_CHECK( cast<Character>(s[6]) );
    {
        FormattedString s2;
        s2 << "Hello" << "World";
        s << s2;
        BOOST_CHECK( s.size() == 9 );
    }
    s << 12.3;
    BOOST_CHECK( cast<AsciiSubstring>(s[9]) );
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

    // QFont
    QFont qf(1000);
    BOOST_CHECK( qf.alternative() == "" );
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
    BOOST_CHECK( cast<Color>(filtered[0]) );
    BOOST_CHECK( cast<Color>(filtered[0])->color() == color::red );
    BOOST_CHECK( cast<Color>(filtered[2]) );
    BOOST_CHECK( !cast<Color>(filtered[2])->color().is_valid() );

    string = cfg.decode("$(colorize red \"hello $world\")");
    string.replace("world", "pony");
    BOOST_CHECK( string.encode(FormatterAscii()) == "hello pony" );

    string = cfg.decode("$(colorize red $world) yay");
    string.replace("world", "pony");
    BOOST_CHECK( string.encode(FormatterAscii()) == "pony yay" );
}
