#include <xswl/signals.hpp>
#include <memory>
#include <atomic>
#include <iostream>

int main() {
    using namespace xswl;

    // Test A: shared_ptr member connection automatically skips after reset
    {
        signal_t<int> sig;
        std::atomic<int> called{0};
        struct Receiver {
            std::atomic<int>* counter;
            explicit Receiver(std::atomic<int>* c) : counter(c) {}
            void on_int(int) { counter->fetch_add(1); }
        };

        auto r = std::make_shared<Receiver>(&called);
        sig.connect(r, &Receiver::on_int);

        sig(1);
        if (called.load() != 1) {
            std::cerr << "Shared_ptr connect: expected 1 after first emit, got " << called.load() << std::endl;
            return 1;
        }

        // Reset receiver; subsequent emits must NOT call the handler
        r.reset();
        sig(2);
        if (called.load() != 1) {
            std::cerr << "Shared_ptr lifetime: handler invoked after reset" << std::endl;
            return 1;
        }
    }

    // Test B: scoped_connection_t auto disconnects when leaving scope
    {
        signal_t<> sig;
        std::atomic<int> called{0};
        {
            scoped_connection_t sc = sig.connect([&]() { called.fetch_add(1); });
            sig();
            if (called.load() != 1) {
                std::cerr << "scoped_connection: expected 1 after emit inside scope" << std::endl;
                return 1;
            }
        }
        // After scoped_connection out of scope, emit should not invoke handler
        sig();
        if (called.load() != 1) {
            std::cerr << "scoped_connection: handler was called after scope exit" << std::endl;
            return 1;
        }
    }

    // Test C: raw pointer member connect works while object alive (caller must guarantee lifetime)
    {
        signal_t<> sig;
        std::atomic<int> called{0};
        struct S { std::atomic<int>* c; void on() { c->fetch_add(1); } } s{&called};
        sig.connect(&s, &S::on);
        sig();
        if (called.load() != 1) {
            std::cerr << "raw pointer connect: expected 1 after emit" << std::endl;
            return 1;
        }
        // NOTE: Do not emit after 's' goes out of scope here; calling after destruction is undefined.
    }

    std::cout << "PASSED" << std::endl;
    return 0;
}
