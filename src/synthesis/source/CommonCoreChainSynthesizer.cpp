#include"CommonCoreChainSynthesizer.h"

namespace Syft {

    CommonCoreChainSynthesizer::CommonCoreChainSynthesizer(
        std::shared_ptr<Syft::VarMgr> var_mgr,
        std::string ltlf_goal,
        std::string env_core,
        std::vector<std::string> conjuncts,
        Syft::InputOutputPartition partition,
        Syft::Player starting_player
        ) : var_mgr_(var_mgr),
            ltlf_goal_(ltlf_goal),
            env_core_(env_core),
            conjuncts_(conjuncts),
            partition_(partition),
            starting_player_(starting_player)     // be careful on constructor. bad_alloc might be issued if u use bad arguments
        {
            Syft::Stopwatch ltlf2dfas;
            ltlf2dfas.start();

            Syft::Stopwatch goal2dfa;
            goal2dfa.start();

            ExplicitStateDfaMona mona_goal_dfa =
                ExplicitStateDfaMona::dfa_of_formula(ltlf_goal_);

            double t_goal2dfa = goal2dfa.stop().count() / 1000.0;

            Syft::Stopwatch core2dfa;
            core2dfa.start();

            ExplicitStateDfaMona mona_core_dfa =
                ExplicitStateDfaMona::dfa_of_formula(env_core_);

            double t_core2dfa = core2dfa.stop().count() / 1000.0;

            std::vector<double> conjunct2dfas;

            std::vector<ExplicitStateDfaMona> mona_conjuncts_dfas;
            for (const auto& conjunct: conjuncts_) {
                Syft::Stopwatch conjunct2dfa;
                conjunct2dfa.start();
                mona_conjuncts_dfas.push_back(ExplicitStateDfaMona::dfa_of_formula(conjunct));
                conjunct2dfas.push_back(conjunct2dfa.stop().count() / 1000.0);
            }

            Syft::Stopwatch tau2dfa;
            tau2dfa.start();

            ExplicitStateDfaMona mona_no_empty_dfa =
                ExplicitStateDfaMona::dfa_of_formula("F(true)");

            double t_tau2dfa = tau2dfa.stop().count() / 1000.0;

            // debug
            // mona_goal_dfa.dfa_print();
            // std::cout << std::endl;
            // mona_core_dfa.dfa_print();
            // std::cout << std::endl;
            // for (int i = 0; i < mona_conjuncts_dfas.size(); ++i) {
            //     mona_conjuncts_dfas[i].dfa_print();
            //     std::cout << std::endl;
            // }

            run_times_.push_back(ltlf2dfas.stop().count() / 1000.0);
            std::cout << "[cb-MtSyft] converting LTLf into DFA...DONE (" << run_times_[0] << " s)" << std::endl;
            std::cout << "\t[cb-MtSyft] goal to DFA in " << t_goal2dfa << " s"<< std::endl;
            std::cout << "\t[cb-MtSyft] env core to DFA in " << t_core2dfa << " s"<< std::endl;
            for (int i = 0; i < conjunct2dfas.size(); ++i) {
                std::cout << "\t[cb-MtSyft] env conjunct " << i+1 << " to DFA in " << conjunct2dfas[i] << " s"<< std::endl;
            }
            std::cout << "\t[cb-MtSyft] tautology to DFA in " << t_tau2dfa << " s" << std::endl;

            // converting to symbolic representation
            
            std::cout << "[cb-MtSyft] preprocessing...";
            Syft::Stopwatch pre;
            pre.start();

            // std::string conjunct_formula = "(" + ltlf_goal_ + ")";
            // if (env_core_ != "tt") conjunct_formula += " && (" + env_core + ")";
            // for (const auto& conjunct : conjuncts_) {
            //     if (conjunct != "tt") conjunct_formula += " && (" + conjunct + ")";
            // }
            // std::cout << conjunct_formula << std::endl;

            // formula parsed_conjunct_formula =
            //     parse_formula(conjunct_formula.c_str());

            // std::cout << parsed_conjunct_formula << std::endl;
        
            // Rewrites the preprocessing phase to not use the SPOT parser
            // which does not interact well with Lydia (does not support Lydia's X[!])
            var_mgr_->create_named_variables(mona_goal_dfa.names);
            var_mgr_->create_named_variables(mona_core_dfa.names);
            for (const auto& conjunct_dfa: mona_conjuncts_dfas) var_mgr_->create_named_variables(conjunct_dfa.names);

            // partition
            // var_mgr_->create_named_variables(get_props(parsed_conjunct_formula));
            var_mgr_->partition_variables(partition_.input_variables,
                                        partition_.output_variables);

            double t_pre = pre.stop().count() / 1000.0;
            std::cout << "DONE (" << t_pre << " s)" << std::endl;

            std::cout << "[cb-MtSyft] converting to symbolic DFA...";
            Syft::Stopwatch dfa2symbolic;
            dfa2symbolic.start();

            // TODO (Gianmarco). Rewrite to construct a small game arena for each triple
            // goal to symbolic
            Syft::Stopwatch goal2sym;
            goal2sym.start();

            ExplicitStateDfa goal_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_goal_dfa);
            SymbolicStateDfa goal_sym_dfa = SymbolicStateDfa::from_explicit(std::move(goal_dfa));
            // symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(goal_dfa)));

