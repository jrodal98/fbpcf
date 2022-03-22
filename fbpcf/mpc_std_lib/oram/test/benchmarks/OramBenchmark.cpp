/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <folly/Benchmark.h>

#include "common/init/Init.h"

#include "fbpcf/engine/util/test/benchmarks/BenchmarkHelper.h"
#include "fbpcf/engine/util/test/benchmarks/NetworkedBenchmark.h"
#include "fbpcf/mpc_std_lib/oram/DifferenceCalculatorFactory.h"
#include "fbpcf/mpc_std_lib/oram/IDifferenceCalculatorFactory.h"
#include "fbpcf/mpc_std_lib/oram/ISinglePointArrayGenerator.h"
#include "fbpcf/mpc_std_lib/oram/ObliviousDeltaCalculatorFactory.h"
#include "fbpcf/mpc_std_lib/oram/SinglePointArrayGeneratorFactory.h"
#include "fbpcf/mpc_std_lib/util/test/util.h"
#include "fbpcf/scheduler/IScheduler.h"
#include "fbpcf/scheduler/SchedulerHelper.h"

namespace fbpcf::mpc_std_lib::oram {

const int8_t indicatorWidth = 16;

class DifferenceCalculatorBenchmark : public engine::util::NetworkedBenchmark {
 public:
  void setup() override {
    auto [agentFactory0, agentFactory1] =
        engine::util::getSocketAgentFactories();
    agentFactory0_ = std::move(agentFactory0);
    agentFactory1_ = std::move(agentFactory1);

    auto [input0, input1, _] =
        util::generateRandomInputs<uint32_t, indicatorWidth>(batchSize_);
    input0_ = input0;
    input1_ = input1;
  }

 protected:
  void initSender() override {
    scheduler::SchedulerKeeper<0>::setScheduler(
        scheduler::createLazySchedulerWithRealEngine(0, *agentFactory0_));
    DifferenceCalculatorFactory<uint32_t, indicatorWidth, 0> factory(
        true, 0, 1);
    sender_ = factory.create();
  }

  void runSender() override {
    sender_->calculateDifferenceBatch(
        input0_.indicatorShares,
        input0_.minuendShares,
        input0_.subtrahendShares);
  }

  void initReceiver() override {
    scheduler::SchedulerKeeper<1>::setScheduler(
        scheduler::createLazySchedulerWithRealEngine(1, *agentFactory1_));
    DifferenceCalculatorFactory<uint32_t, indicatorWidth, 1> factory(
        false, 0, 1);
    receiver_ = factory.create();
  }

  void runReceiver() override {
    receiver_->calculateDifferenceBatch(
        input1_.indicatorShares,
        input1_.minuendShares,
        input1_.subtrahendShares);
  }

  std::pair<uint64_t, uint64_t> getTrafficStatistics() override {
    return scheduler::SchedulerKeeper<0>::getTrafficStatistics();
  }

 private:
  size_t batchSize_ = 16384;

  std::unique_ptr<engine::communication::IPartyCommunicationAgentFactory>
      agentFactory0_;
  std::unique_ptr<engine::communication::IPartyCommunicationAgentFactory>
      agentFactory1_;

  std::unique_ptr<IDifferenceCalculator<uint32_t>> sender_;
  std::unique_ptr<IDifferenceCalculator<uint32_t>> receiver_;

