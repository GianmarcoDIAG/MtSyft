/*
*
* Source code of class EnvironmentsChainBestEffortSynthesizer
* Implements LTLf best-effort synthesis for a chain of environments
*
*/

#include"EnvironmentsChainBestEffortSynthesizer.h"

namespace Syft {

    EnvironmentsChainBestEffortSynthesizer::EnvironmentsChainBestEffortSynthesizer(std::shared_ptr<VarMgr> var_mgr, std::string ltlf_goal, std::vector<std::string> ltlf_envs, Syft::InputOutputPartition partition, Syft::Player starting_player) : 
        var_mgr_(var_mgr),
        ltlf_goal_(ltlf_goal),
        ltlf_envs_(ltlf_envs),
        partition_(partition),
        starting_player_(starting_player) {

            // debug
            // std::cout << "Agent goal: " << ltlf_goal_ << std::endl;
            // std::cout << "Env assumptions:" << std::endl;
            // for (const auto& env: ltlf_envs_) {
            //     std::cout << env << std::endl;
            // }  
            // std::cout << std::endl;

            // this version constructs entire game arena
            Syft::Stopwatch ltlf2dfas;
            ltlf2dfas.start();

            std::vector<double> ltlf2dfa_times;

            Syft::Stopwatch goal2dfa;
            goal2dfa.start();
            ExplicitStateDfaMona mona_goal_dfa =
                ExplicitStateDfaMona::dfa_of_formula(ltlf_goal_);
            ltlf2dfa_times.push_back(goal2dfa.stop().count() / 1000.0);
            
            std::vector<ExplicitStateDfaMona> mona_env_dfas; // leftmost is the most determinate. Rightmost the less determinate
            for (const auto& env: ltlf_envs_) {
                Syft::Stopwatch env2dfa;
                env2dfa.start();
                mona_env_dfas.push_back(ExplicitStateDfaMona::dfa_of_formula(env));            
                ltlf2dfa_times.push_back(env2dfa.stop().count() / 1000.0);
            }

            Syft::Stopwatch tau2dfa;
            tau2dfa.start();
            ExplicitStateDfaMona mona_no_empty_dfa =
                ExplicitStateDfaMona::dfa_of_formula("F(true)"); // accepts non-empty traces only
            double t_tau2dfa = tau2dfa.stop().count() / 1000.0;

            // debug
            // std::cout << "No-empty traces DFA: ";
            // mona_no_empty_dfa.dfa_print();
            // std::cout << std::endl;

            run_times_.push_back(ltlf2dfas.stop().count() / 1000.0);

            std::cout << "[MtSyft] converting LTLf into DFA...DONE (" << run_times_[0] << " s)" << std::endl;

            std::cout << "\t[MtSyft] goal to DFA in " << ltlf2dfa_times[0] << " s" << std::endl;
            for (int i = 1; i < ltlf2dfa_times.size(); ++i) {
                std::cout << "\t[MtSyft] environment " << i << " to DFA in " << ltlf2dfa_times[i] << " s" << std::endl;
            }
            std::cout << "\t[MtSyft] tautology to DFA in " << t_tau2dfa << " s" << std::endl;

            // debug
            // mona_goal_dfa.dfa_print();
            // std::cout << std::endl;
            // for (int i = 0; i < mona_env_dfas.size(); ++i) {
            //     mona_env_dfas[i].dfa_print();
            //     std::cout << std::endl;
            // }
            // std::cout << std::endl;


            // we need a formula which includes all variables of the problems
            // could be anything. We simply conjunct goal and all env specs

            Syft::Stopwatch pre;
            pre.start();
            std::cout << "[MtSyft] preprocessing..." ;

            // old prepreocessing code...
            // std::string conjunct_formula = "(" + ltlf_goal_ + ")";
            // for (const auto& env: ltlf_envs_) {
            //     if (env != "tt") conjunct_formula += " && (" + env + ")";
            // }

            // formula parsed_conjunct_formula =
            //     parse_formula(conjunct_formula.c_str());

            // construct partition
            // var_mgr_->create_named_variables(get_props(parsed_conjunct_formula));
            
            // Rewrites the preprocessing phase to not use the SPOT parser
            // which does not interact well with Lydia (does not support Lydia's X[!])
            var_mgr_->create_named_variables(mona_goal_dfa.names);
            for (const auto& env_dfa : mona_env_dfas) var_mgr_->create_named_variables(env_dfa.names);

            var_mgr_->partition_variables(partition_.input_variables,
                                        partition_.output_variables);

            std::cout << "DONE (" << pre.stop().count() / 1000.0 << " s)" << std::endl;

            std::cout << "[MtSyft] converting to symbolic DFA...";
            Syft::Stopwatch dfa2symbolic;
            dfa2symbolic.start();

            Syft::Stopwatch goal2sym;
            goal2sym.start();

            ExplicitStateDfa goal_dfa =
                ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_goal_dfa);
            SymbolicStateDfa goal_sym_dfa =
                SymbolicStateDfa::from_explicit(std::move(goal_dfa));

