#include "PmergeMe.hpp"
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include "WavStreamer.hpp"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"
typedef typename std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
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

void runFordJohnsonTests() {
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<unsigned int> dist(1, 1000);

  // The specific sizes to test
  std::vector<size_t> testSizes = {5, 10, 50, 100, 500, 1000, 3000};

  std::cout << "Starting large-scale Ford-Johnson verification with "
            << COLOR_GREEN << "std::vector " << COLOR_RESET << "..."
            << std::endl;

  for (size_t n : testSizes) {
    size_t maxAllowedComparisons = getFJComparisonLimit(n);

    for (int trial = 0; trial < 100; ++trial) {
      std::vector<PMergeMe::vectorNode> input;
      input.reserve(n);
      for (size_t i = 0; i < n; ++i) {
        input.push_back({dist(rng), {}});
      }

      gComparisons = 0; // Reset counter
      std::vector<PMergeMe::vectorNode> sorted = PMergeMe::vectorMergeSort(input);

      // Verify size
      assert(sorted.size() == n && "Elements were lost or duplicated!");

      // Verify sort order, can't use the object comparator directly to not
      // affect count
      for (size_t i = 1; i < sorted.size(); ++i) {
        assert(sorted[i - 1].value <= sorted[i].value &&
               "Vector is not sorted!");
      }
      // Verify optimal comparisons
      assert(gComparisons <= maxAllowedComparisons && "Too many comparisons!");
    }
    std::cout << "Passed N = " << n
              << " (Max allowed comparisons: " << maxAllowedComparisons << ")"
              << std::endl;
  }
  std::cout << std::endl << "All stress tests passed!" << std::endl;
}

void refSorts(int argc, char *argv[]){
    gComparisons = 0;
    std::cout << "Theoretical limit: " << getFJComparisonLimit(argc - 1) <<std::endl;
    TimePoint start = std::chrono::high_resolution_clock::now();
    try {
        std::vector<unsigned int> inputVector =
            PMergeMe::parseInput<std::vector<unsigned int>>(argc, argv);
        std::vector<PMergeMe::vectorNode> inputNodes;
        inputNodes.reserve(inputVector.size());
        for (size_t i = 0; i < inputVector.size(); ++i) {
            inputNodes.push_back({inputVector[i], {}});
        }
        std::sort(inputNodes.begin(), inputNodes.end());
    } catch (const std::exception &e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return;
    }
    TimePoint end = std::chrono::high_resolution_clock::now();
    std::cout << "Time to process a range of " << argc - 1 << " elements with "
              << COLOR_GREEN << "std::vector" << COLOR_RESET << ": ";
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " µs" << std::endl;
    std::cout << "Performed " << gComparisons << " comparisons during sorting." << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <at least 2 unsigned integers>"
              << std::endl;
    if (argc == 1){
      //runFordJohnsonTests();
    }
    return 1;
  }
  WavStreamer::init("ford_johnson_sort.wav");
  TimePoint startVec = std::chrono::high_resolution_clock::now();
  try {
    std::vector<unsigned int> inputVector = PMergeMe::parseInput<>(argc, argv);
    gMin = static_cast<int>(*std::min_element(inputVector.begin(), inputVector.end()));
    gMax = static_cast<int>(*std::max_element(inputVector.begin(), inputVector.end()));
    // move values over to my custom struct
    std::vector<PMergeMe::vectorNode> inputNodes;
    inputNodes.reserve(inputVector.size());
    for (size_t i = 0; i < inputVector.size(); ++i) {
      inputNodes.push_back({inputVector[i], {}});
    }

    std::cout << "Before: ";
    for (unsigned int i : inputVector) {
      std::cout << i << " ";
    }
    std::cout << std::endl;
    inputNodes = PMergeMe::vectorMergeSort(inputNodes);
    std::cout << "After: ";
    for (PMergeMe::vectorNode i : inputNodes) {
      std::cout << i.value << " ";
    }
    std::cout << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  TimePoint endVec = std::chrono::high_resolution_clock::now();
  WavStreamer::close();

  WavStreamer::init("standard_sort.wav");
  try {
    std::vector<unsigned int> inputVector = PMergeMe::parseInput<>(argc, argv);
    gMin = static_cast<int>(*std::min_element(inputVector.begin(), inputVector.end()));
    gMax = static_cast<int>(*std::max_element(inputVector.begin(), inputVector.end()));
    // move values over to my custom struct
    std::vector<PMergeMe::vectorNode> inputNodes;
    inputNodes.reserve(inputVector.size());
    for (size_t i = 0; i < inputVector.size(); ++i) {
      inputNodes.push_back({inputVector[i], {}});
    }

    std::cout << "Before: ";
    for (unsigned int i : inputVector) {
      std::cout << i << " ";
    }
    std::cout << std::endl;
    inputNodes = PMergeMe::vectorMergeSort(inputNodes);
    std::cout << "After: ";
    for (PMergeMe::vectorNode i : inputNodes) {
      std::cout << i.value << " ";
    }
    std::cout << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  WavStreamer::close();

  std::cout << "Time to process a range of " << argc - 1 << " elements with "
            << COLOR_GREEN << "std::vector" << COLOR_RESET << ": ";
  std::cout << std::chrono::duration_cast<std::chrono::microseconds>(endVec - startVec).count() << " µs" << std::endl;
  std::cout << "Performed " << gComparisons << " comparisons during sorting. Worst-case allowance :" << getFJComparisonLimit(static_cast<size_t>(argc -1)) << std::endl;
}
