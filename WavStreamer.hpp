#ifndef WAVSTREAMER_HPP
#define WAVSTREAMER_HPP

#include <fstream>
#include <mutex>
#include <vector>
extern int gMin, gMax;

class WavStreamer {
public:
  static void init(const std::string &filename, bool makeJSON);
  static void close(bool makeJSON);
  static void writeTones(int val1, int val2);
  static void logState(const std::string &jsonState);
  static void logComparison(int val1, int val2);
  static void logStragglerInsert(int value);

private:
  static std::mutex wavMutex;
  static std::ofstream wavFile;
  static uint32_t totalSamples;

  template <typename T>
  static void writeBinary(std::ofstream &out, const T &value) {
    out.write(reinterpret_cast<const char *>(&value), sizeof(T));
  }

  static double valueToFreq(int value);
};

class WavCapture {
public:
  WavCapture() : count(0) {}

  ~WavCapture() {
    if (count == 2) {
      WavStreamer::writeTones(values[0], values[1]);
      WavStreamer::logComparison(values[0], values[1]);
    }
  }

  WavCapture &operator<<(int val) {
    if (count < 2) {
      values[count++] = val;
    }
    return *this;
  }

private:
  int values[2];
  int count;
};

#define WAVFILE WavCapture()

#endif // WAVSTREAMER_HPP
