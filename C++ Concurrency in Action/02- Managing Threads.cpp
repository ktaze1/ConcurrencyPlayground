#include <iostream>
#include <thread>

void do_something(int &i); // takes an reference as parameter
void do_something_in_current_thread();

/* 2.1.1 - Launching a thread:
 * 
 * 
 * 
    void do_some_work();
    std::thread my_thread(do_some_work);

    class background_task
    {
    public:
        void operator() () const
        {
            do_something();
            do_something_else();
        }
    };

    background_task f;
    std::thread my_thread(f);

    ---------------


    std::thread my_thread(background_task()); -> This declares
    a my_thread function that takes single parameter and returning std::thread
    object.

    A way to overcome this: std::thread my_thread((background_task())) => extra paranthesis
    OR
    std::thread my_thread{background_task()};
*/

struct func
{ // this will called from oops() from below
    int &i;
    func(int &i_) : i(i_) {}

    void operator()()
    {
        for (unsigned j = 0; j < 1000000; ++j)
            do_something(i); // oops() might finish but do_something still running and has
                             // access to destroyed var. This is Undefined Behaivor!
    }
};

void oops()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread my_thread(my_func);
    my_thread.detach(); // don't wait for thread to finish
} // when going out of scope, new thread might still be running

/** 
 * One solution is to take copy the data rather than sharing by giving reference,
 * If callable object for thread function is used, object is copied.
 * /!\ Beware of objects containing pointers or references
 * 
 * Alternatively, we can ensure that thread has completed execution before the func exits by
 * joining
*/

/* **************************************************************************************** */

/** 2.1.2 - Waiting for a thread to complete:
 * 
 * Replace my_thread.detach() with my_thread.join()
 *
 *  - It's a brute-force technique
 *  - It also cleans up any storage associated with the thread;
 *      so std::thread object is no longer associated with the thread,
 *      so the std::thread object is no longer associated with any thread;
 *  - join() can be called only "once" for a given thread
*/

/* **************************************************************************************** */

/** 2.1.3 Waiting in exceptional circumstances
 * 
 * Be careful where you put the join()
*/

void f()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread t(my_func);

    try
    {
        do_something_in_current_thread();
    }
    catch (const std::exception &e)
    {
        t.join();
        throw e;
    }

    t.join();
}

// Try-catch is verbose. It's not ideal
// If thread must join before function ends, it also must be the same case in every possible senario

// One way to achieve this to use RAII

class thread_guard
{
    std::thread &t;

public:
    explicit thread_guard(std::thread &t_) : t(t_) {}

    ~thread_guard()
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    thread_guard(thread_guard const &) = delete;            // copy constructor
    thread_guard &operator=(thread_guard const &) = delete; // copy assignment
    /* copy constructor and assignment is dangerous because it might outlive 
        the scope of the thread it was joining.*/
};

void f2()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread thread(my_func);
    thread_guard guard(thread);
    do_something_in_current_thread();
}

/* **************************************************************************************** */

/* 2.1.4 Running threads in the background *
* Detached threads truly run in the background; ownership and control are passed over to the
* C++ Runtime Library, which ensures that the resources associated with the thread are correctly
* reclaimed when the thread exits.
*
* Detached threads are often called daemon threads
*/

// Example: detaching a thread to handle other documents

void edit_document(std::string const &filename)
{
    open_document_and_display_gui(filename);

    while (!done_editing())
    {
        user_command cmd = get_user_input();

        if (cmd.type == open_new_document)
        {
            std::string const new_name = get_filename_from_user();
            std::thread thread(edit_document, new_name);
            thread.detach();
        }
        else
        {
            process_user_input();
        }
    }
}

/* **************************************************************************************** */

/** 2.2 Passing arguments to a thread function
 * 
 * by default, arguments are copied
 * 
*/

void f(int i, std::string const &s);
std::thread t(f, 3, "hello"); // this creates thread named t, which calls f(3, "hello")
                              // One thing to notice is that f has string as parameter but
                              // "hello" is a char const* and this is converted to string
                              // only in the context of the new thread

void oops(int some_parameter)
{
    char buffer[1024];
    sprintf(buffer, "%i", some_parameter);
    std::thread thread(f, 3, buffer);               // pointer to the local variable is given.
                                                    // this probably will lead to undefined behavior.
                                                    // convert to std::string
    std::thread thread2(f, 3, std::string(buffer)); // this is okayish
    t.detach();
}

void update_data_for_widget(widget_id w, widget_data &data);

void oops_again(widget_id w)
{
    widget_data data;
    std::thread t(update_data_for_widget, w, data); // 2nd paramater expected to be
        // passed by reference, std::thread constructor doens't know that. Internal
        // code passes copied arguments as rvalues in order to work with move-only
        // types, and will thus try to call update_data_for_widget with an rvalue.
        // This will fail to compile: we can't possible to pass an rvalue to a function
        // that expects a non-const reference. use std::ref()
    std::thread t(update_data_for_widget, w, std::ref(data));

    display_status();
    t.join();
    process_widget_data(data);
}

