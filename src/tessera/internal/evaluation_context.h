#pragma once

#include "value.h"
#include <optional>
#include <map>
#include "gc_heap.h"

namespace tess {

    class evaluation_context;

    class scope_frame {
    public:
        scope_frame() {};
        scope_frame(const std::vector<std::string>& param, const std::vector<value_>& arg);
        scope_frame(const std::vector<std::tuple<std::string, value_>>& assignments);
        scope_frame(const std::string& var, value_ val);
        scope_frame(const std::vector<value_>& arg);
        std::optional<value_> get(int ph) const;
        std::optional<value_> get(std::string str) const;
        bool has(std::string str) const;
        std::vector<value_> values() const;
        void set(const std::string& var, value_ val);
        void set(int i, value_ val);
        void set(const::std::vector<std::string>& vars, const::std::vector<value_>& vals);
        using iterator = std::map<std::string, value_>::const_iterator;
        iterator begin() const;
        iterator end() const;
        std::string to_string() const;
        std::string serialize(serialization_state& state) const;
        //void get_references(std::unordered_set<tess::obj_id>& objects) const;
    private:
        std::map<std::string, value_> definitions_;
    };

    class execution_state;

    class evaluation_context
    {
        friend class execution_state;

        std::optional<value_> get_maybe(const std::string& var) const;

    public:
        bool contains(const std::string& var) const;
        bool contains(int i) const;
        bool has(const std::string& var) const;
        value_ get(const std::string& var) const;
        value_ get(int i) const;
        scope_frame& peek();
        void push_scope();
        void push_scope(scope_frame&& scope);
        void push_scope(const scope_frame& scope);
        scope_frame pop_scope();
        tess::gc_heap& allocator();
        class execution_state& execution_state();
        bool empty() const;
        int num_frames() const;
        //void get_references(std::unordered_set<tess::obj_id>& objects) const;
    private:
        std::vector<scope_frame> scopes_;
        tess::execution_state& state_;

        evaluation_context(tess::execution_state& es) :
            state_(es)
        {}
    };

    class memoization_tbl
    {
    public:
        void insert(const std::string& key, tess::value_ v);
        bool contains(const std::string& key) const;
        tess::value_ get(const std::string& key) const;
        std::size_t size() const;
    private:
        std::unordered_map<std::string, tess::value_> tbl_;
    };

    class context_stack
    {
    public:
        bool empty() const;
        const evaluation_context& top() const;
        evaluation_context& top();
        void pop();
        void push(evaluation_context&& ctxt);
        memoization_tbl& memos();
    private:
        std::vector<evaluation_context> impl_;
        memoization_tbl memos_;
    };
}