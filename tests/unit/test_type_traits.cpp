#include <xswl/youdidit/core/types.hpp>
#include <type_traits>

using namespace xswl::youdidit;

static_assert(std::is_copy_constructible<Error>::value, "Error must be copy-constructible");
static_assert(std::is_move_constructible<Error>::value, "Error must be move-constructible");
static_assert(std::is_copy_assignable<Error>::value, "Error must be copy-assignable");
static_assert(std::is_move_assignable<Error>::value, "Error must be move-assignable");

static_assert(std::is_copy_constructible<TaskResult>::value, "TaskResult must be copy-constructible");
static_assert(std::is_move_constructible<TaskResult>::value, "TaskResult must be move-constructible");
static_assert(std::is_copy_assignable<TaskResult>::value, "TaskResult must be copy-assignable");
static_assert(std::is_move_assignable<TaskResult>::value, "TaskResult must be move-assignable");

int main() { return 0; }
