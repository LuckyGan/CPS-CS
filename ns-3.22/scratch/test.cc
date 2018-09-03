#include "ns3/simulator.h"
using namespace ns3;

void test (int i) {
  std::cout << i << std::endl;
}

int main () {
  Simulator::Schedule (Time (1), &test, 1);
  Simulator::Run ();
  return 0;
}

