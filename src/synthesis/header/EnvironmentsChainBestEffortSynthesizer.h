/**
 * 
 * EnvironmentsChainBestEffortSynthesizer Header
 * Solves LTLf best-effort synthesis under a chain of env specs
 * 
*/

#ifndef SYFT_ENVIRONMENTCHAINBESTEFFORTSYNTHESIZER_H
#define SYFT_ENVIRONMENTCHAINBESTEFFORTSYNTHESIZER_H

#include<string>
#include<vector>
#include<utility>
#include<algorithm>
#include"VarMgr.h"
#include"Synthesizer.h"
#include"SymbolicStateDfa.h"
#include"InputOutputPartition.h"
#include"ReachabilitySynthesizer.h"
#include"CoOperativeReachabilitySynthesizer.h"
#include"Player.h"
#include"spotparser.h"
#include"Stopwatch.h"

namespace Syft {

    class EnvironmentsChainBestEffortSynthesizer {

        // data members
        protected:
            std::shared_ptr<VarMgr> var_mgr_;

            std::string ltlf_goal_;
            std::vector<std::string> ltlf_envs_;

            Player starting_player_;

            std::vector<std::vector<SymbolicStateDfa>> symbolic_dfas_;
            std::vector<SymbolicStateDfa> arena_;
            std::vector<SymbolicStateDfa> restricted_arena_;

            Syft::InputOutputPartition partition_;

            std::vector<double> run_times_; 
        
        public:
            // member functions
            // constructor
            EnvironmentsChainBestEffortSynthesizer(
                std::shared_ptr<Syft::VarMgr> var_mgr,
                std::string ltlf_goal,
                std::vector<std::string> ltlf_envs,
                Syft::InputOutputPartition partition,
                Syft::Player starting_player
            );

            // DFA game solving
            // virtual std::pair<std::vector<SynthesisResult>, std::vector<SynthesisResult>> run() final;
            virtual ChainedStrategies run() final;

            void dump_chained_strategies(const std::vector<std::vector<CUDD::BDD>>& chained_strategies) const;

            void realizability(const std::vector<Syft::SynthesisResult>& adv_results,
                                const std::vector<Syft::SynthesisResult>& coop_results) const;

            // TODO. Implement interactive member function
            void interactive(const std::vector<Syft::SynthesisResult>& adv_results,
                            const std::vector<Syft::SynthesisResult>& coop_result) const;

            std::vector<double> get_run_times() const;

            // std::shared_ptr<Syft::VarMgr> get_mgr() const;


            // std::vector<std::vector<CUDD::BDD>> get_chained_strategies(const std::vector<Syft::SynthesisResult>& adv_results,
            //                                         const std::vector<Syft::SynthesisResult>& coop_results);

            /**
             * @brief Prints a vector of positional strategies which induce the best-effort strategy
             * 
             * @param adv_results vector of solutions to adversarial DFA games
             * @param coop_results vector of solutions to cooperative DFA games
            */
            // void merge_and_dump_dots(const std::vector<Syft::SynthesisResult>& adv_results,
            //                         const std::vector<Syft::SynthesisResult>& coop_results) const;



            // prints realizability of synthesis for goal under each env specs
            // void merge_and_dump_dot(const std::vector<Syft::SynthesisResult>& adv_results,
            //                         const std::vector<Syft::SynthesisResult>& coop_results,
            //                         const std::string& outfile) const;

    };

}
#endif // SYFT_ENVIRONMENTCHAINBESTEFFORTSYNTHESIZER_H
