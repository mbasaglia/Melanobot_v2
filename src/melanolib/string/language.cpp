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
#include "language.hpp"

namespace melanolib {
namespace string {

Inflector::Inflector(const std::initializer_list<std::pair<std::string,std::string>>& rules, bool whole_words )
{
    auto alter_regex = [whole_words](const std::string& re){
        return whole_words ? "\\b"+re+"\\b" : re;
    };

    this->rules.reserve(rules.size());
    for ( auto& rule : rules )
    {
        this->rules.emplace_back(
            alter_regex(rule.first),
            std::move(rule.second)
        );
    }
}

std::string English::ordinal_suffix(int i) const
{

    if ( i <= 0 )
        return "";
    if ( i%100 < 11 || i%100 > 13 )
    {
        if ( i % 10 == 1 )
            return "st";
        if ( i % 10 == 2 )
            return "nd";
        if ( i % 10 == 3 )
            return "rd";
    }
    return "th";
}

std::string English::genitive(const std::string& noun) const
{
    return infl_genitive.inflect_one(noun);
}

std::string English::pronoun_to3rd(const std::string& sentence,
                                   const std::string& me,
                                   const std::string& you) const
{
    auto my = genitive(me);

    Inflector pronoun_swap ( {
        {"you\\s+are",  you+" is"},
        {"are\\s+you",  "is "+you},
        {"yourself",    "itself"},
        {"yours",       "its"},
        {"your",        "its"},
        {"you",         you},

        {"thou\\s+art", you+" is"},
        {"art\\s+thou", "is "+you},
        {"thyself",     "itself"},
        {"thine",       "its"},
        {"thy",         "its"},
        {"thou",        you},
        {"thee",        you},

        {"am",          "is"},
        {"I\'m",        me+" is"},
        {"I",           me},
        {"me",          me},
        {"myself",      me},
        {"my",          my},
        {"mine",        my},
    }, true );
    return pronoun_swap.inflect_all(sentence);

}
std::string English::pronoun_1stto3rd(const std::string& sentence,
                                   const std::string& me) const
{
    auto my = genitive(me);

    Inflector pronoun_swap ( {
        {"am",          "is"},
        {"I\'m",        me+" is"},
        {"I",           me},
        {"me",          me},
        {"myself",      me},
        {"my",          my},
        {"mine",        my},
    }, true );
    return pronoun_swap.inflect_all(sentence);

}

std::string English::imperate(const std::string& verb) const
{
    return infl_imperate.inflect_one(verb);
}

/**
 * \todo Unit test
 */
std::string English::pluralize(int number, const std::string& noun) const
{
    if ( number == 1 )
        return noun;
    return infl_plural.inflect_one(noun);
}

std::string English::pluralize_with_number(int number, const std::string& noun) const
{
    return std::to_string(number)+" "+pluralize(number,noun);
}

std::string English::indefinite_article(const std::string& subject) const
{
    auto it = std::find_if(subject.begin(), subject.end(),
        [](char c){return std::isalpha(c);}
    );
    if ( it == subject.end() )
        return {};

    switch ( *it )
    {
        case 'a': case 'e': case 'i': case 'o': case 'u':
            return "an ";
        default:
            return "a ";
    }
}

Inflector English::infl_imperate({
        {"can",                          "can"},
        {"don\'t be",                    "isn\'t"},
        {"be",                           "is"},
        {"have",                         "has"},
        {"say",                          "says"},
        {"don\'t",                       "doesn\'t"},
        {"(.*[bcdfghjklmnpqrstvwxyz]o)", "$1es"},
        {"(.*(z|s|ch|sh|j|zh))",         "$1es"},
        {"(.*[bcdfghjklmnpqrstvwxyz])y", "$1ies"},
        {"(.+)",                         "$1s"},
    }, true);

Inflector English::infl_genitive = {
    {"(.*s)$", "$1'"},
    {"(.+)$", "$1's"},
};

/// \todo Irregular English plurals
Inflector English::infl_plural = {
    {"(.*[bcdfghjklmnpqrstvwxyz]o)", "$1es"},
    {"(.*(z|s|ch|sh|j|zh))",         "$1es"},
    {"(.*[bcdfghjklmnpqrstvwxyz])y", "$1ies"},
    {"(.+)$", "$1s"},
};


} // namespace string
} // namespace melanolib
