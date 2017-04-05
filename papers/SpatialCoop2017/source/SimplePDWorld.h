#include "base/vector.h"
#include "tools/Random.h"

struct Org {
  double x;
  double y;
  bool coop;
  double fitness;

  emp::vector<size_t> neighbors;
};

// Create a class to maintain a simple Prisoner's Dilema world.
class SimplePDWorld {
public:
  // Parameters
  double r;         // Neighborhood radius
  double u;         // cost / benefit ratio
  size_t N;         // Population size
  size_t E;         // How many epochs should a popuilation run for?
  size_t num_runs;  // How many runs should we do?

  emp::Random random;
  size_t epoch;

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
  SimplePDWorld(double _r=0.02, double _u=0.175, size_t _N=6400, size_t _E=5000, int seed=0)
    : num_runs(10), random(seed)
  {
    Setup(_r, _u, _N, _E);
  }

  const emp::vector<Org> & GetPop() const { return pop; }
  double GetR() const { return r; }
  double GetU() const { return u; }
  size_t GetN() const { return N; }
  size_t GetE() const { return E; }
  size_t GetNumRuns() const { return num_runs; }
  size_t GetEpoch() const { return epoch; }

  void SetR(double _r) { r = _r; }
  void SetU(double _u) { u = _u; }
  void SetN(size_t _N) { N = _N; }
  void SetE(size_t _E) { E = _E; }
  void SetNumRuns(size_t n) { num_runs = n; }

  void Setup(double _r=0.02, double _u=0.0025, size_t _N=6400, size_t _E=5000) {
    // Store the input values.
    r = _r;
    u = _u;
    N = _N;
    E = _E;
    epoch = 0;

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

  void Reset() { Setup(r,u,N,E); }

  void Run(size_t steps=-1) {
    if (steps > E) steps = E;
    // Run the organisms!
    size_t end_epoch = epoch + steps;
    while (epoch < end_epoch) {
      for (size_t o = 0; o < N; o++) Repro();
      epoch++;
    }
  }

  size_t CountCoop();
  void PrintNeighborInfo(std::ostream & os);
};



void SimplePDWorld::CalcFitness(size_t id) {
  Org & org = pop[id];

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

void SimplePDWorld::PrintNeighborInfo(std::ostream & os) {
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

  os << "ave_size";
  for (size_t i = 0; i < hist.size(); i++) os << ',' << i;
  os << '\n';

  os << avg_size;
  for (size_t i = 0; i < hist.size(); i++) os << ',' << hist[i];
  os << '\n';
}