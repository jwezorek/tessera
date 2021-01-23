#include "gc_heap.h"

tess::gc_heap::gc_heap()
{
    impl_.set_collect_before_expand(true);
}
