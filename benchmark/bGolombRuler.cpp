#include <benchmark/benchmark.h>

#include <cassert>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

#include "atlantis/propagation/invariants/absDiff.hpp"
#include "atlantis/propagation/invariants/linear.hpp"
#include "atlantis/propagation/solver.hpp"
#include "atlantis/propagation/violationInvariants/allDifferent.hpp"
#include "atlantis/propagation/violationInvariants/equal.hpp"
#include "atlantis/propagation/violationInvariants/lessThan.hpp"
#include "benchmark.hpp"

namespace atlantis::benchmark {

class GolombRuler : public ::benchmark::Fixture {
 public:
  std::shared_ptr<propagation::Solver> solver;
  std::mt19937 gen;

  size_t markCount{0};

  std::vector<propagation::VarViewId> marks;
  std::vector<propagation::VarViewId> differences;

  std::vector<propagation::VarViewId> violations;
  propagation::VarViewId totalViolation{propagation::NULL_ID};

  void SetUp(const ::benchmark::State& state) override {
    solver = std::make_shared<propagation::Solver>();

    markCount = state.range(0);
    const size_t ub = markCount * markCount;
    std::random_device rd;
    gen = std::mt19937(rd());

    solver->open();
    setSolverMode(*solver, static_cast<int>(state.range(1)));

    // Let first mark equal 0
    marks.emplace_back(solver->makeIntVar(0, 0, 0));
    Int prevVal = 0;
    for (Int i = 1; i < static_cast<Int>(markCount); ++i) {
      const Int markLb = i;
      const Int markUb =
          static_cast<Int>(ub) - (static_cast<Int>(markCount) - i + 1);

      const Int markVal = std::uniform_int_distribution<Int>(
          std::max<Int>(prevVal + 1, markLb), markUb)(gen);

      marks.emplace_back(solver->makeIntVar(markVal, markLb, markUb));
      prevVal = markVal;
    }

    differences.reserve(((markCount - 1) * (markCount)) / 2);

    // creating invariants quadratic in markCount
    // each with two edges, resulting in a total
    // number of edges quadratic in markCount
    for (size_t i = 0; i < markCount; ++i) {
      for (size_t j = i + 1; j < markCount; ++j) {
        solver->makeInvariant<propagation::AbsDiff>(
            *solver,
            differences.emplace_back(
                solver->makeIntVar(0, 0, static_cast<Int>(ub))),
            marks.at(i), marks.at(j));
      }
    }
    assert(differences.size() == ((markCount - 1) * (markCount)) / 2);

    Int maxViol = 0;
    for (propagation::VarViewId viol : differences) {
      maxViol += solver->upperBound(viol);
    }

    // differences must be unique
    totalViolation = solver->makeIntVar(0, 0, maxViol);
    solver->makeViolationInvariant<propagation::AllDifferent>(
        *solver, totalViolation,
        std::vector<propagation::VarViewId>(differences));

    solver->close();
  }

  void TearDown(const ::benchmark::State&) override {
    marks.clear();
    differences.clear();
  }
};

BENCHMARK_DEFINE_F(GolombRuler, probe_single)(::benchmark::State& st) {
  size_t probes = 0;

  std::vector<size_t> indices(marks.size());
  std::iota(indices.begin(), indices.end(), 0);
  const size_t lastIndex = marks.size() - 1;

  for ([[maybe_unused]] const auto& _ : st) {
    for (size_t i = 0; i <= lastIndex; ++i) {
      assert(i <= lastIndex);

      std::swap<size_t>(
          indices[i], indices[rand_in_range(std::min<size_t>(i + 1, lastIndex),
                                            lastIndex, gen)]);
      assert(all_in_range(0, indices.size() - 1, [&](const size_t a) {
        return all_in_range(a + 1, indices.size(),
                            [&](const size_t b) { return a != b; });
      }));
      const size_t index = indices[i];
      assert(index <= lastIndex);

      const Int markLb =
          index == 0
              ? solver->lowerBound(marks[index])
              : std::max<Int>(solver->lowerBound(marks[index]),
                              solver->committedValue(marks[index - 1]) + 1);
      const Int markUb =
          index == lastIndex
              ? solver->upperBound(marks[index])
              : std::min<Int>(solver->upperBound(marks[index]),
                              solver->committedValue(marks[index + 1]) - 1);

      assert(markLb <= markUb);
      assert(solver->lowerBound(marks.at(index)) <= markLb);
      assert(solver->upperBound(marks.at(index)) >= markUb);
      assert(index == 0 ||
             solver->committedValue(marks.at(index - 1)) < markLb);
      assert(index == lastIndex ||
             solver->committedValue(marks.at(index + 1)) > markUb);

      if (markLb == markUb) {
        continue;
      }

      const Int newVal =
          static_cast<Int>(rand_in_range(markLb, markUb - 1, gen));

      solver->beginMove();
      solver->setValue(
          marks[index],
          newVal == solver->committedValue(marks[index]) ? markUb : newVal);
      solver->endMove();

      solver->beginProbe();
      solver->query(totalViolation);
      solver->endProbe();

      ++probes;
      break;
    }
    assert(all_in_range(1, marks.size(), [&](const size_t i) {
      return solver->committedValue(marks.at(i - 1)) <
                 solver->committedValue(marks.at(i)) &&
             solver->currentValue(marks.at(i - 1)) <
                 solver->currentValue(marks.at(i));
    }));
  }
  st.counters["probes_per_second"] = ::benchmark::Counter(
      static_cast<double>(probes), ::benchmark::Counter::kIsRate);
}

//*
BENCHMARK_REGISTER_F(GolombRuler, probe_single)
    ->Unit(::benchmark::kMillisecond)
    ->Apply(defaultArguments);
//*/
}  // namespace atlantis::benchmark
