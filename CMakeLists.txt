cmake_minimum_required(VERSION 3.16)
project(tessera_project LANGUAGES CXX)

find_package(Eigen3 CONFIG)
if (${Eigen3_FOUND})
   Message(STATUS "eigen found...")
endif (${Eigen3_FOUND})

find_package(Boost 1.7 CONFIG QUIET)
if (${Boost_FOUND})
	Message(STATUS "boost found...")
endif (${Boost_FOUND})

#------------------------ the tessera library ----------------------------

add_library(tessera 
	src/tessera/internal/gc_heap.cpp
	src/tessera/internal/cluster.cpp
	src/tessera/internal/cluster_expr.cpp
	src/tessera/internal/error.cpp
	src/tessera/internal/evaluation_context.cpp
	src/tessera/internal/execution_state.cpp
	src/tessera/internal/expression.cpp
	src/tessera/internal/field_ref.cpp
	src/tessera/internal/function_def.cpp
	src/tessera/internal/geometry.cpp
	src/tessera/internal/lambda.cpp
	src/tessera/internal/lay_expr.cpp
	src/tessera/internal/number.cpp
	src/tessera/internal/object_expr.cpp
	src/tessera/internal/ops.cpp
	src/tessera/internal/script.cpp
	src/tessera/internal/script_impl.cpp
	src/tessera/internal/stack_machine.cpp
	src/tessera/internal/text_range.cpp
	src/tessera/internal/tile.cpp
	src/tessera/internal/tile_def.cpp
	src/tessera/internal/tile_impl.cpp
	src/tessera/internal/tile_patch.cpp
	src/tessera/internal/tile_patch_impl.cpp
	src/tessera/internal/value.cpp
	src/tessera/internal/where_expr.cpp
	src/tessera/internal/with_expr.cpp
	src/tessera/internal/parser/basic_expr_parser.cpp
	src/tessera/internal/parser/cluster_expr_parser.cpp
	src/tessera/internal/parser/cond_expr_parser.cpp
	src/tessera/internal/parser/expr_parser.cpp
	src/tessera/internal/parser/function_parser.cpp
	src/tessera/internal/parser/keywords.cpp
	src/tessera/internal/parser/lay_expr_parser.cpp
	src/tessera/internal/parser/object_expr_parser.cpp
	src/tessera/internal/parser/script_parser.cpp
	src/tessera/internal/parser/skipper.cpp
	src/tessera/internal/parser/special_parser.cpp
	src/tessera/internal/parser/where_parser.cpp
	src/tessera/internal/parser/with_parser.cpp
)

target_include_directories(tessera
	PRIVATE ${EIGEN3_INCLUDE_DIR}
	PRIVATE ${Boost_INCLUDE_DIRS}
	PRIVATE src/tessera/internal/third-party/gc/include
	PRIVATE src/tessera/internal/third-party/gsl/include
	PUBLIC src/tessera/include
)

set_target_properties(tessera
    PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
)

#----------------------- the cmd line exe -------------------------------

add_executable( tessera_workbench
	src/tessera_workbench/tessera_workbench.cpp
)

target_include_directories(tessera_workbench
	PRIVATE src/tessera/include
)

target_link_libraries( tessera_workbench
	PRIVATE tessera
)

set_target_properties( tessera_workbench
    PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
)


