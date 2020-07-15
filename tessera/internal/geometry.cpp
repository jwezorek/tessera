#include "boost/functional/hash.hpp"
#include "geometry.h"

std::size_t tess::edge_hash::operator()(const edge_indices& key) const
{
    return boost::hash_value(key);
}
