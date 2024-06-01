#ifndef SYFT_COMMONCORECHAINSYNTHESIZER_H
#define SYFT_COMMONCORECHAINSYNTHESIZER_H

#include<string>
#include<vector>
#include<utility>
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

    class CommonCoreChainSynthesizer {

        protected:
            std::shared_ptr<VarMgr> var_mgr_;

            std::string ltlf_goal_;
            std::string env_core_; // E_{c}
            std::vector<std::string> conjuncts_; // E'_{i}

            Player starting_player_;

            std::vector<std::vector<SymbolicStateDfa>> symbolic_dfas_;
            std::vector<SymbolicStateDfa> arena_;
            std::vector<SymbolicStateDfa> restricted_arena_;

            Syft::InputOutputPartition partition_;

            std::vector<double> run_times_;

        public:

            CommonCoreChainSynthesizer(
                std::shared_ptr<Syft::VarMgr> var_mgr,
                std::string ltlf_goal,
                std::string env_core,
                std::vector<std::string> conjuncts,
                Syft::InputOutputPartition partition,
                Syft::Player starting_player
            );

            virtual ChainedStrategies run() final;

            void realizability(const std::vector<SynthesisResult>& adv_results,
                                 const std::vector<SynthesisResult>& coop_results) const;

            std::vector<double> get_run_times() const;
    };


} 
#endif