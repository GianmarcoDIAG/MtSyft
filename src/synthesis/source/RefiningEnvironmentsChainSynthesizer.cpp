#include"RefiningEnvironmentsChainSynthesizer.h"

namespace Syft {

    RefiningEnvironmentsChainSynthesizer::RefiningEnvironmentsChainSynthesizer(
                std::shared_ptr<Syft::VarMgr> var_mgr,
                std::string ltlf_goal,
                std::vector<std::string> refinements,
                std::string env_base,
                Syft::InputOutputPartition partition,
                Syft::Player starting_player
            ): var_mgr_(var_mgr),
            ltlf_goal_(ltlf_goal),
            refinements_(refinements),
            env_base_(env_base),
            partition_(partition),
            starting_player_(starting_player) 
        {
            Syft::Stopwatch ltlf2dfas;
            ltlf2dfas.start();

            Syft::Stopwatch goal2dfa;
            goal2dfa.start();

            ExplicitStateDfaMona mona_goal_dfa =
                ExplicitStateDfaMona::dfa_of_formula(ltlf_goal_);

            double t_goal2dfa = goal2dfa.stop().count() / 1000.0;

            std::vector<double> t_refinement2dfas;

            std::vector<ExplicitStateDfaMona> mona_refinements_dfas;
            for (const auto& refinement: refinements_) {
                Syft::Stopwatch refinement2dfa;
                refinement2dfa.start();
                mona_refinements_dfas.push_back(ExplicitStateDfaMona::dfa_of_formula(refinement)); // E'_1, E'_2, ..., E'_{n-1}
                t_refinement2dfas.push_back(refinement2dfa.stop().count() / 1000.0);
            }

            Syft::Stopwatch base2dfa;
            base2dfa.start();

            ExplicitStateDfaMona mona_base_dfa =
                ExplicitStateDfaMona::dfa_of_formula(env_base); // E_{n}

            double t_base2dfa = base2dfa.stop().count() / 1000.0;

            Syft::Stopwatch tau2dfa;
            tau2dfa.start();

            ExplicitStateDfaMona mona_no_empty_dfa =
                ExplicitStateDfaMona::dfa_of_formula("F(true)");

            double t_tau2dfa = tau2dfa.stop().count() / 1000.0;

            // debug
            // mona_goal_dfa.dfa_print();
            // std::cout << std::endl;
            // for (int i = 0; i < mona_refinements_dfas.size(); ++i) {
            //     mona_refinements_dfas[i].dfa_print();
            //     std::cout << std::endl;
            // }
            // mona_base_dfa.dfa_print();
            // std::cout << std::endl;

            run_times_.push_back(ltlf2dfas.stop().count() / 1000.0);
            std::cout << "[conj-MtSyft] converting LTLf into DFA...DONE (" << run_times_[0] << " s)" << std::endl;
            std::cout << "\t[conj-MtSyft] goal DFA in " << t_goal2dfa << " s" << std::endl;
            for (int i = 0; i < t_refinement2dfas.size(); ++i) {
                std::cout << "\t[conj-MtSyft] refinement " << i+1 << " to DFA in " << t_refinement2dfas[i] << " s" << std::endl;
            }
            std::cout << "\t[conj-MtSyft] env base to DFA in " << t_base2dfa << " s" << std::endl;
            std::cout << "\t[conj-MtSyft] tautoloty to DFA in " << t_tau2dfa << " s" << std::endl;

            std::cout << "[conj-MtSyft] preprocessing...";
            Syft::Stopwatch pre;
            pre.start();

            // std::string conjunct_formula = "(" + ltlf_goal_ + ")";
            // for (auto const& refinement : refinements_) {
                // if (refinement != "tt") conjunct_formula += " && (" + refinement + ")";
            // }
            // if (env_base_ != "tt") conjunct_formula += " && (" + env_base + ")";

            // formula parsed_conjunct_formula =
                // parse_formula(conjunct_formula.c_str());

            // var_mgr_->create_named_variables(get_props(parsed_conjunct_formula));
            
            // Rewrites the preprocessing phase to not use the SPOT parser
            // which does not interact well with Lydia (does not support Lydia's X[!])
            var_mgr_->create_named_variables(mona_goal_dfa.names);
            for (const auto& refinement_dfa: mona_refinements_dfas) var_mgr_->create_named_variables(refinement_dfa.names);

            var_mgr_->partition_variables(partition.input_variables,
                                        partition.output_variables);

            double t_pre = pre.stop().count() / 1000.0;
            std::cout <<  "DONE (" << t_pre << " s)" << std::endl;

            std::cout <<  "[conj-MtSyft] converting to symbolic DFA...";
            Syft::Stopwatch dfa2symbolic;
            dfa2symbolic.start();

            Syft::Stopwatch goal2sym;
            goal2sym.start();

            ExplicitStateDfa goal_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_goal_dfa);
            symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(goal_dfa)));

            double t_goal2sym = goal2sym.stop().count() / 1000.0;

            std::vector<double> t_refinement2sym;

            std::vector<ExplicitStateDfa> refinement_dfas;

            for (int i = 0; i < mona_refinements_dfas.size(); ++i) {
                Syft::Stopwatch refinement2sym;
                refinement2sym.start();
                refinement_dfas.push_back(ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_refinements_dfas[i]));
                symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(refinement_dfas[i])));
                t_refinement2sym.push_back(refinement2sym.stop().count() / 1000.0);
            }

            Syft::Stopwatch base2sym;
            base2sym.start();

            ExplicitStateDfa base_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_base_dfa);
            symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(base_dfa)));

            double t_base2sym = base2sym.stop().count() / 1000.0;

            // for (const auto& refinement_dfa: mona_refinements_dfas) {
            //     refinement_dfas.push_back(ExplicitStateDfa::from_dfa_mona(var_mgr_, refinement_dfa));
            // }

            Syft::Stopwatch tau2sym;
            tau2sym.start();

            ExplicitStateDfa no_empty_dfa = ExplicitStateDfa::from_dfa_mona(var_mgr_, mona_no_empty_dfa);
            symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(no_empty_dfa)));

            double t_tau2sym = tau2sym.stop().count() / 1000.0;

            // symbolic DFAs
            
            // for (const auto& refinement_dfa: refinement_dfas) {
            //     symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(refinement_dfa)));
            // }
            
            Syft::Stopwatch sym2arena;
            sym2arena.start();

            arena_.push_back(SymbolicStateDfa::product(symbolic_dfas_));

            double t_sym2arena = sym2arena.stop().count() / 1000.0;

            run_times_.push_back(dfa2symbolic.stop().count() / 1000.0);
            std::cout << "DONE (" << run_times_[1] << " s)" << std::endl;
            std::cout << "\t[conj-MtSyft] goal DFA to symbolic in " << t_goal2sym << " s" << std::endl;
            for (int i = 0; i < t_refinement2sym.size(); ++i) {
                std::cout << "\t[conj-MtSyft] env refinement "  << i+1 << " DFA to symbolic in " << t_refinement2sym[i] << " s" << std::endl; 
            }
            std::cout << "\t[conj-MtSyft] env base DFA to symbolic in " << t_base2sym << " s" << std::endl;
            std::cout << "\t[conj-MtSyft] symbolic arena construction in " << t_sym2arena << " s" << std::endl;
        }

    ChainedStrategies RefiningEnvironmentsChainSynthesizer::run() {

        std::cout << "[conj-MtSyft] constructing and solving games...";

        ChainedStrategies result;

        Syft::Stopwatch adv_games;
        adv_games.start();

        // adversarial games
    
        std::vector<double> t_adv_games;
        CUDD::BDD env_final_states = var_mgr_->cudd_mgr()->bddOne();
        for (int i = symbolic_dfas_.size()-2; i >= 1; --i) {
            
            Syft::Stopwatch adv_game;
            adv_game.start();

            env_final_states = env_final_states * symbolic_dfas_[i].final_states(); // construct env final states function
            CUDD::BDD adv_goal = (!(env_final_states) + symbolic_dfas_[0].final_states()) * !arena_[0].initial_state_bdd(); // adv agent goal
            // CUDD::BDD adv_goal = ((!env_final_states) + symbolic_dfas_[0].final_states()) * symbolic_dfas_[symbolic_dfas_.size() - 1].final_states();
            // solve game
            ReachabilitySynthesizer adversarial_synthesizer(
                arena_[0],
                starting_player_,
                Player::Agent,
                adv_goal,
                var_mgr_->cudd_mgr()->bddOne()
            );
            
            result.adversarial_results.push_back(adversarial_synthesizer.run());

            t_adv_games.push_back(adv_game.stop().count() / 1000.0); // note that these running times are from E_n -> \varphi to E_1 -> \varphi
        }
        run_times_.push_back(adv_games.stop().count() / 1000.0);

        // cooperative games
        Syft::Stopwatch coop_games;
        coop_games.start();

        std::vector<double> t_coop_games;
        env_final_states = var_mgr_->cudd_mgr()->bddOne();
        for (int i = symbolic_dfas_.size() - 2; i >= 1; --i) {
            Syft::Stopwatch coop_game;
            coop_game.start();

            // game on negated env DFA

            env_final_states = env_final_states * symbolic_dfas_[i].final_states(); // construct env final states function
            CUDD::BDD negated_env_goal = !(env_final_states) * !arena_[0].initial_state_bdd();

            ReachabilitySynthesizer negated_env_synthesizer(
                arena_[0],
                starting_player_,
                Player::Agent,
                negated_env_goal,
                var_mgr_->cudd_mgr()->bddOne()
            );
            SynthesisResult enviroment_result = negated_env_synthesizer.run();
            CUDD::BDD non_environment_winning_region = enviroment_result.winning_states;
            arena_.push_back(arena_[0].get_restriction(non_environment_winning_region));

            // cooperative game
            CUDD::BDD cooperative_goal = env_final_states * symbolic_dfas_[0].final_states();

            CoOperativeReachabilitySynthesizer co_operative_synthesizer(
                arena_[arena_.size() - 1], // last added element is restricted arena
                starting_player_,
                Player::Agent,
                cooperative_goal,
                var_mgr_->cudd_mgr()->bddOne()
            );
            
            result.cooperative_results.push_back(co_operative_synthesizer.run());

            t_coop_games.push_back(coop_game.stop().count() / 1000.0);
        }

        run_times_.push_back(coop_games.stop().count() / 1000.0);
        std::cout << "DONE (" << run_times_[2] + run_times_[3] << " s)"  << std::endl;
        int j = 1;
        for (int i = t_adv_games.size() - 1; i >= 0; --i) {
            std::cout << "\t[conj-MtSyft] adv game in env " << j << " solved in " << t_adv_games[i] << " s" << std::endl;
            ++j;
        }
        j = 1;
        for (int i = t_coop_games.size() - 1; i >= 0; --i) {
            std::cout <<  "\t[conj-MtSyft] coop game in env " << j << " solved in " << t_coop_games[i] << " s" << std::endl; 
            ++j;
        }

        return result;
    }

    void RefiningEnvironmentsChainSynthesizer::realizability(const std::vector<Syft::SynthesisResult>& adv_results, const std::vector<Syft::SynthesisResult>& coop_results) const {


        std::vector<int> adv_realizable, coop_realizable, unrealizable;
        int j = 1;
        // std::cout << adv_results.size() << std::endl;
        for (int i = adv_results.size() - 1; i >= 0; --i) {
            if (adv_results[i].realizability) adv_realizable.push_back(j);
            else if (!adv_results[i].realizability && coop_results[i].realizability) coop_realizable.push_back(j);
            else unrealizable.push_back(j);
            ++j;
        }
        
        std::cout << "[conj-MtSyft] Goal realizable in environments: ";
        for (const auto& i: adv_realizable) std::cout << i << " ";
        std::cout << std::endl;

        std::cout << "[conj-MtSyft] Goal cooperatively realizable in environments: ";
        for (const auto& i: coop_realizable) std::cout << i << " ";
        std::cout << std::endl;

        std::cout <<"[conj-MtSyft] Goal unrealizable in environments: ";
        for (const auto& i: unrealizable) std::cout << i << " ";
        std::cout << std::endl;

    }

    std::vector<double> RefiningEnvironmentsChainSynthesizer::get_run_times() const {
        return run_times_;
    }
}

