/*
*
* This file declares the structure of the BestEffortSynthesizer type
* Created by Gianmarco P.
*
*/

#ifndef SYFT_BESTEFFORTSYNTHESIZER_H
#define SYFT_BESTEFFORTSYNTHESIZER_H

#include"ExplicitStateDfaMona.h"
#include"ExplicitStateDfa.h"
#include"SymbolicStateDfa.h"
#include"ReachabilitySynthesizer.h"
#include"CoOperativeReachabilitySynthesizer.h"
#include"InputOutputPartition.h"
#include"spotparser.h"

namespace Syft {

	class BestEffortSynthesizer {
	
		protected:
			std::shared_ptr<Syft::VarMgr> var_mgr_;

			std::string agent_specification_;
			std::string environment_specification_;
			// std::string environment_assumption_;

			Player starting_player_;

			// std::vector<CUDD::BDD> final_states;
			std::vector<SymbolicStateDfa> symbolic_dfas_;
			std::vector<SymbolicStateDfa> arena_;

			InputOutputPartition partition_;
		public:
		
			/**
			* \brief Construct an object for symbolic LTLf/LDLf best-effort synthesis
			* 
			* \param var_mgr Dictionary to store variables of the problem
			* \param agent_specification LTLf/LDLf agent goal specification in Lydia syntax
			* \param environment_assumption LTLf/LDLf environment behavior specification in Lydia syntax
			* \param partition Partitioning of problem variables
			* \param starting_player Player who moves first each turn
			* 
			*/
			
			BestEffortSynthesizer(std::shared_ptr<VarMgr> var_mgr,
									std::string agent_specification,
									std::string environment_specification,
									InputOutputPartition partition,
									Player starting_player);
			
			/**
			* \brief Synthesize a reactive and a cooperative strategy
			*
			* \return Results of reactive and cooperative synthesis as structures (realizability, winning states, transducer)
			*/
			virtual std::pair<SynthesisResult, SynthesisResult> run() final;

			/**
			* \brief Merges two transducers into a best-effort strategy
			*
			* \param adversarial_result Reactive synthesis result
			* \param coopeartive_result Cooperative synthesis result
			* \param filename A path to the output .dot file
			* \return void. Prints implementation function of best-effort strategy into a .dot file
			*/
			void merge_and_dump_dot(const SynthesisResult& adversarial_result, 
									const SynthesisResult& cooperative_result, 
									const string& filename) const;

			/**
			 * @brief Merges two transducers into a best-effort strategy (visualization purposes only)
			 * 
			 * \param adversarial_result Reactive synthesis result
			 * \param cooperative_result Cooperative synthesis result
			 * \param in_file A path to uncolored transducer (as a .dot file)
			 * \param out_file A path to the output .dot colored transducer
			 * \return void. Prints the colored implementation function of best-effort strategy into a .dot file
			 */
			void merge_and_dump_colored_dot(const SynthesisResult& adversarial_result,
											const SynthesisResult& cooperative_result,
											const string& in_file,
											const string& out_file) const;

			/**
			 * @brief Uses the best-effort implementation function to dump the correspnding explicit state transducer
			 * 
			 * \param adversarial_result Reactive Synthesis result
			 * \param cooperative_result Cooperative synthesis result
			 * \param file_name A path to the out explicit state transducer
			 * \return void- Prints the explicit state transducer into a .dot file
			 */
			void merge_and_dump_explicit(const SynthesisResult& adversarial_resut,
										const SynthesisResult& cooperative_result,
										const string& filename);
			
	};
}
#endif // SYFT_BESTEFFORTSYNTHESIZER_H

// REFERENCES
// [1] Benjamin Aminof, Giuseppe De Giacomo, and Sasha Rubin. Best-EffortSynthesis: Doing Your Best Is Not Harder Than Giving Up. In IJCAI, 2021
// [2] Shufang Zhu, Lucas M. Tabajara, Jianwen Li, Geguang Pu, and Moshe Y.Vardi. Symbolic LTLf Synthesis. In IJCAI, 2017.