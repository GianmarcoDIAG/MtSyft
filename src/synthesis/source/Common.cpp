#include "Common.h"
#include <bitset>

struct BestEffortExplicitState {

    // In Best-effort arena states are represented as triples
    int adversarial_state_;
    int negated_env_state_;
    int cooperative_state_;

    BestEffortExplicitState() = default;

    BestEffortExplicitState(const int& adversarial_state, 
                            const int& negated_env_state, 
                            const int& cooperative_state) :
                                                            adversarial_state_(adversarial_state),
                                                            negated_env_state_(negated_env_state),
                                                            cooperative_state_(cooperative_state) {};
};

std::string state2bin(int n){
  std::string res;
    while (n)
    {
        res.push_back((n & 1) + '0');
        n >>= 1;
    }

    if (res.empty())
        res = "0";
    else
        reverse(res.begin(), res.end());
   return res;
}