class X
{
public:
    void do_lengthy_work();
};

X my_x;
std::thread threa(&X::do_lengthy_work, &my_x);

/* **************************************************************************************** */

/***2.3 Transferring ownership of a thread
 * 
 * std::thread is movable but not copyable 
*/

void some_func();
void some_other_func();
std::thread thread1(some_func);           //t1 has some func
std::thread thread2 = std::move(thread1); // t1 = empty, t2 = some func
thread1 = std::thread(some_other_func);   // t1 = some other func, t2 = some func
std::thread thread3;                      // t3 = empty
thread3 = std::move(thread2);             // t1 = some other func, t2 = empty, t3 = some func
thread1 = std::move(thread3);             // std::terminate() is called

// The move support in std::thread means that ownership can readily be transferred out of a function

std::thread f()
{ //return std::thread
    void some_func();
    return std::thread(some_func);
}

std::thread g()
{
    void some_other_func(int);
    std::thread t(some_other_func, 42);
    return t;
}

// it can accept an instance of std::thread by value as paramater

void f(std::thread t);

void g()
{
    void some_func();
    f(std::thread(some_func));
    std::thread t(some_func);
    f(std::move(t));
}

class scoped_thread
{
    std::thread t;

public:
    explicit scoped_thread(std::thread t_) : t(std::move(t_))
    {
        if (!t.joinable())
            throw std::logic_error(“No thread”);
    }
    ~scoped_thread()
    {
        t.join();
    }
    scoped_thread(scoped_thread const &) = delete;
    scoped_thread &operator=(scoped_thread const &) = delete;
};

struct func;

void f()
{
    int some_local_state;
    scoped_thread t{std::thread(func(some_local_state))};
    do_something_in_current_thread();
}

class joining_thread{
    std::thread t;
public:
    joining_thread() noexcept = default;
    template<typename Callable, typename ... Args>
    explicit joining_thread(Callable&& func, Args&& ... args):
        t(std::forward<Callable>(func), std::forward<Args>(args)...)
    {}

    explicit joining_thread(std::thread t_) noexcept:
        t(std::move(t_)) 
    {}

    joining_thread(joining_thread&& other) noexcept:
        t(std::move(other.t))
    {}

    joining_thread& operator=(joining_thread&& other) noexcept {
        if(joinable())
            join();
        t = std::move(other.t);
        return *this;
    }

    joining_thread& operator=(std::thread other) noexcept {
        if(joinable())
            join();
        t = std::move(other);
        return *this;
    } 

    ~joining_thread() noexcept {
        if(joinable())
            join();
    }

    void swap(joining_thread &other) noexcept {
        t.swap(other.t);
    }

    std::thread::id get_id() const noexcept {
        return t.get_id();
    }

    bool joinable() const noexcept {
        return t.joinable();
    }
   
    void join() {
        t.join();
    }
    
    void detach() {
        t.detach();
    }
    
    std::thread &as_thread() noexcept {
        return t;
    }
   
    const std::thread &as_thread() const noexcept {
        return t;
    }
};

void do_work(uint8_t id);

void f() {
    std::vector<std::thread> threads;
    for (uint8_t i = 0; i < 20; ++i)
    {
        threads.emplace_back(do_work, i);
    }
    for (auto &entry : threads)
        entry.join();
}


/* **************************************************************************************** */

/*** 2.4 Choosing the number of threads at runtime 
 * 
 * std::thread::hardware_concurrency() returns an indication of number of threads can truly
 * run concurrently for a given execution of a program. On a multicore system it might be
 * the no of CPU Cores. If there is no info, it returns 0
*/

// A naïve parallel version of std::accumulate

template<typename Iterator, typename T>
struct accumulate_block{
    void operator() (Iterator first, Iterator last, T& result){
        result = std::accumulate(first, last, result);
    }
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init){
    unsigned long const length = std::distance(fist, last);
    
    if(!length)
        return init;
    
    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = 
                (length+min_per_thread-1)/min_per_thread;
    unsigned long const hardware_threads = 
                std::thread::hardware_concurrency();
    unsigned long const num_thread =
                std::min(hardware_threads != 0 ? hardware_threads:2, max_threads);
    unsigned long const block_size = length / num_threads;

    std::vector<T> result(num_threads);
    std::vector<std::thread> threads(num_threads - 1);
    Iterator block_start = first;
    
    for (unsigned long i = 0; i < (num_threads - 1); ++i)
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);
        threads[i] = std::thread(
            accumulate_block<Iterator, T>(),
            block_start, block_end, std::ref(results[i]));
        block_start = block_end;
    }
    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads - 1]);
    for (auto &entry : threads)
        entry.join();
    return std::accumulate(results.begin(), results.end(), init);
}


/**2.5 Identifying threads */
std::thread::id master_thread;
void some_core_part_of_algorithm()
{
    if (std::this_thread::get_id() == master_thread)
    {
        do_master_thread_work();
    }
    do_common_work();
}