            // add goal symbolic DFA to all products for game arenas
            for (int i = 0; i < mona_env_dfas.size(); ++i) {
                symbolic_dfas_.emplace_back();
                symbolic_dfas_[i].push_back(goal_sym_dfa);
            }

            double t_goal2sym = goal2sym.stop().count() / 1000.0;

            std::vector<double> t_env2sym;

            std::vector<ExplicitStateDfa> env_dfas;

            for (int i = 0; i < mona_env_dfas.size(); ++i) {
                Syft::Stopwatch env2sym;
                env2sym.start();

                env_dfas.push_back(ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_env_dfas[i]));
                symbolic_dfas_[i].push_back(SymbolicStateDfa::from_explicit(std::move(env_dfas[i]))); // add each E_i to the corresponding product vector

                t_env2sym.push_back(env2sym.stop().count() / 1000.0);
            }

            Syft::Stopwatch tau2sym;
            tau2sym.start();

            // tautology symbolic DFA
            ExplicitStateDfa no_empty_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_no_empty_dfa);
            SymbolicStateDfa tau_sym_dfa = SymbolicStateDfa::from_explicit(std::move(no_empty_dfa));

            // add tautology to the product forming each arena
            for (int i = 0; i < mona_env_dfas.size(); ++i) {
                symbolic_dfas_[i].push_back(tau_sym_dfa);
            }

            double t_tau2sym = tau2sym.stop().count() / 1000.0;
            
            // construct all game arenas
            Syft::Stopwatch arena_time;
            arena_time.start();

            std::vector<double> t_arena;

            for (int i = 0; i < symbolic_dfas_.size(); ++i) { // for all defined products
                Syft::Stopwatch arena;
                arena.start();
                arena_.push_back(SymbolicStateDfa::product(symbolic_dfas_[i]));
                t_arena.push_back(arena.stop().count() / 1000.0);
            }

            double arena_stop = arena_time.stop().count() / 1000.0;
            run_times_.push_back(dfa2symbolic.stop().count() / 1000.0);

            std::cout << "DONE (" << run_times_[1] << " s)" << std::endl;
            std::cout << "\t[MtSyft] goal DFA to symbolic in " << t_goal2sym << " s" << std::endl;
            for (int i = 0; i < t_env2sym.size(); ++i) {
                std::cout << "\t[MtSyft] environment " << i+1 << " to symbolic in " << t_env2sym[i] << " s" << std::endl;
            }
            std::cout << "\t[MtSyft] tautology DFA to symbolic in " << t_tau2sym << " s" << std::endl;
            std::cout << "\t[MtSyft] symbolic arenas constructed in " << arena_stop << " s" << std::endl;
            for (int i = 0; i < t_arena.size(); ++i) {
                std::cout << "\t\t[MtSyft] arena " << i+1 << " constructed in " << t_arena[i] << " s" << std::endl;
            }
    }

    // std::pair<std::vector<SynthesisResult>, std::vector<SynthesisResult>> EnvironmentsChainBestEffortSynthesizer::run() {
    ChainedStrategies EnvironmentsChainBestEffortSynthesizer::run() {
    
            std::cout << "[MtSyft] constructing and solving games...";
            ChainedStrategies result;

            Syft::Stopwatch adv_games;
            adv_games.start();

            // adversarial games
            std::vector<double> t_adv_games;
            for (int i = 0; i < arena_.size(); ++i) {
                Syft::Stopwatch adv_game;
                adv_game.start();
                CUDD::BDD adversarial_goal = ((!symbolic_dfas_[i][1].final_states()) + symbolic_dfas_[i][0].final_states()) * (!arena_[i].initial_state_bdd());
                ReachabilitySynthesizer adversarial_synthesizer(
                    arena_[i],
                    starting_player_,
                    Player::Agent,
                    adversarial_goal,
                    var_mgr_->cudd_mgr()->bddOne());
                result.adversarial_results.push_back(adversarial_synthesizer.run());
                t_adv_games.push_back(adv_game.stop().count() / 1000.0);
                }

            run_times_.push_back(adv_games.stop().count() / 1000.0);

            // cooperative games
            Syft::Stopwatch coop_games;
            coop_games.start();

            std::vector<double> t_coop_games;
            for (int i = 0; i < arena_.size(); ++i) {
                Syft::Stopwatch coop_game;
                coop_game.start();
                // restriction
                CUDD::BDD negated_env_goal = (!symbolic_dfas_[i][1].final_states()) * (!arena_[i].initial_state_bdd());
                ReachabilitySynthesizer negated_environment_synthesizer(
                    arena_[i],
                    starting_player_,
                    Player::Agent,
                    negated_env_goal,
                    var_mgr_->cudd_mgr()->bddOne()
                );
                SynthesisResult environment_result = negated_environment_synthesizer.run();
                CUDD::BDD non_environment_winning_region = environment_result.winning_states;
                restricted_arena_.push_back(arena_[i].get_restriction(non_environment_winning_region));

                // cooperation
                CUDD::BDD cooperative_goal = symbolic_dfas_[i][1].final_states() * symbolic_dfas_[i][0].final_states();
                CoOperativeReachabilitySynthesizer co_operative_reachability_synthesizer(
                    restricted_arena_[i],
                    starting_player_,
                    Player::Agent,
                    cooperative_goal,
                    var_mgr_->cudd_mgr()->bddOne()
                );
                result.cooperative_results.push_back(co_operative_reachability_synthesizer.run());
                t_coop_games.push_back(coop_game.stop().count() / 1000.0);
            }

            run_times_.push_back(coop_games.stop().count() / 1000.0);
            std::cout << "DONE (" << run_times_[2] + run_times_[3] << " s)"  << std::endl;
            for (int i = 0; i < t_adv_games.size(); ++i)
                std::cout << "\t[MtSyft] adv game in env " << i + 1 << " solved in " << t_adv_games[i] << " s" << std::endl;
            for (int i = 0; i < t_coop_games.size(); ++i)
                std::cout << "\t[MtSyft] coop game in env " << i + 1 << " solved in " << t_coop_games[i] << " s" << std::endl;

            return result;
    }


    void EnvironmentsChainBestEffortSynthesizer::dump_chained_strategies(const std::vector<std::vector<CUDD::BDD>>& chained_strategies) const {

        std::cout << "[MtSyft] dumping dot...";

        std::vector<std::string> output_labels = var_mgr_->output_variable_labels();
        
        int i = 1;
        for (const auto& strategy: chained_strategies) {
            std::vector<CUDD::ADD> add_strategy;
            for (const auto& bdd: strategy) {
                add_strategy.push_back(bdd.Add());
            }
            var_mgr_->dump_dot(add_strategy, output_labels, "positional_strategy_" + std::to_string(i) + ".dot");
            ++i;
        }

        std::cout << "DONE" << std::endl;

    }

    void EnvironmentsChainBestEffortSynthesizer::realizability(const std::vector<Syft::SynthesisResult>& adv_results, const std::vector<Syft::SynthesisResult>& coop_results) const {
 
        std::vector<int> adv_realizable, coop_realizable, unrealizable;
        for (int i = 0; i < ltlf_envs_.size(); ++i) {
            if (adv_results[i].realizability) adv_realizable.push_back(i);
            else if (!adv_results[i].realizability && coop_results[i].realizability) coop_realizable.push_back(i);
            else unrealizable.push_back(i);
    
        }
        std::cout << "[MtSyft] Goal realizable in environments: ";
        for (const auto& i: adv_realizable) std::cout << i+1 << " ";
        std::cout << std::endl;

        std::cout << "[MtSyft] Goal cooperatively realizable in environments: ";
        for (const auto& i: coop_realizable) std::cout << i+1 << " ";
        std::cout << std::endl;

        std::cout <<"[MtSyft] Goal unrealizable in environments: ";
        for (const auto& i: unrealizable) std::cout << i+1 << " ";
        std::cout << std::endl;
    }

    void EnvironmentsChainBestEffortSynthesizer::interactive(const std::vector<SynthesisResult>& adversarial_result,
                                                            const std::vector<SynthesisResult>& cooperative_result) const 
    {
        std::cout << "[MtSyft][interactive] on-the-fly execution" << std::endl;

        // var_mgr_->print_index_to_name();
        // std::cout << std::endl;
        // var_mgr_->print_name_to_variable();

        // INITIAL STATE
        // order of variables is (X \/ Y, Z_{G}, Z_{E_1}, ..., Z_{E_n}, Z_{tt})
        std::vector<int> state, goal_init = symbolic_dfas_[0][0].initial_state(), tau_init = symbolic_dfas_[0][2].initial_state();
        for (int i = 0; i < var_mgr_->get_index_to_name().size(); ++i) state.push_back(0); // X \/ Y vars are 0 at the initial state
        state.insert(state.end(), goal_init.begin(), goal_init.end()); // Z_{G}
        for (int i = 0; i < symbolic_dfas_.size(); ++i) {
          std::vector<int> env_init = symbolic_dfas_[i][1].initial_state();
          state.insert(state.end(), env_init.begin(), env_init.end()); // Z_{E_1}, ... Z_{E_n}
        } 
        state.insert(state.end(), tau_init.begin(), tau_init.end()); // Z_{tt}

        // std::cout << "Initial state: ";
        // for (const auto& b : state) std::cout << b;
        // std::cout << ". Size: " << state.size()  << std::endl;

        if (starting_player_==Player::Agent) {
        bool running = true;
        int least_valid_env = 0;
        while (running) {
            // determines maximum env with winning output
            int max_win = -1, min_coop = -1;
            for (int i = symbolic_dfas_.size() - 1; i >= least_valid_env; --i) {
                CUDD::BDD winning_region = adversarial_result[i].winning_states;
                if (winning_region.Eval(state.data()).IsOne()) {max_win = i; break;}    
            }
            // determines minimum env with coopearive output (which is not winning)
            for (int i = max_win+1; i < symbolic_dfas_.size(); ++i) {
                CUDD::BDD cooperative_region = cooperative_result[i].winning_states;
                if (cooperative_region.Eval(state.data()).IsOne()) {min_coop = i; break;}
            }

            std::cout << "[MtSyft][interactive] Max win: " << max_win+1 << ". Min coop: " << min_coop+1 << std::endl;

            // gets output function
            std::unordered_map<int, CUDD::BDD> output_function;
            if (max_win >= 0) output_function = adversarial_result[max_win].transducer.get()->get_output_function();
            else if (max_win < 0 && min_coop >= 0) output_function = cooperative_result[min_coop].transducer.get()->get_output_function();
            else {
                // output_function = cooperative_result[0].transducer.get()->get_output_function(); // i.e. any output is best-effort
                std::cout << "[MtSyft][interactive] Losing region reached. Terminating" << std::endl;
                return;
            }
            

            // agent moves first
            std::vector<int> transition = state; //
            std::unordered_map<int, std::string> id_to_var = var_mgr_->get_index_to_name();
            std::cout << "[MtSyft][interactive] agent choice: " << std::endl;
            for(int i = 0; i < id_to_var.size(); ++i) {
                std::string var = id_to_var[i];
                int agent_eval;
                if (var_mgr_->is_output_variable(var)) {
                    std::cout << "Variable: " << var;
                    std::cout << ". Agent output (0=false, 1=true): ";
                    agent_eval = output_function[i].Eval(state.data()).IsOne();
                    std::cout << agent_eval << std::endl;
                    transition[i] = agent_eval;
                }
            }

            // environment turn
            std::cout << "[MtSyft][interactive] environment choice (type 1 if var is true, else 0): " << std::endl;
            for (int i = 0; i < id_to_var.size(); ++i) {
                std::string var = id_to_var[i];
                int env_eval;
                if (var_mgr_->is_input_variable(var)) {
                    std::cout << "Variable: " << var;
                    std::cout << ". Env Input (0=false, 1=true): ";
                    std::cin >> env_eval;
                    transition[i] = env_eval;
                } 
            }

            std::cout << "[MtSyft][interactive] Current state: ";
            for (const auto&b : state) std::cout << b;
            // std::cout << " Size. " << state.size() << std::endl;
            std::cout << std::endl;

            std::cout << "[MtSyft][interactive] Input to transitions: ";
            for (const auto&b : transition) std::cout << b;
            // std::cout << " Size. "<< transition.size() << std::endl;
            std::cout << std::endl;

            // successor state
            int curr_state_var = id_to_var.size();
            // std::cout << "Current state variable: " << curr_state_var << std::endl;
            std::vector<int> new_state = state;
            for(int i = 0; i < symbolic_dfas_[0][0].transition_function().size(); ++i) {
                new_state[curr_state_var] = symbolic_dfas_[0][0].transition_function()[i].Eval(transition.data()).IsOne();
                ++curr_state_var;
            }
            for(int i = 0; i < symbolic_dfas_.size(); ++i) {
                for (int j = 0; j < symbolic_dfas_[i][1].transition_function().size(); ++j) {
                    new_state[curr_state_var] = symbolic_dfas_[i][1].transition_function()[j].Eval(transition.data()).IsOne();
                    ++curr_state_var;
                }
            }
            for(int i = 0; i < symbolic_dfas_[0][2].transition_function().size(); ++i) {
                new_state[curr_state_var] = symbolic_dfas_[0][2].transition_function()[i].Eval(transition.data()).IsOne();
                ++curr_state_var;
            }

            std::cout << "[MtSyft][interactive] Successor state: ";
            for (const auto& b: new_state) std::cout << b;

            // update state
            state = new_state;
            std::cout << std::endl;

            // evaluate whether we can stop the loop
            // debug
            for (int i = 0; i < symbolic_dfas_.size(); ++i) std::cout << "[MtSyft][interactive] Environment " << i+1 << " status (0 = invalid, 1 = valid): " << symbolic_dfas_[i][1].final_states().Eval(state.data()).IsOne() << std::endl;

            for (int i = 0; i < symbolic_dfas_.size(); ++i) {
                if (symbolic_dfas_[i][1].final_states().Eval(state.data()).IsOne()) {least_valid_env = i; break;}
                else {least_valid_env = symbolic_dfas_.size();}
            }

            // std::cout << "Least valid environment: " << least_valid_env << std::endl;
            if (least_valid_env == symbolic_dfas_.size()) {
                std::cout << "[MtSyft][interactive] All environments are negated. Terminating" << std::endl;
                running = false;
            }

            if (symbolic_dfas_[0][0].final_states().Eval(state.data()).IsOne()) {
                std::cout << "[MtSyft][interactive] The goal has been reached. Terminating" << std::endl;
                running = false;
            }
        }
        } else if (starting_player_==Player::Environment) { // not implemented yet
        }   
    }

    std::vector<double> EnvironmentsChainBestEffortSynthesizer::get_run_times() const {
        return run_times_;
    }
}