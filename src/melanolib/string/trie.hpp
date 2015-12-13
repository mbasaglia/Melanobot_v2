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
#ifndef MELANOLIB_STRING_TRIE_HPP
#define MELANOLIB_STRING_TRIE_HPP

#include <algorithm>
#include <type_traits>
#include <unordered_map>

namespace melanolib {
namespace string {

/**
 * \brief Prefix Tree with optionally associated data
 * \tparam MappedType   Value associated with the keys.
 *                      Must be void or DefaultConstructible, MoveConstructible
 *                      For deep copy to work, CopyAssignable
 */
template<class MappedType>
class BasicTrie
{
public:
    using value_type = MappedType;
    using key_type   = std::string;

private:

    /**
     * \brief Class to allow real data as well as no data (void) for TrieNode
     */
    class TrieNodeData
    {
    public:
        TrieNodeData() {}

        TrieNodeData(TrieNodeData&&)
            noexcept(std::is_nothrow_move_constructible<MappedType>::value) = default;

        TrieNodeData& operator=(TrieNodeData&&)
            noexcept(std::is_nothrow_move_assignable<MappedType>::value) = default;

        template<class... Data>
            TrieNodeData(Data&&... data) : data(std::forward<Data>(data)...) {}

        /**
         * \brief Copy assigns \c data
         */
        void copy_data(const TrieNodeData& other) { data = other.data; }

        /**
         * \brief Returns \c data
         */
        const MappedType& get_data() const { return data; }

    private:
        MappedType data; ///< Associated data
    };

    /**
     * \brief A node in the tree
     */
    struct TrieNode : TrieNodeData
    {
        TrieNode* parent{nullptr};                      ///< Parent node
        bool marks_end{false};                          ///< Accepts the input
        std::unordered_map<char,TrieNode*> children;    ///< Child nodes
        int depth{0};                                   ///< Distance from the root

        TrieNode() : parent(nullptr), depth(0) {}
        TrieNode(TrieNode* parent, int depth) : parent(parent), depth(depth) {}
        TrieNode(const TrieNode&) = delete;
        TrieNode(TrieNode&&)
            noexcept(std::is_nothrow_move_constructible<TrieNodeData>::value &&
                std::is_nothrow_move_constructible<std::remove_reference_t<decltype(children)>>::value
            )
            = default;
        TrieNode& operator=(const TrieNode&) = delete;
        TrieNode& operator=(TrieNode&&)
            noexcept(std::is_nothrow_move_assignable<TrieNodeData>::value &&
                std::is_nothrow_move_constructible<std::remove_reference_t<decltype(children)>>::value
            ) = default;
        ~TrieNode()
        {
            for ( auto child : children )
                delete child.second;
        }

        /**
         * \brief Increase the depth by that amount
         * \complexity O(size of the sub-tree)
         */
        void deepen(int amount)
        {
            depth += amount;
            for (auto& pair : children)
            {
               pair.second->deepen(amount);
            }
        }

        /**
        * \brief Gets the child corresponding to the given character
        * \complexity Best: O(1) Worst: O(number of children)
        */
        TrieNode* get_child(char c) const
        {
            auto iter = children.find(c);
            if (iter == children.end())
                return nullptr;
            return iter->second;
        }

        /**
        * \brief Gets the child corresponding to the given character (creating it when needed)
        * \complexity Best: O(1) Worst: O(number of children)
        */
        TrieNode* get_or_create_child(char c)
        {
            if (children[c])
                return children[c];
            return children[c] = new TrieNode(this, depth+1);
        }

        /**
        * \brief Copy the sub-tree rooted in the current node
        * \complexity O(size of the sub-tree)
        */
        TrieNode* deep_copy() const
        {
            auto node = new TrieNode();
            node->marks_end = marks_end;
            node->depth = depth;
            node->copy_data(*this);

            for (const auto& pair : children)
            {
                auto new_child = pair.second->deep_copy();
                new_child->parent = node;
                node->children[pair.first] = new_child;
            }

            return node;
        }

