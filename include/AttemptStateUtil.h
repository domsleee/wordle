#pragma once
#include <fstream>
#include <string>
#include <filesystem>

struct AttemptStateUtil {
    template <class T>
    static void readFromFile(T& res, uint64_t inputSize, const std::string &filename) {
        std::ifstream fin(filename, std::ios::binary | std::ios::in);
        if (!fin.good()) {
            DEBUG("bad filename " << filename);
            exit(1);
        }
        uint64_t size;
        fin >> size;
        if (size != inputSize) {
            DEBUG("Expected size " << inputSize << " does not match actual size " << size);
            exit(1);
        }
        uint64_t numBytes = (size+7)/8;
        std::vector<uint8_t> inBytes(numBytes);

        DEBUG("reading " << numBytes << " bytes");
        fin.read((char*)inBytes.data(), numBytes);

        DEBUG("loading bytes into T");
        for (uint64_t i = 0; i < size;) {
            uint8_t byte = inBytes[i/(int64_t)8];
            uint64_t ct = 0, j;
            for (j = i; j < std::min(i + 8, size); ++j, ++ct) {
                res[j] = (byte >> ct) & 0b1;
            }
            i = j;
        }
        fin.close();
    }

    template <class T>
    static void writeToFile(const T& data, uint64_t size, const std::string &filename) {
        std::ofstream fout(filename, std::ios::binary | std::ios::out);
        std::vector<uint8_t> outBytes;
        fout << size;
        for (uint64_t i = 0; i < size;) {
            if (i%10000000 == 0) DEBUG("writeToFile: " << getPerc(i, size));
            uint64_t ct = 0, j;
            uint8_t byte = 0;
            for (j = i; j < std::min(i + 8, size); ++j, ++ct) {
                byte |= (data[j] << ct);
            }
            outBytes.push_back(byte);
            i = j;
        }
        DEBUG("writing " << outBytes.size() << " bytes...");
        fout.write((char*)outBytes.data(), outBytes.size());
        fout.close();
    }
};
