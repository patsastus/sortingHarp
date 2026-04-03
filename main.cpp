#include "PmergeMe.hpp"
#include "WavStreamer.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"
typedef typename std::chrono::time_point<std::chrono::high_resolution_clock>
    TimePoint;
int gMin = 0;
int gMax = 0;
size_t getFJComparisonLimit(size_t n) {
  // the theoretical limit is: sum (ceil (log2 (3k/4) ) ), k = 1...n
  size_t total = 0;
  for (size_t k = 1; k <= n; ++k) {
    // ceil(log2(3k / 4)) <==> first m where 2^m > 3k, and then subtract 2 from
    // m (because divided by 2^2), starting at 2^0 = 1
    size_t target = 3 * k, power = 1, m = 0;
    while (power < target) {
      power *= 2;
      ++m;
    }
    total += (m >= 2) ? (m - 2) : 0;
  }
  return total;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <at least 2 unsigned integers>"
              << std::endl;
    return 1;
  }
  WavStreamer::init("ford_johnson_sort.wav");
  std::vector<PMergeMe::vectorNode> copy;
  try {
    std::vector<unsigned int> inputVector = PMergeMe::parseInput<>(argc, argv);
    gMin = static_cast<int>(
        *std::min_element(inputVector.begin(), inputVector.end()));
    gMax = static_cast<int>(
        *std::max_element(inputVector.begin(), inputVector.end()));
    // move values over to my custom struct
    std::vector<PMergeMe::vectorNode> inputNodes;
    inputNodes.reserve(inputVector.size());
    for (size_t i = 0; i < inputVector.size(); ++i) {
      inputNodes.push_back({inputVector[i], {}});
    }
    copy = inputNodes;
    std::cout << "Input: ";
    for (unsigned int i : inputVector) {
      std::cout << i << " ";
    }
    std::cout << std::endl;
    inputNodes = PMergeMe::vectorMergeSort(inputNodes);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  WavStreamer::close();
  std::cout
      << "Performed " << gComparisons
      << " comparisons during merge-insert sorting. Worst-case allowance :"
      << getFJComparisonLimit(static_cast<size_t>(argc - 1)) << std::endl;

  // do the sorting with standard sort
  WavStreamer::init("standard_sort.wav");
  gComparisons = 0;
  std::sort(copy.begin(), copy.end());
  WavStreamer::close();

  std::cout << "Performed " << gComparisons
            << " comparisons during standard sorting." << std::endl;
}
