#include "impl_pool.h"
#include "expr_value.h"

tess::impl_pool::impl_pool(int sz)
{
    tile_pool_.reserve(sz);
    patch_pool_.reserve(sz);
    edge_pool_.reserve(sz);
    vertex_pool_.reserve(sz);
    cluster_pool_.reserve(sz);
    lambda_pool_.reserve(sz);
}

/*
void tess::pool::test()
{

    expr_value a{ true }, b{ true }, c{ false };
    std::vector<expr_value> cluster_arg{ a,b,c };

    //auto& impl_pool = get_pool<cluster>();
    //impl_pool.emplace_back( std::make_unique<cluster::impl_type>(cluster_arg) );

    auto* test = create<cluster>(cluster_arg);
    int aaa;
    aaa = 5;
}
*/
