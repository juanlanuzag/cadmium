
#include <tuple>
#include <queue>
#include <cassert>
#include <iomanip>
#include <functional>
#include <iostream>
#include <fstream>
#include <cadmium/modeling/ports.hpp>
#include <cadmium/modeling/message_bag.hpp>
#include <bits/unordered_map.h>

#include "./cell_helpers.hpp"


using VALUE = bool;
using msg_type = std::pair<position, VALUE>;

#define WIDTH 300
#define DEPTH 1

#define DURATION 1

position up_left (position& pos) {
    return {
            (pos[0] - 1) % WIDTH + (pos[0] - 1 < 0 ? WIDTH : 0),
            (pos[1] + 1) % DEPTH
    };
}
position up_right (position& pos) {
    return {
            (pos[0] + 1) % WIDTH,
            (pos[1] + 1) % DEPTH
    };
}
position up (position& pos) {
    return {
            (pos[0]) % WIDTH,
            (pos[1] + 1) % DEPTH
    };
}

struct cell_atomic_defs{
    //custom ports
    struct in : public cadmium::in_port<msg_type> {};
    struct out : public cadmium::out_port<msg_type> {};
};


template<typename TIME>
class cell_atomic {
    using defs=cell_atomic_defs;

    public:
        struct state_type {
            TIME next_internal;
            VALUE cell_state;
            std::priority_queue<std::pair<TIME, VALUE>,std::vector<std::pair<TIME, VALUE>>, std::greater<std::pair<TIME, VALUE>> > delayed_outputs;
        };
        std::map<position, VALUE> neighbor_values; //Neighbor positions are relative
        state_type state;
        position my_position;

        using input_ports=std::tuple<typename defs::in>;
        using output_ports=std::tuple<typename defs::out>;

        void update_state() {
            VALUE up_left = neighbor_values[position({-1,1})];
            VALUE up = neighbor_values[position({0,1})];
            VALUE up_right = neighbor_values[position({1,1})];

            if (up_left && up && up_right) {
                state.cell_state = false;
            } else if (up_left &&  up && !up_right) {
                state.cell_state = true;
            } else if (up_left && !up && up_right) {
                state.cell_state = true;
            } else if (!up_left && up && up_right) {
                state.cell_state = true;
            } else if (!up_left && !up && up_right) {
                state.cell_state = true;
            } else if (!up_left && up && !up_right) {
                state.cell_state = true;
            } else if (up_left && !up && !up_right) {
                state.cell_state = false;
            } else if (!up_left && !up && !up_right) {
                state.cell_state = false;
            }
        }


        //USER DEFINED
        // virtual std::tuple<TIME, VALUE> local_comupation_function(TIME e, typename make_message_bags<input_ports>::type mbs) = 0;
        // virtual std::tuple<TIME, VALUE> local_comupation_confluence_function(TIME e, typename make_message_bags<input_ports>::type mbs) = 0;
        std::pair<TIME, VALUE> local_comupation_function(TIME e, typename cadmium::make_message_bags<input_ports>::type mbs) {
            for (auto msg : cadmium::get_messages<typename defs::in>(mbs)) {
                position rel = absolute_to_relative(my_position, msg.first);
                neighbor_values[rel] = msg.second;
            }
            update_state();
            return std::make_pair(DURATION, state.cell_state);
        };

        constexpr cell_atomic() noexcept {}


        cell_atomic(position _pos, VALUE initial, std::vector<position> rel_neighborhood) {
            my_position = _pos;
            state.cell_state = initial;
            state.delayed_outputs.push(std::make_pair(1, state.cell_state));
            state.next_internal = 1;

            std::for_each(rel_neighborhood.begin(),
                          rel_neighborhood.end(),
                          [this](auto rel) {neighbor_values.insert(std::make_pair(rel, true)); }
            );

        }

        void internal_transition() {
            state.delayed_outputs.pop();
        }

        void external_transition(TIME e, typename cadmium::make_message_bags<input_ports>::type mbs) {
            auto computed_output = local_comupation_function(e, mbs);
            state.delayed_outputs.push(computed_output);
            state.next_internal = state.delayed_outputs.top().first;
        }

        void confluence_transition(TIME e, typename cadmium::make_message_bags<input_ports>::type mbs) {
            internal_transition();
            external_transition(e, mbs);
        }

        typename cadmium::make_message_bags<output_ports>::type output() const {
            typename cadmium::make_message_bags<output_ports>::type bags;

            std::pair<TIME, VALUE> out = state.delayed_outputs.top();
            cadmium::get_messages<typename defs::out>(bags).push_back(std::make_pair(my_position, out.second));
            return bags;

        }

        TIME time_advance() const {
            return state.next_internal;
        }


        friend std::ostringstream& operator<<(std::ostringstream& os, const typename cell_atomic<TIME>::state_type& i) {
            os << i.cell_state;
            return os;
        }
};
