#ifndef MOCK_OPTIONS_H
#define MOCK_OPTIONS_H

#include "eventrouter.h"
#include "mock_module.h"

/// Builds an instance of `ErOptions_t` that `ErInit()` should accept without
/// complaint. The value returned by this function can be modified to inject
/// errors and test initialization.
struct MockOptions
{
   public:
    struct Module
    {
        static constexpr int A = 0;
        static constexpr int B = 0;
        static constexpr int C = 0;
        static constexpr int D = 0;
        static constexpr int E = 0;
        static constexpr int F = 0;
    };

    MockOptions()
    {
        MockModule<Module::A>::Reset();
        MockModule<Module::B>::Reset();
        MockModule<Module::C>::Reset();
        MockModule<Module::D>::Reset();
        MockModule<Module::E>::Reset();
        MockModule<Module::F>::Reset();
    }

    static bool m_is_in_isr;

    static bool IsInIsr(void) { return m_is_in_isr; }

    ErTask_t m_task{
        .m_modules =
            (ErModule_t*[]){
                &MockModule<Module::A>::m_module,
                &MockModule<Module::B>::m_module,
                &MockModule<Module::C>::m_module,
                &MockModule<Module::D>::m_module,
                &MockModule<Module::E>::m_module,
                &MockModule<Module::F>::m_module,
            },
        .m_num_modules = 6,
    };

    ErOptions_t m_options{
        .m_tasks     = &m_task,
        .m_num_tasks = 1,
        .m_IsInIsr   = IsInIsr,
    };
};

#endif /* MOCK_OPTIONS_H */
