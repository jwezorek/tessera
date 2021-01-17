cmake_minimum_required(VERSION 3.16)
project(tessera_project LANGUAGES CXX)

find_package(Eigen3 CONFIG REQUIRED)
if (${Eigen3_FOUND})
   Message(STATUS "eigen found...")
endif (${Eigen3_FOUND})

find_package(Boost 1.7 CONFIG REQUIRED QUIET)
if (${Boost_FOUND})
	Message(STATUS "boost found...")
endif (${Boost_FOUND})

#------------------------ the tessera library ----------------------------

add_library(tessera 
	tessera/internal/allocator.cpp
	tessera/internal/cluster.cpp
	tessera/internal/cluster_expr.cpp
	tessera/internal/error.cpp
	tessera/internal/evaluation_context.cpp
	tessera/internal/execution_state.cpp
	tessera/internal/expression.cpp
	tessera/internal/field_ref.cpp
	tessera/internal/function_def.cpp
	tessera/internal/geometry.cpp
	tessera/internal/lambda.cpp
	tessera/internal/lay_expr.cpp
	tessera/internal/number.cpp
	tessera/internal/object_expr.cpp
	tessera/internal/ops.cpp
	tessera/internal/script.cpp
	tessera/internal/script_impl.cpp
	tessera/internal/stack_machine.cpp
	tessera/internal/text_range.cpp
	tessera/internal/tile.cpp
	tessera/internal/tile_def.cpp
	tessera/internal/tile_impl.cpp
	tessera/internal/tile_patch.cpp
	tessera/internal/tile_patch_impl.cpp
	tessera/internal/value.cpp
	tessera/internal/where_expr.cpp
	tessera/internal/with_expr.cpp
	tessera/internal/parser/basic_expr_parser.cpp
	tessera/internal/parser/cluster_expr_parser.cpp
	tessera/internal/parser/cond_expr_parser.cpp
	tessera/internal/parser/expr_parser.cpp
	tessera/internal/parser/function_parser.cpp
	tessera/internal/parser/keywords.cpp
	tessera/internal/parser/lay_expr_parser.cpp
	tessera/internal/parser/object_expr_parser.cpp
	tessera/internal/parser/script_parser.cpp
	tessera/internal/parser/skipper.cpp
	tessera/internal/parser/special_parser.cpp
	tessera/internal/parser/where_parser.cpp
	tessera/internal/parser/with_parser.cpp
)

target_include_directories(tessera
	PRIVATE ${EIGEN3_INCLUDE_DIR}
	PRIVATE ${Boost_INCLUDE_DIRS}
	PRIVATE tessera/internal/third-party/gc/include
	PRIVATE tessera/internal/third-party/gsl/include
	PUBLIC tessera/include
)

set_target_properties(tessera
    PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
)

#----------------------- the cmd line exe -------------------------------

add_executable( tessera_workbench
	tessera_workbench/tessera_workbench.cpp
)

target_include_directories(tessera_workbench
	PRIVATE tessera/include
)

target_link_libraries( tessera_workbench
	PRIVATE tessera
)

set_target_properties( tessera_workbench
    PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
)


