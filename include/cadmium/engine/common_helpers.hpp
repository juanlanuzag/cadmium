/**
 * Copyright (c) 2018, Laouen M. L. Belloli
 * Carleton University, Universidad de Buenos Aires
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CADMIUM_COMMON_HELPERS_HPP
#define CADMIUM_COMMON_HELPERS_HPP

#include <tuple>
#include <vector>
#include <sstream>

#define BOOST_THREAD_PROVIDES_FUTURE
#include <boost/thread/executors/basic_thread_pool.hpp>
#include <boost/thread/future.hpp>

namespace cadmium {
    namespace helper {

        //Generic tuple for_each function
        template<typename TUPLE, typename FUNC>
        void for_each(TUPLE& ts, FUNC&& f) {

            auto for_each_fold_expression = [&f](auto &... e)->void { (f(e) , ...); };
            std::apply(for_each_fold_expression, ts);
        }

        /*
         * for_each that runs using a thread_pool (assumed without running tasks),
         * and waits por all tasks to finish until it returns
         */
        template<typename ITERATOR, typename FUNC>
        void concurrent_for_each(boost::basic_thread_pool& threadpool, ITERATOR first, ITERATOR last, FUNC& f) {
          std::vector<boost::future<void> > task_statuses;

          for (; first != last; ++first) {
            boost::packaged_task<void> task(boost::bind<void>(f, *first));
            task_statuses.push_back(task.get_future());

            threadpool.submit(std::move(task));
          }
          auto wait_until_done = [](auto &t)->void { t.wait(); };
          std::for_each(task_statuses.begin(), task_statuses.end(), wait_until_done);
        }

        std::string join(std::vector<std::string> v) {
            std::ostringstream oss;
            oss << "{";
            auto it = v.begin();
            if (it != v.end()) {
                oss << *it;
                ++it;
            }
            while (it != v.end()){
                oss << ", ";
                oss << *it;
                ++it;
            }
            oss << "}";
            return oss.str();
        }

    }
}

#endif //CADMIUM_COMMON_HELPERS_HPP
