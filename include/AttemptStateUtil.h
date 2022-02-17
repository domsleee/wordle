#pragma once
#include <fstream>
#include <string>



struct AttemptStateUtil {
    template <class T>
    static void readFromFile(T& res, const std::string &filename) {
        std::ifstream fin{filename};
        if (!fin.good()) {
            DEBUG("bad filename " << filename);
            exit(1);
        }
        uint64_t size;
        fin >> size;
        for (uint64_t i = 0; i < size;) {
            uint8_t byte;
            uint64_t ct = 0, j;
            fin >> byte;
            for (j = i; j < std::min(i + 8, size); ++j, ++ct) {
                res[j] = (byte >> ct) & 0b1;
            }
            i = j;
        }
        fin.close();
    }

    template <class T>
    static void writeToFile(const T& data, uint64_t size, const std::string &filename) {
        std::ofstream fout{filename};
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
        for (auto b: outBytes) fout << b;
        fout.close();
    }
};
