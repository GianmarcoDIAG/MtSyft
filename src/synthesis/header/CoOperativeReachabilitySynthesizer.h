#ifndef CO_OP_REACHABILITY_SYNTHESIZER_H
#define CO_OP_REACHABILITY_SYNTHESIZER_H

#include "CoOperativeDfaGameSynthesizer.h"

namespace Syft {

/**
 * \brief A synthesizer for cooperative reachability game given as a symbolic-state DFA.
 */
class CoOperativeReachabilitySynthesizer : public CoOperativeDfaGameSynthesizer {
 private:

  CUDD::BDD goal_states_;
  CUDD::BDD state_space_;
  
 public:

  /**
   * \brief Construct a synthesizer for the given reachability game.
   *
   * \param spec A symbolic-state DFA representing the reachability game.
   * \param starting_player The player that moves first each turn.
   * \param goal_states The set of states that the agent must reach to win.
   */
  CoOperativeReachabilitySynthesizer(SymbolicStateDfa spec, Player starting_player, Player protagonist_player,
			  CUDD::BDD goal_states, CUDD::BDD state_space);

    /**
     * \brief Solves the reachability game.
     *
     * \return The result consists of
     * realizability
     * a set of agent winning states
     * a transducer representing a winning strategy or nullptr if the game is unrealizable.
     */
  virtual SynthesisResult run() const final;
};

}

#endif // CO_OP_REACHABILITY_SYNTHESIZER_H