            for(int i = 0; i < mona_conjuncts_dfas.size(); ++i) {
                symbolic_dfas_.emplace_back();
                symbolic_dfas_[i].push_back(goal_sym_dfa);
            }

            double t_goal2sym = goal2sym.stop().count() / 1000.0;

            // core to symbolic
            Syft::Stopwatch core2sym;
            core2sym.start();

            ExplicitStateDfa core_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_core_dfa);
            SymbolicStateDfa core_sym_dfa = SymbolicStateDfa::from_explicit(std::move(core_dfa));
            // symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(core_dfa)));

            for(int i =0; i < mona_conjuncts_dfas.size(); ++i) symbolic_dfas_[i].push_back(core_sym_dfa);

            double t_core2sym = core2sym.stop().count() / 1000.0;

            std::vector<double> t_conjunct2dfas;

            std::vector<ExplicitStateDfa> conjunct_dfas;
            for (int i = 0; i < mona_conjuncts_dfas.size(); ++i) {
                Syft::Stopwatch conjunct2dfa;
                conjunct2dfa.start();
                conjunct_dfas.push_back(ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_conjuncts_dfas[i]));
                symbolic_dfas_[i].push_back(SymbolicStateDfa::from_explicit(std::move(conjunct_dfas[i])));
                t_conjunct2dfas.push_back(conjunct2dfa.stop().count() / 1000.0);
            }

            Syft::Stopwatch tau2sym;
            tau2sym.start();

            // add tautology to the product forming each game arena
            ExplicitStateDfa no_empty_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_no_empty_dfa);
            SymbolicStateDfa no_empty_sym_dfa = SymbolicStateDfa::from_explicit(std::move(no_empty_dfa));
            for(int i = 0; i < mona_conjuncts_dfas.size(); ++i) symbolic_dfas_[i].push_back(no_empty_sym_dfa);

            // symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(no_empty_dfa)));

            double t_tau2sym = tau2sym.stop().count() / 1000.0;

            // symbolic DFAs
            // for (int i = 0; i < conjunct_dfas.size(); ++i) {
            //     symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(conjunct_dfas[i])));
            // }
            
            // symbolic dfas are {\varphi, E_c, E'_1, E'_2, ..., E'_n, tt}
            Syft::Stopwatch arena_time;
            arena_time.start();

            std::vector<double> t_arena;

            for(int i = 0; i < symbolic_dfas_.size(); ++i) {
                Syft::Stopwatch arena;
                arena.start();
                arena_.push_back(SymbolicStateDfa(SymbolicStateDfa::product(symbolic_dfas_[i])));
                t_arena.push_back(arena.stop().count() / 1000.0);
            }

            double arena_stop = arena_time.stop().count() / 1000.0;
            run_times_.push_back(dfa2symbolic.stop().count() / 1000.0);

            std::cout << "DONE (" << run_times_[1] << " s)" << std::endl;
            std::cout << "\t[cb-MtSyft] goal DFA to symbolic in " << t_goal2sym << " s" << std::endl;
            std::cout << "\t[cb-MtSyft] env core DFA to symbolic in " << t_core2sym << " s" << std::endl;
            for (int i = 0; i < t_conjunct2dfas.size(); ++i) {
                std::cout << "\t[cb-MtSyft] env conjunct DFA " << i+1 << " to symbolic in " << t_conjunct2dfas[i] << " s" << std::endl;
            }
            std::cout << "\t[cb-MtSyft] tautology DFA to symbolic in " << t_tau2dfa << std::endl;
            for (int i = 0; i < t_arena.size(); ++i) std::cout << "\t[cb-MtSyft] arena " << i + 1 << " constructed in " << t_arena[i] << " s" <<  std::endl;
        }

        ChainedStrategies CommonCoreChainSynthesizer::run() {
            
            std::cout << "[cb-MtSyft] constructing and solving games...";
            ChainedStrategies result;

            Syft::Stopwatch adv_games;
            adv_games.start();

            // adversarial games
            std::vector<double> t_adv_games;
            for (int i = 0; i < arena_.size(); ++i) {
                Syft::Stopwatch adv_game;
                adv_game.start();
                CUDD::BDD adversarial_goal = ((!(symbolic_dfas_[i][1].final_states() * symbolic_dfas_[i][2].final_states())) + symbolic_dfas_[i][0].final_states()) * (!arena_[i].initial_state_bdd());
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
                CUDD::BDD negated_env_goal = (!(symbolic_dfas_[i][1].final_states() * symbolic_dfas_[i][2].final_states())) * (!arena_[i].initial_state_bdd());
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
                CUDD::BDD cooperative_goal = (symbolic_dfas_[i][1].final_states() * symbolic_dfas_[i][2].final_states()) * symbolic_dfas_[i][0].final_states();
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
                std::cout << "\t[cb-MtSyft] adv game in env " << i + 1 << " solved in " << t_adv_games[i] << " s" << std::endl;
            for (int i = 0; i < t_coop_games.size(); ++i)
                std::cout << "\t[cb-MtSyft] coop game in env " << i + 1 << " solved in " << t_coop_games[i] << " s" << std::endl;

            return result;
        }

        void CommonCoreChainSynthesizer::realizability(const std::vector<Syft::SynthesisResult>& adv_results, const std::vector<Syft::SynthesisResult>& coop_results) const {
 
        std::vector<int> adv_realizable, coop_realizable, unrealizable;
        for (int i = 0; i < adv_results.size(); ++i) {
            if (adv_results[i].realizability) adv_realizable.push_back(i);
            else if (!adv_results[i].realizability && coop_results[i].realizability) coop_realizable.push_back(i);
            else unrealizable.push_back(i);
    
        }
        std::cout << "[cb-MtSyft] Goal realizable in environments: ";
        for (const auto& i: adv_realizable) std::cout << i+1 << " ";
        std::cout << std::endl;

        std::cout << "[cb-MtSyft] Goal cooperatively realizable in environments: ";
        for (const auto& i: coop_realizable) std::cout << i+1 << " ";
        std::cout << std::endl;

        std::cout <<"[cb-MtSyft] Goal unrealizable in environments: ";
        for (const auto& i: unrealizable) std::cout << i+1 << " ";
        std::cout << std::endl;
    }

    std::vector<double> CommonCoreChainSynthesizer::get_run_times() const {
        return run_times_;
    }


}