#pragma once

#include "tile_def.h"
#include "tile_patch_def.h"
#include "expr_value.h"
#include "tableau_def.h"
#include <optional>
#include <unordered_map>
#include <memory>


namespace tess {

	class tessera_script::script_impl {
	private:
		std::unordered_map<std::string, tile_def> tiles_;
		std::unordered_map<std::string, std::shared_ptr<tile_def>> tiles_prototypes_;
		std::unordered_map<std::string, tile_patch_def> patches_;
		std::unordered_map<std::string, expr_value> globals_;
		tableau_def tableau_;
	public:
		void insert_tile_def(const std::string& name, std::vector<std::string> params, const text_range& source_code);
		void insert_patch_def(const std::string& name, std::vector<std::string> params, const text_range& source_code);
		void insert_tableau_def(std::vector<std::string> params, const text_range& source_code);
		void insert_globals(execution_ctxt& ctxt, global_vars global_defs);
		void build_tiles(execution_ctxt& ctxt);
		std::optional<expr_value> get_global(const std::string& var) const;
		std::optional<std::variant<tile_def, tile_patch_def>> get_functional(const std::string& var) const;
	};

}