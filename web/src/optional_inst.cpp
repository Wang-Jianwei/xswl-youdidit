#include <tl/optional.hpp>
#include <xswl/youdidit/core/types.hpp>

// Force explicit instantiation so the linker gets the symbols for these
// template specializations on toolchains that don't emit them as inline.
template class tl::optional<int>;
template class tl::optional<xswl::youdidit::TaskStatus>;