        /**
         * \brief Removes the given child
         * \complexity O(number of children)
         */
        void remove_child(TrieNode* child)
        {
            auto it = std::find_if(children.begin(),children.end(),
                [child](auto p) { return p.second == child; });
            if ( it != children.end() )
            {
                delete it->second;
                children.erase(it);
            }
        }
    };

public:
    /**
     * \brief Handle to the node
     */
    class iterator
    {
    public:
        iterator() : node(nullptr) {}

        /**
         * \brief Whether the iterator is valid
         * \complexity O(1)
         */
        bool valid() const
        {
            return node;
        }

        /**
         * \brief Move up the trie
         * \complexity O(1)
         */
        void move_up()
        {
            if ( node )
                node = node->parent;
        }

        /**
         * \brief Move down the trie
         * \complexity Average: O(1) Worst: O(number of children)
         */
        void move_down(char c)
        {
            if ( node )
                node = node->get_child(c);
        }

        /**
         * \brief Whether it can move down the tree
         * \complexity Average: O(1) Worst: O(number of children)
         */
        bool can_move_down(char c)
        {
            return node && node->get_child(c);
        }

        /**
         * \brief Whether the node the root
         * \complexity O(1)
         */
        bool root() const
        {
            return node && !node->parent;
        }

        /**
         * \brief Whether the node accepts the input
         * \complexity O(1)
         */
        bool accepts() const
        {
            return node && node->marks_end;
        }

        /**
         * \brief Depth of the node / length of the prefix
         * \complexity O(1)
         */
        int depth() const
        {
            return node ? node->depth : 0;
        }

        MappedType data()
        {
            if ( node ) return node->get_data();
            return MappedType();
        }

    private:
        TrieNode* node{nullptr};        ///< Node handled by the iterator

        iterator(TrieNode* node) : node(node) {}

        friend class BasicTrie;
    };
    using const_iterator = iterator;

    /**
     * \brief Constructs an empty tree
     * \compexity O(1)
     */
    BasicTrie() : root_(new TrieNode) {}

    /**
     * \brief Initialize from the words in the initializer list
     */
    BasicTrie(const std::initializer_list<std::string>& il) : BasicTrie()
    {
        for ( const auto& word : il )
            insert(word);
    }

    /**
     * \brief Copies from another trie
     * \complexity O(number of nodes in \c other)
     */
    BasicTrie(const BasicTrie& other) : root_(other.root_->deep_copy()) {}

    /**
     * \brief Copies from another trie
     * \complexity O(number of nodes in \c other)
     */
    BasicTrie& operator= (const BasicTrie& other)
    {
        root_ = other.root_->deep_copy();
        return *this;
    }

    BasicTrie(BasicTrie&& other) noexcept : root_(other.root_)
    {
        other.root_ = nullptr;
    }

    BasicTrie& operator= (BasicTrie&& other) noexcept
    {
        std::swap(root_,other.root_);
        return *this;
    }

    ~BasicTrie()
    {
        delete root_;
    }

    /**
     * \brief Returns \b true if the trie doesn't have any meaningful node
     */
    bool empty() const
    {
        return !root_ || root_->children.empty();
    }

    /**
     * \brief Adds a new word to the trie
     * \complexity O(word.size())
     */
    iterator insert(const std::string& word, const TrieNodeData& data = {})
    {
        TrieNode* node = root_;
        for ( auto c : word )
            node = node->get_or_create_child(c);
        node->marks_end = true;
        node->copy_data(data);
        return iterator(node);
    }

    /**
     * \brief Prepends a single character to all words
     * \complexity O(number of node) (caused by deepen)
     */
    void prepend(char c)
    {
        if ( empty() )
            return;
        TrieNode* new_root = new TrieNode;
        new_root->children[c] = root_;
        root_->deepen(1);
        root_ = new_root;
    }

    /**
     * \brief Prepends a prefix to all words
     * \complexity O(prefix.size())
     */
    void prepend(const std::string& prefix)
    {
        if ( prefix.empty() || empty() )
            return;

        TrieNode* new_root = new TrieNode;

        TrieNode* node = new_root;
        for ( std::string::size_type i = 0; i < prefix.size()-1; i++ )
            node = node->get_or_create_child(prefix[i]);

        node->children[prefix.back()] = root_;

        root_->deepen(prefix.size());
        root_ = new_root;

    }

