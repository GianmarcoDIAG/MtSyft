/*
*
* This file defines the BestEffortSynthesizer type
* Created by Gianmarco P.
*
*/

#include "BestEffortSynthesizer.h"
#include <boost/algorithm/string.hpp>
#include <queue>

namespace Syft
{

    BestEffortSynthesizer::BestEffortSynthesizer(std::shared_ptr<VarMgr> var_mgr,
                                                 std::string agent_specification,
                                                 std::string environment_specification,
                                                 InputOutputPartition partition,
                                                 Player starting_player)    :   var_mgr_(var_mgr),
                                                                                agent_specification_(agent_specification),
                                                                                environment_specification_(environment_specification),
                                                                                partition_(partition),
                                                                                starting_player_(starting_player)
    {
        // 1. Step 1 of algorithm by Arminof, De Giacomo and Rubin [1]
        // Construct symbolic DFAs formulas {E -> phi, !E, E && phi}
        // i. Build MONA DFAs for agent and environment specifications

        auto start_mona_dfa = std::chrono::high_resolution_clock::now();

        ExplicitStateDfaMona agent_spec_dfa =
            ExplicitStateDfaMona::dfa_of_formula(agent_specification); // DFA A_{phi}
        ExplicitStateDfaMona environment_spec_dfa =
            ExplicitStateDfaMona::dfa_of_formula(environment_specification); // DFA A_{E}
        ExplicitStateDfaMona tautology_dfa =
            ExplicitStateDfaMona::dfa_of_formula("tt"); // accepts only non-empty traces

        // DFA A_{phi}
        std::cout << std::endl;
        std::cout << "Agent goal DFA\n";
        agent_spec_dfa.dfa_print();
        std::cout << std::endl;

        // DFA A_{E}
        std::cout << std::endl;
        std::cout << "Environment Specification DFA\n";
        environment_spec_dfa.dfa_print();
        std::cout << std::endl;

        // tautoloty DFA
        std::cout << std::endl;
        std::cout << "Tautology DFA\n";
        tautology_dfa.dfa_print();
        std::cout << std::endl;

        auto stop_mona_dfa = std::chrono::high_resolution_clock::now();
        auto mona_dfa_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_mona_dfa - start_mona_dfa);
        std::cout << "MONA DFA construction DONE in: " << std::to_string(mona_dfa_duration.count()) << " ms" << std::endl;

        // ii. Obtain parsed formulas (requirement to construct symbolic DFAs)
        auto start_symbolic_dfa = std::chrono::high_resolution_clock::now();

        std::string adversarial_formula = 
            "(" + environment_specification + ") -> (" + agent_specification +")"; 

        formula parsed_adversarial_formula = 
            parse_formula(adversarial_formula.c_str()); // parses (E -> phi)

        // iii. Extract propositions from formula and partition
        var_mgr_->create_named_variables(get_props(parsed_adversarial_formula)); // (E -> phi) includes all problem variables
        var_mgr_->partition_variables(partition_.input_variables,
                                        partition_.output_variables);

        // iv. Get explicit state DFA from MONA DFA

        ExplicitStateDfa explicit_agent_dfa =
            ExplicitStateDfa::from_dfa_mona(var_mgr_, agent_spec_dfa);
        ExplicitStateDfa explicit_env_dfa =
            ExplicitStateDfa::from_dfa_mona(var_mgr_, environment_spec_dfa); 
        ExplicitStateDfa explicit_tau_dfa =
            ExplicitStateDfa::from_dfa_mona(var_mgr_, tautology_dfa);

        // v. Get Symbolic State DFA from Explicit DFA

        symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(explicit_agent_dfa)));
        symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(explicit_env_dfa)));
        symbolic_dfas_.push_back(SymbolicStateDfa::from_explicit(std::move(explicit_tau_dfa)));
        
        // f_{phi} is stored in symbolic_dfas_[0].final_states()
        // f_{E} is stored in symbolic_dfas_[1].final_states()

        auto stop_symbolic_dfa = std::chrono::high_resolution_clock::now();
        auto symbolic_dfa_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_symbolic_dfa - start_symbolic_dfa);
        std::cout << "Symbolic DFA construction DONE in: " << std::to_string(symbolic_dfa_duration.count()) << " ms" << std::endl;

        auto start_arena = std::chrono::high_resolution_clock::now();

        // Step 2. a. Construct ARENA for best-effort synthesis through product
        SymbolicStateDfa arena = 
            SymbolicStateDfa::product(symbolic_dfas_);
        arena_.push_back(arena);
        // b. Lifting implemented as part of the run member function

        auto stop_arena = std::chrono::high_resolution_clock::now();
        auto arena_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_arena - start_arena);
        std::cout << "Arena construction DONE in: " << std::to_string(arena_duration.count()) << std::endl;
    }

    std::pair<SynthesisResult, SynthesisResult> BestEffortSynthesizer::run() {

        // Pair to store results
        std::pair<SynthesisResult, SynthesisResult> best_effort_result;

        // Step 3. Compute winning strategy in the adversarial game

        auto start_adversarial_game = std::chrono::high_resolution_clock::now();

        std::cout << "Solving synthesis on the adversarial game..." << std::endl;
        ReachabilitySynthesizer adversarial_synthesizer(arena_[0],
                                                        starting_player_,
                                                        Player::Agent,
                                                        (!(symbolic_dfas_[1].final_states() * (!symbolic_dfas_[0].final_states()))) * !arena_[0].initial_state_bdd(), // Lifting
                                                        var_mgr_->cudd_mgr()->bddOne());
        // Above, note that the set of final states is forged through lifting
        // i.e. Game final states depend on adversarial DFA final states
        best_effort_result.first = adversarial_synthesizer.run();
        // std::cout << "Solving synthesis on the adversarial game...DONE\n" << std::endl;

        auto stop_adversarial_game = std::chrono::high_resolution_clock::now();
        auto adversarial_game_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_adversarial_game - start_adversarial_game);
        std::cout << "Solving synthesis on the adversarial game...DONE in: " << std::to_string(adversarial_game_duration.count()) << " ms" << std::endl;

        if (best_effort_result.first.realizability) {
            return best_effort_result;
        } // Terminate if game is reactively realizable; i.e. enforcing strategy is best-effort
        // Comment for performance tests

        // Step 4. Compute environment's winning region in negated environment game

        auto start_restriction = std::chrono::high_resolution_clock::now();

        std::cout << "Solving synthesis on the environment assumption only dfa..." << std::endl;
        ReachabilitySynthesizer negated_environment_synthesizer(arena_[0],
                                                                starting_player_,
                                                                Player::Agent,  // get env winning region from agent's
                                                                ((!symbolic_dfas_[1].final_states())) * !arena_[0].initial_state_bdd(), // Lifting
                                                                var_mgr_->cudd_mgr()->bddOne());
        SynthesisResult environment_result = negated_environment_synthesizer.run();
        std::cout << "Solving synthesis on the environment assumption only dfa...DONE" << std::endl;
        CUDD::BDD non_environment_winning_region = environment_result.winning_states;

        // Step 5. Restrict arena to environemt winning region.
        // i.e. all states that are in non_environment_winning_region have to be pruned as invalid

        arena_[0].prune_invalid_states(non_environment_winning_region);

        auto stop_restriction = std::chrono::high_resolution_clock::now();
        auto restriction_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_restriction - start_restriction);
        std::cout << "Restriction DONE in: " << std::to_string(restriction_duration.count()) << " ms" << std::endl;

        // Step 6. Compute cooperatively winning strategy in restricted game

        auto start_cooperative_game = std::chrono::high_resolution_clock::now();

        std::cout << "Solving cooperative synthesis on the reduced arena..." << std::endl;
        CoOperativeReachabilitySynthesizer co_operative_synthesizer(arena_[0],
                                                                    starting_player_,
                                                                    Player::Agent,
                                                                    (symbolic_dfas_[0].final_states() * symbolic_dfas_[1].final_states()), // Lifting
                                                                    var_mgr_->cudd_mgr()->bddOne()); 
        best_effort_result.second = co_operative_synthesizer.run();
        // For experiment consider final states with symbolic_dfas_[1].final_states() * (!symbolic_dfas_[0].final_states())
        // std::cout << "Solving cooperative synthesis on the reduced arena...DONE\n" << std::endl;

        auto end_coopeartive_game = std::chrono::high_resolution_clock::now();
        auto cooperative_game_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_coopeartive_game - start_cooperative_game);
        std::cout << "Solving cooperative synthesis on the reduced arena...DONE in: " << std::to_string(cooperative_game_duration.count()) << " ms" << std::endl;

        // CUDD::BDD cooperative_final_states = (symbolic_dfas_[0].final_states() * symbolic_dfas_[1].final_states());
        // CUDD::BDD adversarial_final_states = !(symbolic_dfas_[1].final_states() * (!symbolic_dfas_[0].final_states()));
        // CUDD::BDD negated_env_states = !(symbolic_dfas_[1].final_states());

        // var_mgr_->dump_dot(arena_[0].initial_state_bdd().Add(), "arena_init_state.dot");
        // var_mgr_->dump_dot((!arena_[0].initial_state_bdd()).Add(), "arena_not_init_state.dot");

        // var_mgr_->dump_dot((adversarial_final_states * !(arena_[0].initial_state_bdd())).Add(), "adversarial_states.dot");
        // var_mgr_->dump_dot((negated_env_states * !(arena_[0].initial_state_bdd())).Add(), "negated_env_states_2.dot");
        // var_mgr_->dump_dot((cooperative_final_states * !(arena_[0].initial_state_bdd())).Add(), "cooperative_states.dot");

        return best_effort_result;
    }

    void BestEffortSynthesizer::merge_and_dump_dot(const SynthesisResult& adversarial_result, const SynthesisResult& cooperative_result, const string& filename) const {
        // Instruction for performance measurements
        auto start_merging = std::chrono::high_resolution_clock::now();

        std::vector<std::string> output_labels = var_mgr_->output_variable_labels(); // i.e. Y variables

        std::size_t output_count = cooperative_result.transducer.get()->output_function_.size();
        std::vector<CUDD::ADD> output_vector(output_count);

        // Cooperatively only winning states, i.e. states in cooperatively, but not reactively, winning region
        CUDD::BDD cooperative_only_winning_states = (!adversarial_result.winning_states) * cooperative_result.winning_states;
        //var_mgr_->dump_dot(cooperative_only_winning_states.Add(), "coop_only_states.dot");
        //var_mgr_->dump_dot(adversarial_result.winning_states.Add(), "adv_states.dot");
        for(std::size_t i=0; i < output_count; ++i) {
            std::string label = output_labels[i];
            int index = var_mgr_->name_to_variable(label).NodeReadIndex();
            // i. For reactively winning states use reactive output function
            CUDD::BDD restricted_adversarial_bdd = 
                adversarial_result.transducer.get()->output_function_.at(index) * adversarial_result.winning_states; 
            // ii. For cooperatively only winning states use cooperative output function
            CUDD::BDD restricted_cooperative_bdd = 
                cooperative_result.transducer.get()->output_function_.at(index) * cooperative_only_winning_states; 
            /// iii. For any state keep best-effort output
            CUDD::BDD merged_bdd = restricted_adversarial_bdd + restricted_cooperative_bdd;
            output_vector[i] = merged_bdd.Add();
        }
        var_mgr_->dump_dot(output_vector, output_labels, filename);

        auto stop_merging = std::chrono::high_resolution_clock::now();
        auto merging_duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_merging - start_merging);
        std::cout << "Merging DONE in: " << std::to_string(merging_duration.count()) << " ms" << std::endl;
    }
}

// REFERENCES
// [1] Benjamin Aminof, Giuseppe De Giacomo, and Sasha Rubin. Best-EffortSynthesis: Doing Your Best Is Not Harder Than Giving Up. In IJCAI, 2021
// [2] Shufang Zhu, Lucas M. Tabajara, Jianwen Li, Geguang Pu, and Moshe Y.Vardi. Symbolic LTLf Synthesis. In IJCAI, 2017.
