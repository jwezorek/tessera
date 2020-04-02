#pragma once

#include "tile_def.h"
#include "tile_patch_def.h"
#include "tessera_impl.h"
#include "expr_value.h"
#include "execution_ctxt.h"
#include <optional>
#include <unordered_map>
#include <memory>

namespace tess {

	class script_impl : public tessera_impl  {
	private:
		std::unordered_map<std::string, std::shared_ptr<const tile_def>> tiles_;
		std::unordered_map<std::string, tile_patch_def> patches_;
		std::unordered_map<std::string, expr_value> globals_;
	public:
		void insert_tile_def(const std::string& name, std::vector<std::string> params, const text_range& source_code);
		void insert_patch_def(const std::string& name, std::vector<std::string> params, const text_range& source_code);
		void insert_tableau_def(std::vector<std::string> params, const text_range& source_code);
		void insert_globals(execution_ctxt& ctxt, global_vars global_defs);
		std::optional<expr_value> get_global(const std::string& var) const;
		std::optional<std::variant<tile_def, tile_patch_def>> get_functional(const std::string& var) const;
		std::shared_ptr<const tile_def> get_tile_prototype(const std::string& name) const;
		const tile_patch_def& tableau() const;
		std::vector<tess::tile> execute(execution_ctxt& ctxt) const;
	};

}