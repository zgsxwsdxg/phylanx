//  Copyright (c) 2016-2017 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_start.hpp>

#include <mutex>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Store the command line arguments in global variables to make them available
// to the startup code.

#if defined(linux) || defined(__linux) || defined(__linux__)

int __argc = 0;
char** __argv = nullptr;

void set_argv_argv(int argc, char* argv[], char* env[])
{
    __argc = argc;
    __argv = argv;
}

__attribute__((section(".init_array")))
    void (*set_global_argc_argv)(int, char*[], char*[]) = &set_argv_argv;

#elif defined(__APPLE__)

#include <crt_externs.h>

inline int get_arraylen(char** argv)
{
    int count = 0;
    if (nullptr != argv)
    {
        while(nullptr != argv[count])
            ++count;
    }
    return count;
}

int __argc = get_arraylen(*_NSGetArgv());
char** __argv = *_NSGetArgv();

#elif defined(WIN32)

#include <boost/program_options.hpp>

#undef __argc
#undef __argv

int __argc = 0;
char** __argv = nullptr;

struct get_command_line
{
    get_command_line()
      : args_(boost::program_options::split_winmain(GetCommandLine()))
    {
        argv_.resize(args_.size() + 1);

        std::size_t argcount = 0;
        for (std::size_t i = 0; i != args_.size(); ++i)
        {
            argv_[argcount++] = const_cast<char*>(args_[i].data());
        }

        // add a single nullptr in the end as some application rely on that
        argv_[argcount] = nullptr;

        __argc = int(args_.size());
        __argv = argv_.data();
    }

    std::vector<std::string> args_;
    std::vector<char*> argv_;
};

get_command_line const& init_command_line()
{
    static get_command_line cmdline;
    return cmdline;
}

#endif

///////////////////////////////////////////////////////////////////////////////
// This class initializes a console instance of HPX (locality 0).
struct manage_global_runtime
{
    manage_global_runtime()
      : running_(false), rts_(nullptr)
    {
#if defined(HPX_WINDOWS)
        hpx::detail::init_winsocket();
        init_command_line();
#endif

        std::vector<std::string> const cfg = {
            // make sure hpx_main is always executed
            "hpx.run_hpx_main!=1",
            // allow for unknown command line options
            "hpx.commandline.allow_unknown!=1",
            // disable HPX' short options
            "hpx.commandline.aliasing!=0",
            // run one thread only (for now)
            "hpx.os_threads!=1"
        };

        using hpx::util::placeholders::_1;
        using hpx::util::placeholders::_2;
        hpx::util::function_nonser<int(int, char**)> start_function =
            hpx::util::bind(&manage_global_runtime::hpx_main, this, _1, _2);

        if (!hpx::start(
                start_function, __argc, __argv, cfg, hpx::runtime_mode_console))
        {
            // Something went wrong while initializing the runtime.
            // This early we can't generate any output, just bail out.
            std::abort();
        }

        // Wait for the main HPX thread (hpx_main below) to have started running
        std::unique_lock<std::mutex> lk(startup_mtx_);
        while (!running_)
            startup_cond_.wait(lk);
    }

    ~manage_global_runtime()
    {
        // notify hpx_main above to tear down the runtime
        {
            std::lock_guard<hpx::lcos::local::spinlock> lk(mtx_);
            rts_ = nullptr;               // reset pointer
        }

        cond_.notify_one();     // signal exit

        // wait for the runtime to exit
        hpx::stop();
    }

    // registration of external (to HPX) threads
    void register_thread(char const* name)
    {
        hpx::register_thread(rts_, name);
    }
    void unregister_thread()
    {
        hpx::unregister_thread(rts_);
    }

protected:
    // Main HPX thread, does nothing but wait for the application to exit
    int hpx_main(int argc, char* argv[])
    {
        // Store a pointer to the runtime here.
        rts_ = hpx::get_runtime_ptr();

        // Signal to constructor that thread has started running.
        {
            std::lock_guard<std::mutex> lk(startup_mtx_);
            running_ = true;
        }

        startup_cond_.notify_one();

        // Here other HPX specific functionality could be invoked...

        // Now, wait for destructor to be called.
        {
            std::unique_lock<hpx::lcos::local::spinlock> lk(mtx_);
            if (rts_ != nullptr)
                cond_.wait(lk);
        }

        // tell the runtime it's ok to exit
        return hpx::finalize();
    }

private:
    hpx::lcos::local::spinlock mtx_;
    hpx::lcos::local::condition_variable_any cond_;

    std::mutex startup_mtx_;
    std::condition_variable startup_cond_;
    bool running_;

    hpx::runtime* rts_;
};

///////////////////////////////////////////////////////////////////////////////
// This global object will initialize HPX in its constructor and make sure HPX
// stops running in its destructor.
manage_global_runtime* rts = nullptr;

void init_hpx_runtime()
{
    rts = new manage_global_runtime;
}

void stop_hpx_runtime()
{
    manage_global_runtime* r = rts;
    rts = nullptr;
    delete r;
}
