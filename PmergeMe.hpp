#ifndef PMERGEME_HPP
#define PMERGEME_HPP
#include <charconv>
#include <cstddef>
#include <list>
#include <stdexcept>
#include <string_view>
#include <vector>

extern size_t gComparisons; // for tracking the number of comparisons

class PMergeMe {
public:

    struct vectorNode {
        unsigned int value;
        std::vector<vectorNode> losers;

        vectorNode();
        vectorNode(unsigned int value, std::vector<vectorNode> losers);
        vectorNode(const vectorNode& other) = default;
        vectorNode& operator=(const vectorNode& other) = default;
        ~vectorNode() = default;

        bool operator<(const vectorNode &other) const;
    };

  template <typename Container = std::vector<unsigned int>>
  static Container parseInput(int argc, char *argv[]) {
    Container results;
    if constexpr (requires { results.reserve(1); }) {
      results.reserve(argc - 1);
    }

    for (int i = 1; i < argc; ++i) {
      unsigned int value = 0;
      std::string_view arg = argv[i];

      auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), value);
      if (ec == std::errc() &&              //input was a valid number,
          ptr == arg.data() + arg.size() && //with no trailing characters
          value > 0) {
        results.push_back(value);
      } else {
        std::string m = "Invalid input: " + std::string(arg);
        throw std::invalid_argument(m);
      }
    }
    return results;
  }
  static std::vector<vectorNode> vectorMergeSort(std::vector<vectorNode> &input);

private:
  PMergeMe();
  PMergeMe(const PMergeMe &other);
  PMergeMe &operator=(const PMergeMe &other);
  ~PMergeMe();
};

#endif
