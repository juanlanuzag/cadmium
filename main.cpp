#include "./cell_atomic.hpp"
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


// template<typename TIME, typename CELL_ATOMIC>
// class cell_coupled {
//
//     public:
//         cell_coupled
// }
using TIME = float;
using hclock=std::chrono::high_resolution_clock;

std::string get_cell_name(position pos) {
    std::stringstream mname;
    mname << "cell_" << pos.first << "_" << pos.second;
    return mname.str();
}








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



cadmium::dynamic::modeling::Ports iports = {};
cadmium::dynamic::modeling::Ports oports = {};

cadmium::dynamic::modeling::Models cells;
cadmium::dynamic::modeling::ICs ics;

for(int i = 0; i < WIDTH; i++) {
    for(int j = 0; j < DEPTH; j++) {
        position current_pos = std::make_pair(i,j);
        cells.push_back(cadmium::dynamic::translate::make_dynamic_atomic_model<cell_atomic, TIME>(get_cell_name(current_pos), current_pos, current_pos.first==150));

        for (auto pos : vecinos(current_pos)) {
            ics.push_back(
                cadmium::dynamic::translate::make_IC<cell_atomic_defs::out, cell_atomic_defs::in>(get_cell_name(pos), get_cell_name(current_pos))
            );
        }
    }
}



cadmium::dynamic::modeling::EICs eics = {};
cadmium::dynamic::modeling::EOCs eocs = {};

std::shared_ptr<cadmium::dynamic::modeling::coupled<TIME>> GRILLA = std::make_shared<cadmium::dynamic::modeling::coupled<TIME>>(
 "Grilla",
 cells,
 iports,
 oports,
 eics,
 eocs,
 ics
);





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
