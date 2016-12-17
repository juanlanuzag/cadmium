/**
 * Copyright (c) 2013-2016, Damian Vicino
 * Carleton University, Universite de Nice-Sophia Antipolis
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


#ifndef PDEVS_ENGINE_HELPERS_HPP
#define PDEVS_ENGINE_HELPERS_HPP

#include<type_traits>
#include<tuple>
#include<cadmium/engine/pdevs_simulator.hpp>
#include<cadmium/engine/pdevs_coordinator.hpp>
#include<cadmium/concept/concept_helpers.hpp>


namespace cadmium {
    namespace engine {
        //forward declaration
        template<template<typename T> class MODEL, typename TIME>
        class coordinator;

        //finding the min next from a tuple of coordinators and simulators
        template<typename T, std::size_t S>
        struct min_next_in_tuple_impl {
            static auto value(T& t) {
                return std::min(std::get<S - 1>(t).next(), min_next_in_tuple_impl<T, S - 1>::value(t));
            }
        };

        template<typename T>
        struct min_next_in_tuple_impl<T, 1> {
            static auto value(T& t) {
                return std::get<0>(t).next();
            }
        };

        template<typename T>
        auto min_next_in_tuple(T& t) {
            return min_next_in_tuple_impl<T, std::tuple_size<T>::value>::value(t);
        }

        //We use COS to accumulate coordinators and simulators while iterating MT using the S index
        template<typename TIME, template<typename> class MT, std::size_t S, typename... COS> //COS accumulates coords or sims
        struct coordinate_tuple_impl {
            template<typename T>
            using current=typename std::tuple_element<S - 1, MT<T>>::type;
            using current_coordinated=typename std::conditional<cadmium::concept::is_atomic<current>::value(), simulator<current, TIME>, coordinator<current, TIME>>::type;
            using type=typename coordinate_tuple_impl<TIME, MT, S - 1, current_coordinated, COS...>::type;
        };

        //When the S reaches 0, all coordinators and simulators are put into a tuple for return
        template<typename TIME, template<typename> class MT, typename... COS>
        struct coordinate_tuple_impl<TIME, MT, 0, COS...> {
            using type=std::tuple<COS...>;
        };

        template<typename TIME, template<typename> class MT>
        struct coordinate_tuple {
            //the size should not be affected by the type used for TIME, simplifying passing float
            using type=typename coordinate_tuple_impl<TIME, MT, std::tuple_size<MT<float>>::value>::type;
        };

        //init every subcooridnator in the coordination tuple
        template <typename TIME, typename  CST, std::size_t S>
        struct init_subcoordinators_impl{
            static void value(const TIME& t, CST& cs){
                std::get<S-1>(cs).init(t);
                init_subcoordinators_impl<TIME, CST, S-1>::value(t, cs);
                return;
            }
        };

        template <typename TIME, typename  CST>
        struct init_subcoordinators_impl<TIME, CST, 1>{
            static void value(const TIME& t, CST& cs){
                std::get<0>(cs).init(t);
                return;
            }
        };

        template<typename TIME, typename CST>
        void init_subcoordinators(const TIME& t, CST& cs ) {
            init_subcoordinators_impl<TIME, CST, std::tuple_size<CST>::value>::value(t, cs);
        }

        //populate the outbox of every subcoordinator recursively
        template<typename TIME, typename CST, std::size_t S>
        struct collect_outputs_in_subcoordinators_impl{
            static void run(const TIME& t, CST& cs){
                std::get<S-1>(cs).collect_outputs(t);
            }
        };

        template<typename TIME, typename CST>
        struct collect_outputs_in_subcoordinators_impl<TIME, CST, 0>{
            static void run(const TIME& t, CST& cs){
                std::get<0>(cs).collect_outputs(t);
            }
        };

        template<typename TIME, typename CST>
        void collect_outputs_in_subcoordinators(const TIME& t, CST& cs){
            collect_outputs_in_subcoordinators_impl<TIME, CST, std::tuple_size<CST>::value>::run(t, cs);
        }

        //get the engine  from a tuple of engines that is simulating the model provided
        template<typename MODEL, typename CST, std::size_t S>
        struct get_engine_by_model_impl{
            using found_element_type =  typename std::conditional<
                                            std::is_same<typename std::tuple_element<S-1, CST>::type::model_type, MODEL>::value,
                                            typename std::tuple_element<S-1, CST>::type,
                                            get_engine_by_model_impl<MODEL, CST, S-1 >
                                        >::type;
        };

        template<typename MODEL, typename CST>
        auto get_engine_by_model(CST& cst){
            using found_element_type=typename get_engine_by_model_impl<MODEL, CST, std::tuple_size<CST>::value>::found_element_type;
            return std::get<found_element_type>(cst);
        };

        //map the messages in the outboxes of subengines to the messages in the outbox of current coordinator
        template<typename TIME, typename EOC, std::size_t S, typename OUT_BAG, typename CST>
        struct collect_messages_by_eoc_impl{
            using external_output_port=typename std::tuple_element<S-1, EOC>::type::external_output_port;
            using submodel_from = typename std::tuple_element<S-1, EOC>::type::template submodel<TIME>;
            using submodel_output_port=typename std::tuple_element<S-1, EOC>::type::submodel_output_port;
            using submodel_out_messages_type=boost::optional<typename make_message_bags<typename std::tuple<submodel_output_port>>::type>;

            static void fill(OUT_BAG& messages, CST& cst){
                //precess one coupling
                auto& to_messages = get_messages<external_output_port>(messages);
                if (submodel_out_messages_type from_messages_opt = get_engine_by_model<submodel_from, CST>(cst).outbox()){
                    auto& from_messages = get_messages<submodel_output_port>(*from_messages_opt);
                    to_messages.insert(to_messages.end(), from_messages.begin(), from_messages.end());
                }
                //recurse
                collect_messages_by_eoc_impl<TIME, EOC, S-1, OUT_BAG, CST>::fill(messages, cst);
            }
        };

        template<typename TIME, typename EOC, typename OUT_BAG, typename CST>
        struct collect_messages_by_eoc_impl<TIME, EOC, 0, OUT_BAG, CST>{
            static void fill(OUT_BAG& messages, CST& cst){} //nothing to do here
        };

        template<typename TIME, typename EOC, typename OUT_BAG, typename CST>
        OUT_BAG collect_messages_by_eoc(CST& cst){
            OUT_BAG ret;
            collect_messages_by_eoc_impl<TIME, EOC, std::tuple_size<EOC>::value, OUT_BAG, CST>::fill(ret, cst);
            return ret;
        }

        //advance the simulation in every subengine
        template <typename TIME, typename CST, std::size_t S>
        struct advance_simulation_in_subengines_impl{
            static void run(const TIME& t, CST& cst){
                std::get<S-1>(cst).advance_simulation(t);
            }
        };

        template <typename TIME, typename CST>
        struct advance_simulation_in_subengines_impl<TIME, CST, 0>{
            static void run(const TIME& t, CST& cst){
                //nothing to do here
            }
        };

        template <typename TIME, typename CST>
        void advance_simulation_in_subengines(const TIME& t, CST& subcoordinators){
            advance_simulation_in_subengines_impl<TIME, CST, std::tuple_size<CST>::value>::run(t, subcoordinators);
            return;
        }


        //route messages following ICs
        template<typename TIME, typename CST, typename ICs, std::size_t S>
        struct route_internal_coupled_messages_on_subcoordinators_impl{
            using current_IC=typename std::tuple_element<S-1, ICs>::type;
            using from_model=typename current_IC::template from_model<TIME>;
            using from_port=typename current_IC::from_model_output_port;
            using to_model=typename current_IC::template to_model<TIME>;
            using to_port=typename current_IC::to_model_input_port;

            static void route(const TIME& t, CST& engines){
                //route messages for 1 coupling
                auto from_engine=get_engine_by_model<from_model, CST>(engines);
                auto to_engine=get_engine_by_model<to_model, CST>(engines);
                //if outbox empty
                //if inbox empty assign

                //if inbox not empty concatanate
                //cadmium::get_messages<to_port>(to_engine)

                //recurse
                route_internal_coupled_messages_on_subcoordinators_impl<TIME, CST, ICs, S-1>::route(t, engines);
            }
        };

        template<typename TIME, typename CST, typename ICs>
        struct route_internal_coupled_messages_on_subcoordinators_impl<TIME, CST, ICs, 0>{
            static void route(const TIME& t, CST& subcoordinators){
            //nothing to do here
            }
        };

        template <typename TIME, typename CST, typename ICs >
        void route_internal_coupled_messages_on_subcoordinators(const TIME& t, CST& cst){
            route_internal_coupled_messages_on_subcoordinators_impl<TIME, CST, ICs, std::tuple_size<ICs>::value>::route(t, cst);
            return;
        };
    }

}
#endif // PDEVS_ENGINE_HELPERS_HPP
