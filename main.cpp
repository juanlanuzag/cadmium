#include <chrono>
#include <iostream>
#include <fstream>
#include <random>



#include <cadmium/modeling/coupled_model.hpp>
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/dynamic_model_translator.hpp>
#include <cadmium/concept/coupled_model_assert.hpp>
#include <cadmium/modeling/dynamic_coupled.hpp>
#include <cadmium/modeling/dynamic_atomic.hpp>
#include <cadmium/engine/pdevs_dynamic_runner.hpp>
#include <cadmium/logger/tuple_to_ostream.hpp>
#include <cadmium/logger/common_loggers.hpp>

#include "./cell_atomic.hpp"
#include "./cell_coupled.hpp"


using TIME = float;
using hclock=std::chrono::high_resolution_clock;

/*************** Loggers *******************/
static std::ofstream out_data("output.txt");
struct oss_sink_provider{
    static std::ostream& sink(){
        return out_data;
    }
};

int main(int argc, char ** argv) {

using info=cadmium::logger::logger<cadmium::logger::logger_info, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
using debug=cadmium::logger::logger<cadmium::logger::logger_debug, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
using state=cadmium::logger::logger<cadmium::logger::logger_state, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
using log_messages=cadmium::logger::logger<cadmium::logger::logger_messages, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
using routing=cadmium::logger::logger<cadmium::logger::logger_message_routing, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
using global_time=cadmium::logger::logger<cadmium::logger::logger_global_time, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
using local_time=cadmium::logger::logger<cadmium::logger::logger_local_time, cadmium::dynamic::logger::formatter<TIME>, oss_sink_provider>;
using log_all=cadmium::logger::multilogger<info, debug, state, log_messages, routing, global_time, local_time>;
using logger_top=cadmium::logger::multilogger<log_messages, global_time>;

std::shared_ptr<cadmium::dynamic::modeling::coupled<TIME>> GRILLA = std::make_shared<cadmium::dynamic::modeling::cell_coupled<TIME>>("Grilla", std::vector<int>({WIDTH, DEPTH}));





///****************////
    auto start = hclock::now();
    auto elapsed1 = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(hclock::now() - start).count();
    std::cout << "Model Created. Elapsed time: " << elapsed1 << "sec" << std::endl;

    cadmium::dynamic::engine::runner<TIME, state> r(GRILLA, {0});
    elapsed1 = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(hclock::now() - start).count();
    std::cout << "Runner Created. Elapsed time: " << elapsed1 << "sec" << std::endl;

    std::cout << "Simulation starts" << std::endl;

    r.run_until(300);
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(hclock::now() - start).count();
    std::cout << "Simulation took:" << elapsed << "sec" << std::endl;
    return 0;
}
