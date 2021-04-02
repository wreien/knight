#include "ir.hpp"

namespace kn::ir {

  std::vector<eval::Operation> optimise(const parser::Block& block) {
    // TODO: actually optimise
    return { block.begin(), block.end() };
  }

  std::vector<eval::Operation> optimise(const std::vector<parser::Block> &blocks) {
    auto result = std::vector<eval::Operation>{};
    for (const auto& blk : blocks) {
      auto opt = optimise(blk);
      result.insert(result.end(), opt.begin(), opt.end());
    }
    return result;
  }

}
