#include "patch.h"

tess::tile_patch::tile_patch(const std::string& name, std::vector<std::string> params, const text_range& source_code) :
    name_(name), params_(params), code_(source_code)
{
}
