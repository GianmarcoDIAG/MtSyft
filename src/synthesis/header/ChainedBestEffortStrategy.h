#include<vector>
#include<cuddObj.hh>

namespace Syft {

    class ChainedBestEffortStrategy {

        protected:
            std::vector<std::vector<CUDD::BDD>> strategies_;
            int valid;

        public:
            ChainedBestEffortStrategy(const std::vector<std::vector<CUDD::BDD>>& strategies): strategies_(strategies), valid(0) {}

            void update() {++valid;}

            std::vector<int> Eval(std::vector<int>& state_vec ) const {                
                std::vector<int> result;
                for (const auto& bdd : strategies_[valid]) {
                    result.push_back(bdd.Eval(state_vec.data()).IsOne());
                }
                return result;
            }

            void dump_strategies() const;
    };
}