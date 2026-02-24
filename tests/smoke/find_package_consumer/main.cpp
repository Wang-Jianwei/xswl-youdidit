#include <xswl/youdidit/youdidit.hpp>

int main() {
    xswl::youdidit::TaskPlatform platform;
    platform.set_name("consumer-smoke");
    return platform.name().empty() ? 1 : 0;
}
