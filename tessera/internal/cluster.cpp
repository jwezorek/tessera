#include "cluster.h"
#include "tile_patch_impl.h"
#include "expr_value.h"

tess::cluster::cluster(const std::vector<expr_value>& values) :
    impl_(std::make_shared<impl_type>(values))
{
}