// std::cout << "[conj-MtSyft] constructing and solving games...";

//         ChainedStrategies result;

//         Syft::Stopwatch adv_games;
//         adv_games.start();

//         // adversarial games
//         for (int i = 1; i < symbolic_dfas_.size() - 1; ++i) {
//             // construct adversarial goal
//             CUDD::BDD envs_final_states = var_mgr_->cudd_mgr()->bddOne();
//             for (int j = i; j < symbolic_dfas_.size() - 1; ++j) {
//                 envs_final_states = envs_final_states * symbolic_dfas_[j].final_states();
//             }
//             CUDD::BDD adversarial_goal = ((!(envs_final_states)) + symbolic_dfas_[0].final_states()) * (!(arena_[0].initial_state_bdd()));

//             // solve game
//             ReachabilitySynthesizer adversarial_synthesizer(
//                 arena_[0],
//                 starting_player_,
//                 Player::Agent,
//                 adversarial_goal,
//                 var_mgr_->cudd_mgr()->bddOne()
//             );
//             result.adversarial_results.push_back(adversarial_synthesizer.run());
//         }

//         run_times_.push_back(adv_games.stop().count() / 1000.0);

//         // cooperative games
//         Syft::Stopwatch coop_games;
//         coop_games.start();

//         for (int i = 1; i < symbolic_dfas_.size() - 1; ++i) {
//             // construct goal on negated env DFA game
//             CUDD::BDD envs_final_states = var_mgr_->cudd_mgr()->bddOne();
//             for (int j = i; j < symbolic_dfas_.size() - 1; ++j) {
//                 envs_final_states = envs_final_states * symbolic_dfas_[j].final_states();
//             }
//             CUDD::BDD negated_env_goal = !(envs_final_states) * !(arena_[0].initial_state_bdd());

//             // DFA game for negated env
//             ReachabilitySynthesizer negated_env_synthesizer(
//                 arena_[0],
//                 starting_player_,
//                 Player::Agent,
//                 negated_env_goal,
//                 var_mgr_->cudd_mgr()->bddOne()
//             );
//             SynthesisResult enviroment_result = negated_env_synthesizer.run();
//             CUDD::BDD non_environment_winning_region = enviroment_result.winning_states;
//             arena_.push_back(arena_[0].get_restriction(non_environment_winning_region));

//             // cooperative DFA game
//             CUDD::BDD cooperative_goal = envs_final_states * symbolic_dfas_[0].final_states();
//             CoOperativeReachabilitySynthesizer co_operative_synthesizer(
//                 arena_[i],
//                 starting_player_,
//                 Player::Agent,
//                 cooperative_goal,
//                 var_mgr_->cudd_mgr()->bddOne()
//             );
//             result.cooperative_results.push_back(co_operative_synthesizer.run());
//         }

//         run_times_.push_back(coop_games.stop().count() / 1000.0);
//         std::cout << "DONE (" << run_times_[2] + run_times_[3] << " s)"  << std::endl;

//         return result;