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
#ifndef STRING_LANGUAGE_HPP
#define STRING_LANGUAGE_HPP

#include <string>
#include <regex>
#include <unordered_map>

namespace melanolib {
namespace string {

/**
 * \brief Class to perform simple pattern-based transformations
 */
class Inflector
{
private:
    /**
     * \brief Pattern/replacement pair
     */
    struct Rule
    {
        std::regex  search;
        std::string replace;

        Rule(std::string search, std::string replace)
            : search(std::move(search)), replace(std::move(replace)) {}

        Rule(std::regex search, std::string replace)
            : search(std::move(search)), replace(std::move(replace)) {}
    };

public:

    Inflector(const std::initializer_list<Rule>& il)
        : rules(il) {}

    Inflector(const std::initializer_list<std::pair<std::string,std::string>>& rules, bool whole_words);

    /**
     * \brief Inflects a phrase
     * \param phrase Phrase to inflect
     *
     * Runs all the rules on the phrase
     */
    std::string inflect_all(std::string phrase)
    {
        for ( const auto& regex : rules )
        {
            phrase = std::regex_replace(phrase,regex.search,regex.replace);
        }

        return phrase;
    }

    /**
     * \brief Inflects a phrase
     * \param phrase Phrase to inflect
     *
     * Runs the rules in order, stops at the first match
     * \note \c phrase must match the regex to apply the rule
     */
    std::string inflect_one(std::string phrase)
    {
        for ( const auto& regex : rules )
        {
            if ( std::regex_match(phrase, regex.search) )
                return std::regex_replace(phrase,regex.search,regex.replace);
        }

        return phrase;
    }

private:
    std::vector<Rule> rules;
};

/**
 * \brief Inflects words to be used in a natural language
 * \note This is English-centric might not be suitable for other language
 */
class Language
{
public:
    virtual ~Language() {}

    /**
     * \brief Returns the suffix used to represent an ordinal number
     */
    virtual std::string ordinal_suffix(int n) const = 0;

    /**
     * \brief Builds the genitive of a noun (indicating possession)
     */
    virtual std::string genitive(const std::string& noun) const = 0;

    /**
     * \brief Converts a sentence to 3rd person singular
     * \param sentence  Sentence to be transformed
     * \param me        Name considered to replace the first person
     * \param you       Name considered to replace the second person
     */
    virtual std::string pronoun_to3rd(const std::string& sentence,
                                 const std::string& me,
                                 const std::string& you) const = 0;


    /**
     * \brief Converts a sentence from 1st to 3rd person singular
     * \param sentence  Sentence to be transformed
     * \param me        Name considered to replace the first person
     * \param you       Name considered to replace the second person
     */
    virtual std::string pronoun_1stto3rd(const std::string& sentence,
                                 const std::string& me) const = 0;

    /**
     * \brief Transform a verb from its imperative form
     *        to 3rd person singular (present tense)
     */
    virtual std::string imperate(const std::string& verb) const = 0;

    /**
     * \brief Pluralize a noun according to the given number
     */
    virtual std::string pluralize(int number, const std::string& noun) const = 0;

    /**
     * \brief Pluralize a noun according to the given number
     *        (The output shall contain the number)
     */
    virtual std::string pluralize_with_number(int number, const std::string& noun) const = 0;

    /**
     * \brief Returns a string to be prepended to \p subject
     */
    virtual std::string indefinite_article(const std::string& subject) const = 0;
};

/**
 * \brief English language
 */
class English final : public Language
{
public:

    std::string ordinal_suffix(int n) const override;

    std::string genitive(const std::string& noun) const override;

    std::string pronoun_to3rd(const std::string& sentence,
                              const std::string& me,
                              const std::string& you) const override;

    std::string pronoun_1stto3rd(const std::string& sentence,
                                 const std::string& me) const override;

    std::string imperate(const std::string& verb) const override;

    std::string pluralize(int number, const std::string& noun) const override;

    std::string pluralize_with_number(int number, const std::string& noun) const override;

    std::string indefinite_article(const std::string& subject) const override;

private:
    static Inflector infl_imperate;///< Inflector for imperative to 3rd person
    static Inflector infl_genitive;///< Inflector for the English genitive
    static Inflector infl_plural;  ///< Inflector for the English plural
};

} // namespace string
} // namespace melanolib

#endif // STRING_LANGUAGE_HPP
