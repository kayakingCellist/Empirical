#include <iostream>

#include "../../base/vector.h"
#include "../../tools/Random.h"


struct Org {
  double x;
  double y;
  bool coop;
  double fitness;

  emp::vector<size_t> neighbors;
};

// Create a class to maintain a simple Prisoner's Dilema world.
class SimplePDWorld {
private:
  // Parameters
  double r = 0.02;    // Neighborhood radius
  double u = 0.025;   // cost / benefit ratio
  size_t N = 6400;    // Population size
  size_t E = 5000;     // How many epochs should a popuilation run for?

  emp::Random random;

  // Calculations we'll need later.
  double r_sqr;          // r squared (for comparisons)
  emp::vector<Org> pop;

  double payoff_CC;
  double payoff_CD;
  double payoff_DC;
  double payoff_DD;

  // Helper functions
  void CalcFitness(size_t id);
  void Repro();
public:
  SimplePDWorld(double _r=0.02, double _u=0.025, size_t _N=6400, size_t _E=5000, int seed=0)
   : random(seed)
  {
    Setup(_r, _u, _N, _E);
  }

  void Setup(double _r=0.02, double _u=0.0025, size_t _N=6400, size_t _E=5000) {
    // Store the input values.
    r = _r;
    u = _u;
    N = _N;
    E = _E;

    // Calculations we'll need later.
    r_sqr = r * r;  // r squared (for comparisons)
    pop.resize(N);

    payoff_CC = 1.0;
    payoff_CD = 0.0;
    payoff_DC = 1.0 + u;
    payoff_DD = u;

    // Initialize each organism
    for (Org & org : pop) {
      org.x = random.GetDouble(1.0);
      org.y = random.GetDouble(1.0);
      org.coop = random.P(0.5);
      org.neighbors.resize(0);
    }

    // Determine if pairs of organisms are neighbors;
    for (size_t i = 1; i < N; i++) {
      for (size_t j = 0; j < i; j++) {
        Org & org1 = pop[i];
        Org & org2 = pop[j];
        double x_dist = org1.x - org2.x;
        double y_dist = org1.y - org2.y;
        double dist_sqr = x_dist * x_dist + y_dist * y_dist;

        // Test if this pair are within neighbor radius...
        if (dist_sqr < r_sqr) {
          org1.neighbors.push_back(j);
          org2.neighbors.push_back(i);
        }
      }
    }

    // Calculate the initial fitness for each organism in the population.
    for (size_t id = 0; id < N; id++) {
      CalcFitness(id);
    }
  }

  void Reset(double r, double u, size_t N, size_t E) { Setup(r,u,N,E); }

  void Run() {
    // Run the organisms!
    for (size_t epoch = 0; epoch < E; epoch++) {
      std::cout << "Epoch = " << epoch
  	      << ";  #Coop = " << CountCoop()
  	      << std::endl;
      for (size_t o = 0; o < N; o++) Repro();
    }
  }

  size_t CountCoop();
  void PrintNeighborInfo();
};



void SimplePDWorld::CalcFitness(size_t id) {
  Org & org = pop[id];
  size_t num_neighbors = org.neighbors.size();

  int C_count = 0;
  int D_count = 0;
  for (size_t n : org.neighbors) {
    if (pop[n].coop) C_count++;
    else D_count++;
  }

  double C_value = payoff_CC;
  double D_value = payoff_CD;

  if (!org.coop) {
    C_value = payoff_DC;
    D_value = payoff_DD;
  }

  double total_C = C_value * (double) C_count;
  double total_D = D_value * (double) D_count;
  org.fitness = total_C + total_D;

//   std::cout << "total_C = " << total_C
//   	    << "  total_D = " << total_D
//   	    << "  num_neighbors = " << num_neighbors
//   	    << " (" << C_count << " C and " << D_count << " D)"
//   	    << "  self = " << (org.coop ? 'C' : 'D')
// 	    << "  fitness = " << org.fitness
//   	    << std::endl;
}


// Reproduce into a single, random cell.
void SimplePDWorld::Repro() {
  size_t id = random.GetUInt(N);
  Org & org = pop[id];

  // Determine the total fitness of neighbors.
  double total_fitness = 0.0;
  for (size_t n : org.neighbors) {
    total_fitness += pop[n].fitness;
  }

  // If neighbor fitnesses are non-zero, choose one of them.
  if (total_fitness) {
    double choice = random.GetDouble(total_fitness);
    for (size_t n : org.neighbors) {
      if (choice < pop[n].fitness) {
	org.coop = pop[n].coop;   // Copy strategy of winner!
	break;
      }
      choice -= pop[n].fitness;
    }
  }

  // Now that we have updated the organism, calculate its fitness again
  // (even if no change, since neighbors may have changed).
  CalcFitness(id);
}


size_t SimplePDWorld::CountCoop() {
  size_t count = 0.0;
  for (Org & org : pop) if (org.coop) count++;
  return count;
}

void SimplePDWorld::PrintNeighborInfo() {
  size_t total = 0;
  size_t max_size = 0;
  size_t min_size = pop[0].neighbors.size();
  for (Org & org : pop) {
    size_t cur_size = org.neighbors.size();
    total +=cur_size;
    if (cur_size > max_size) max_size = cur_size;
    if (cur_size < min_size) min_size = cur_size;
  }
  emp::vector<int> hist(max_size+1, 0);
  for (Org & org : pop) {
    size_t cur_size = org.neighbors.size();
    hist[cur_size]++;
  }
  double avg_size = ((double) total) / (double) N;
  std::cout << "Average neighborhood size = " << avg_size << std::endl
	    << "Min size = " << min_size
	    << "   Max size = " << max_size << std::endl;
  for (size_t i = 0; i < hist.size(); i++) {
    std::cout << i << " : " << hist[i] << std::endl;
  }
}



#include "../../config/ArgManager.h"

EMP_BUILD_CONFIG( PDWorldConfig,
  GROUP(DEFAULT, "Default settings for SimplePDWorld"),
  VALUE(SEED, int, 0, "Random number seed (0 for based on time)"),
  VALUE(r, double, 0.02, "Neighborhood radius, in fraction of world."),
  VALUE(u, double, 0.0025, "cost / benefit ratio"),
  VALUE(N, size_t, 6400, "Number of organisms in the popoulation."),
  VALUE(E, size_t, 5000, "How many epochs should we process?"),
)

int main(int argc, char * argv[])
{
  PDWorldConfig config;
  config.Read("PDWorld.cfg");

  auto args = emp::cl::ArgManager(argc, argv);
  if (args.ProcessConfigOptions(config, std::cout, "PDWorld.cfg") == false) exit(1);
  if (args.TestUnknown() == false) exit(2);  // If there are leftover args, throw an error.

  SimplePDWorld world(config.r(), config.u(), config.N(), config.E(), config.SEED());
  world.Run();

  // Print extra info?
  world.PrintNeighborInfo();

  return 0;
}
