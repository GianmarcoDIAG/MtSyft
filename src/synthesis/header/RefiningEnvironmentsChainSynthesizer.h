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

    class RefiningEnvironmentsChainSynthesizer {

        protected:
            std::shared_ptr<VarMgr> var_mgr_;

            std::string ltlf_goal_;
            std::vector<std::string> refinements_; // E'_{1}, E'_{2}, ..., E'_{n-1}
            std::string env_base_; // E_{n}

            Player starting_player_;

            std::vector<SymbolicStateDfa> symbolic_dfas_;
            std::vector<SymbolicStateDfa> arena_;

            Syft::InputOutputPartition partition_;

            std::vector<double> run_times_;

        public:

            RefiningEnvironmentsChainSynthesizer(
                std::shared_ptr<Syft::VarMgr> var_mgr,
                std::string ltlf_goal,
                std::vector<std::string> refinements,
                std::string env_base,
                Syft::InputOutputPartition partition,
                Syft::Player starting_player
            );

            virtual ChainedStrategies run() final;

            void realizability(const std::vector<SynthesisResult>& adv_results,
                                const std::vector<SynthesisResult>& coop_results) const;

            std::vector<double> get_run_times() const;

    };

}