  util::InputType<uint32_t> input0_;
  util::InputType<uint32_t> input1_;
};

BENCHMARK_COUNTERS(DifferenceCalculator_Benchmark, counters) {
  DifferenceCalculatorBenchmark benchmark;
  benchmark.runBenchmark(counters);
}

class ObliviousDeltaCalculatorBenchmark
    : public engine::util::NetworkedBenchmark {
 public:
  void setup() override {
    auto [agentFactory0, agentFactory1] =
        engine::util::getSocketAgentFactories();
    agentFactory0_ = std::move(agentFactory0);
    agentFactory1_ = std::move(agentFactory1);

    auto [input0, input1, _] =
        util::generateObliviousDeltaCalculatorInputs(batchSize_);
    input0_ = input0;
    input1_ = input1;
  }

 protected:
  void initSender() override {
    scheduler::SchedulerKeeper<0>::setScheduler(
        scheduler::createLazySchedulerWithRealEngine(0, *agentFactory0_));
    ObliviousDeltaCalculatorFactory<0> factory(true, 0, 1);
    sender_ = factory.create();
  }

  void runSender() override {
    sender_->calculateDelta(
        input0_.delta0Shares, input0_.delta1Shares, input0_.alphaShares);
  }

  void initReceiver() override {
    scheduler::SchedulerKeeper<1>::setScheduler(
        scheduler::createLazySchedulerWithRealEngine(1, *agentFactory1_));
    ObliviousDeltaCalculatorFactory<1> factory(false, 0, 1);
    receiver_ = factory.create();
  }

  void runReceiver() override {
    receiver_->calculateDelta(
        input1_.delta0Shares, input1_.delta1Shares, input1_.alphaShares);
  }

  std::pair<uint64_t, uint64_t> getTrafficStatistics() override {
    return scheduler::SchedulerKeeper<0>::getTrafficStatistics();
  }

 private:
  size_t batchSize_ = 16384;

  std::unique_ptr<engine::communication::IPartyCommunicationAgentFactory>
      agentFactory0_;
  std::unique_ptr<engine::communication::IPartyCommunicationAgentFactory>
      agentFactory1_;

  std::unique_ptr<IObliviousDeltaCalculator> sender_;
  std::unique_ptr<IObliviousDeltaCalculator> receiver_;

  util::ObliviousDeltaCalculatorInputType input0_;
  util::ObliviousDeltaCalculatorInputType input1_;
};

BENCHMARK_COUNTERS(ObliviousDeltaCalculator_Benchmark, counters) {
  ObliviousDeltaCalculatorBenchmark benchmark;
  benchmark.runBenchmark(counters);
}

class SinglePointArrayGeneratorBenchmark
    : public engine::util::NetworkedBenchmark {
 public:
  void setup() override {
    auto [agentFactory0, agentFactory1] =
        engine::util::getSocketAgentFactories();
    agentFactory0_ = std::move(agentFactory0);
    agentFactory1_ = std::move(agentFactory1);

    size_t width = std::ceil(std::log2(length_));
    size_t batchSize = 128;

    party0Input_ =
        std::vector<std::vector<bool>>(width, std::vector<bool>(batchSize));
    party1Input_ =
        std::vector<std::vector<bool>>(width, std::vector<bool>(batchSize));
    for (size_t i = 0; i < batchSize; i++) {
      auto [share0, share1, _] =
          util::generateSharedRandomBoolVectorForSinglePointArrayGenerator(
              length_);
      for (size_t j = 0; j < width; j++) {
        party0Input_[j][i] = share0.at(j);
        party1Input_[j][i] = share1.at(j);
      }
    }
  }

 protected:
  void initSender() override {
    scheduler::SchedulerKeeper<0>::setScheduler(
        scheduler::createLazySchedulerWithRealEngine(0, *agentFactory0_));
    SinglePointArrayGeneratorFactory factory(
        true, std::make_unique<ObliviousDeltaCalculatorFactory<0>>(true, 0, 1));
    sender_ = factory.create();
  }

  void runSender() override {
    sender_->generateSinglePointArrays(party0Input_, length_);
  }

  void initReceiver() override {
    scheduler::SchedulerKeeper<1>::setScheduler(
        scheduler::createLazySchedulerWithRealEngine(1, *agentFactory1_));
    SinglePointArrayGeneratorFactory factory(
        false,
        std::make_unique<ObliviousDeltaCalculatorFactory<1>>(false, 0, 1));
    receiver_ = factory.create();
  }

  void runReceiver() override {
    receiver_->generateSinglePointArrays(party1Input_, length_);
  }

  std::pair<uint64_t, uint64_t> getTrafficStatistics() override {
    return scheduler::SchedulerKeeper<0>::getTrafficStatistics();
  }

 private:
  size_t length_ = 16384;

  std::unique_ptr<engine::communication::IPartyCommunicationAgentFactory>
      agentFactory0_;
  std::unique_ptr<engine::communication::IPartyCommunicationAgentFactory>
      agentFactory1_;

  std::unique_ptr<ISinglePointArrayGenerator> sender_;
  std::unique_ptr<ISinglePointArrayGenerator> receiver_;

  std::vector<std::vector<bool>> party0Input_;
  std::vector<std::vector<bool>> party1Input_;
};

BENCHMARK_COUNTERS(SinglePointArrayGenerator_Benchmark, counters) {
  SinglePointArrayGeneratorBenchmark benchmark;
  benchmark.runBenchmark(counters);
}
} // namespace fbpcf::mpc_std_lib::oram

int main(int argc, char* argv[]) {
  facebook::initFacebook(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}