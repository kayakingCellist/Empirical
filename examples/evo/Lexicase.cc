//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  This file explores the template defined in evo::Population.h with an NK landscape.

#include <iostream>

#include "../../config/ArgManager.h"
#include "../../evo/NK.h"
#include "../../evo/World.h"
#include "../../tools/BitSet.h"
#include "../../tools/Random.h"

EMP_BUILD_CONFIG( NKConfig,
  GROUP(DEFAULT, "Default settings for NK model"),
  VALUE(K, uint32_t, 10, "Level of epistasis in the NK model"),
  VALUE(N, uint32_t, 200, "Number of bits in each organisms (must be > K)"), ALIAS(GENOME_SIZE),
  VALUE(SEED, int, 0, "Random number seed (0 for based on time)"),
  VALUE(POP_SIZE, uint32_t, 1000, "Number of organisms in the popoulation."),
  VALUE(MAX_GENS, uint32_t, 2000, "How many generations should we process?"),
  VALUE(MUT_COUNT, uint32_t, 3, "How many bit positions should be randomized?"), ALIAS(NUM_MUTS),
  VALUE(TEST, std::string, "TestString", "This is a test string.")
)


using BitOrg = emp::BitVector;

int main(int argc, char* argv[])
{
  NKConfig config;
  config.Read("Lexicase.cfg");

  auto args = emp::cl::ArgManager(argc, argv);
  if (args.ProcessConfigOptions(config, std::cout, "Lexicase.cfg", "Lexicase-macros.h") == false) {
    exit(0);
  }
  if (args.TestUnknown() == false) exit(0);  // If there are leftover args, throw an error.

  const uint32_t N = config.N();
  const uint32_t K = config.K();
  const uint32_t POP_SIZE = config.POP_SIZE();
  const uint32_t MAX_GENS = config.MAX_GENS();
  const uint32_t MUT_COUNT = config.MUT_COUNT();

  emp::Random random(config.SEED());
  emp::evo::NKLandscape landscape(N, K, random);
  // emp::evo::NKLandscapeMemo landscape(N, K, random);

  emp::evo::EAWorld<BitOrg, emp::evo::FitCacheOff> pop(random, "NKWorld");

  // Build a random initial population
  for (uint32_t i = 0; i < POP_SIZE; i++) {
    BitOrg next_org(N);
    for (uint32_t j = 0; j < N; j++) next_org[j] = random.P(0.5);
    pop.Insert(next_org);
  }

  pop.SetDefaultMutateFun( [MUT_COUNT, N](BitOrg* org, emp::Random& random) {
      for (uint32_t m = 0; m < MUT_COUNT; m++) {
        const uint32_t pos = random.GetUInt(N);
        (*org)[pos] = random.P(0.5);
      }
      return true;
    } );

  std::cout << 0 << " : " << pop[0] << " : " << landscape.GetFitness(pop[0]) << std::endl;

  emp::vector<std::function<double(BitOrg*)>> fit_funs(2);
  fit_funs[0] = [&landscape](BitOrg * org){ return landscape.GetFitness(*org); };
  fit_funs[1] = [](BitOrg * org) { return (double) org->CountOnes(); };
  fit_funs[2] = [](BitOrg * org) { return (double) (org->size() - org->CountOnes()); };

  // Loop through updates
  for (uint32_t ud = 0; ud < MAX_GENS; ud++) {
    pop.LexicaseSelect(fit_funs, POP_SIZE);
    pop.Update();
    std::cout << "Gen " << (ud+1) << " : " << pop[0] << " : " << landscape.GetFitness(pop[0]) << std::endl;
    pop.MutatePop(1);
  }


  // Print out the whole population:
  for (size_t i = 0; i < POP_SIZE; i++) {
    std::cout << "Org " << i << " : " << pop[i] << std::endl;
  }
}
