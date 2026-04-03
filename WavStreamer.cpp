#include "WavStreamer.hpp"
#include <cmath>
#include <iostream>
#include <math.h>

std::ofstream jsonLogFile;
std::mutex WavStreamer::wavMutex;
std::ofstream WavStreamer::wavFile;
uint32_t WavStreamer::totalSamples = 0;

const int SAMPLE_RATE = 44100;
const double PI = 3.14159265358979323846;
const double NOTE_DURATION = 0.05;

void WavStreamer::init(const std::string &filename) {
  std::lock_guard<std::mutex> lock(wavMutex);
  wavFile.open(filename, std::ios::binary);

  if (!wavFile.is_open()) {
    std::cerr << "Failed to open WAV file!" << std::endl;
    return;
  }

  //.wav header:
  wavFile.write("RIFF", 4);
  writeBinary<uint32_t>(wavFile, 0);
  wavFile.write("WAVE", 4);
  wavFile.write("fmt ", 4);
  writeBinary<uint32_t>(wavFile, 16);
  writeBinary<uint16_t>(wavFile, 1);
  writeBinary<uint16_t>(wavFile, 2);
  writeBinary<uint32_t>(wavFile, SAMPLE_RATE);
  writeBinary<uint32_t>(wavFile, SAMPLE_RATE * 2 * 2);
  writeBinary<uint16_t>(wavFile, 4);
  writeBinary<uint16_t>(wavFile, 16);
  wavFile.write("data", 4);
  writeBinary<uint32_t>(wavFile, 0);

  jsonLogFile.open("events.json");
  jsonLogFile << "[\n";
}

void WavStreamer::writeTones(int val1, int val2) {
  std::lock_guard<std::mutex> lock(wavMutex);
  if (!wavFile.is_open())
    return;

  double freqL = valueToFreq(val1);
  double freqR = valueToFreq(val2);

  int samplesToWrite = SAMPLE_RATE * NOTE_DURATION;

  int attackSamples = SAMPLE_RATE * 0.005;
  int releaseSamples = SAMPLE_RATE * 0.015;

  short maxAmplitude = 16000;

  for (int i = 0; i < samplesToWrite; ++i) {
    double t = (double)i / SAMPLE_RATE;

    double envelope = 1.0;

    if (i < attackSamples) {
      envelope = (double)i / attackSamples;
    } else if (i > samplesToWrite - releaseSamples) {
      envelope = (double)(samplesToWrite - i) / releaseSamples;
    }

    short currentAmplitude = (short)(maxAmplitude * envelope);

    short left = (short)(currentAmplitude * std::sin(2.0 * PI * freqL * t));
    short right = (short)(currentAmplitude * std::sin(2.0 * PI * freqR * t));

    writeBinary<short>(wavFile, left);
    writeBinary<short>(wavFile, right);
  }

  totalSamples += samplesToWrite;
}

void WavStreamer::close() {
  std::lock_guard<std::mutex> lock(wavMutex);
  if (!wavFile.is_open())
    return;

  uint32_t dataSize = totalSamples * 2 * 2;
  uint32_t chunkSize = 36 + dataSize;

  wavFile.seekp(4, std::ios::beg);
  writeBinary<uint32_t>(wavFile, chunkSize);
  wavFile.seekp(40, std::ios::beg);
  writeBinary<uint32_t>(wavFile, dataSize);
  wavFile.close();
  jsonLogFile << "{}\n]\n";
  jsonLogFile.close();
}

double WavStreamer::valueToFreq(int value) {
  static const int PENTATONIC_MAP[12] = {0, 0, 2, 2, 4, 4, 4, 7, 7, 9, 9, 0};

  if (value < gMin)
    value = gMin;
  if (value > gMax)
    value = gMax;

  double normalized = (double)(value - gMin) / (gMax - gMin);

  int min_midi = 36; // C1
  int max_midi = 96; // C6
  int midi_note = min_midi + (int)(normalized * (max_midi - min_midi));

  int octave = midi_note / 12;
  int note_in_octave = midi_note % 12;
  int snapped_note = (octave * 12) + PENTATONIC_MAP[note_in_octave];

  if (note_in_octave == 11) {
    snapped_note += 12;
  }

  double frequency = 440.0 * pow(2.0, (snapped_note - 69.0) / 12.0);
  return frequency;
}

void WavStreamer::logComparison(int val1, int val2) {
  if (jsonLogFile.is_open()) {
    jsonLogFile << "{\"type\": \"compare\", \"left\": " << val1
                << ", \"right\": " << val2 << "},\n";
  }
}

void WavStreamer::logStragglerInsert(int value) {
  if (jsonLogFile.is_open()) {
    jsonLogFile << "{\"type\": \"straggler_insert\", \"value\": " << value
                << "},\n";
  }
}

void WavStreamer::logState(const std::string &jsonState) {
  if (jsonLogFile.is_open()) {
    jsonLogFile << "{\"type\": \"state_update\", \"data\": " << jsonState
                << "},\n";
  }
}