    /**
     * \brief Removes a word from the trie
     * \complexity O(word.size())
     */
    void erase(const std::string& word)
    {
        auto node = find_node(word);
        if ( node )
        {
            node->marks_end = false;
            remove_dangling(node);
        }
    }

    /**
     * \brief Checks if a word exists
     * \complexity O(word.size())
     */
    bool contains(const std::string& word) const
    {
        auto node = find_node(word);
        return node && node->marks_end;
    }

    /**
     * \brief Checks if a prefix exists
     * \complexity O(word.size())
     */
    bool contains_prefix(const std::string& word) const
    {
        return find_node(word);
    }

    /**
     * \brief Returns an iterator to the root
     * \complexity O(1)
     */
    iterator root() const
    {
        return iterator(root_);
    }

    /**
     * \brief Returns an iterator to the root
     * \complexity O(word.size())
     */
    iterator find( const std::string& word) const
    {
        return iterator(find_node(word));
    }


    /**
     * \brief Recursively call a function on iterators
     * \complexity O(number of nodes)
     * \tparam Functor function object taking an \c iterator and a \c std::string
     *
     * Calls \c functor repeatedly passing an iterator to the node and
     * its prefix as a \c std::string
     */
    template<class Functor>
    void recurse(const Functor& functor) const
    {
        recurse(root_,functor,"");
    }

private:

    TrieNode* root_; ///< Root node

    /**
     * \brief Removes branches which don't lead to any word
     * \complexity O(h)
     */
    void remove_dangling(TrieNode* node)
    {
        TrieNode* parent = node->parent;;
        while(node && parent && !parent->marks_end && parent->children.size() <= 1)
        {
            node = parent;
            parent = parent->parent;
        }

        if (parent)
            parent->remove_child(node);
    }

    /**
     * \brief Finds a node matching the given word (or prefix)
     * \returns \b nullptr if not found
     * \complexity O(word.size())
     */
    TrieNode* find_node(const std::string& word) const
    {
        TrieNode* node = root_;
        for ( auto c : word )
        {
            node = node->get_child(c);
            if (!node)
                return nullptr;
        }
        return node;
    }

    /**
     * \brief Recursively call a function on iterators
     * \complexity O(nodes in the sub-tree)
     */
    template<class Functor>
    void recurse(const TrieNode* node, const Functor& functor, const std::string& prefix) const
    {
        if (node)
        {
            functor(iterator(node));
            for(const auto &pair : node->children)
                recurse(pair.second,functor,prefix+pair.first);
        }
    }
};

/**
 * \brief Specialize for void data
 */
template<>
class BasicTrie<void>::TrieNodeData
{
public:
    void get_data() {}
    void copy_data(const TrieNodeData& ) {}
};

/**
 * \brief A trie with no associated data
 */
using Trie = BasicTrie<void>;

/**
 * \brief A trie with string data
 */
using StringTrie = BasicTrie<std::string>;

/**
 * \brief Builds a trie from an associative container
 * \tparam AssocContainer Associative container,
 *         AssocContainer::key_type convertible to string,
 *         AssocContainer::mapped_type is used as parameter for BasicTrie
 */
template<class AssocContainer, class = std::enable_if_t<std::is_convertible<typename AssocContainer::key_type, std::string>::value>>
   auto make_trie(const AssocContainer& container)
    {
        BasicTrie<typename AssocContainer::mapped_type> trie;

        for ( const auto& pair : container )
            trie.insert(pair.first,pair.second);

        return trie;
    }

/**
 * \brief Builds a trie from a container
 * \tparam Container Container, AssocContainer::value_type convertible to string.
 */
template<class Container, class = void, class = std::enable_if_t<std::is_convertible<typename Container::value_type, std::string>::value>>
    auto make_trie(const Container& container)
    {
        Trie trie;

        for ( const auto& val : container )
            trie.insert(val);

        return trie;
    }

} // namespace string
} // namespace melanolib
#endif // MELANOLIB_STRING_TRIE_HPP
