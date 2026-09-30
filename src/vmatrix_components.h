#ifndef CANDELA_VMATRIX_COMPONENTS_H
#define CANDELA_VMATRIX_COMPONENTS_H

#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace VMatrixComponents
{
// Bit positions match ABACUS file order: momentum, nonlocal, meta-GGA.
inline int selection_mask(const std::string& selection)
{
    if (selection == "all") return 7;
    int mask = 0;
    std::size_t begin = 0;
    while (begin < selection.size())
    {
        const std::size_t end = selection.find(',', begin);
        const std::string name = selection.substr(begin, end - begin);
        int bit = 0;
        if (name == "momentum") bit = 1;
        else if (name == "nonlocal") bit = 2;
        else if (name == "mgga") bit = 4;
        else throw std::runtime_error("Unknown vmatrix component: " + name);
        if ((mask & bit) != 0) throw std::runtime_error("Duplicate vmatrix component: " + name);
        mask |= bit;
        if (end == std::string::npos) break;
        begin = end + 1;
        if (begin == selection.size()) throw std::runtime_error("Empty vmatrix component");
    }
    if (mask == 0) throw std::runtime_error("Empty vmatrix component selection");
    return mask;
}

inline void read_record(std::ifstream& input, char* values, const int bytes)
{
    int head = 0;
    int tail = 0;
    input.read(reinterpret_cast<char*>(&head), sizeof(head));
    if (!input || head != bytes) throw std::runtime_error("Invalid vmatrix component record size");
    input.read(values, bytes);
    input.read(reinterpret_cast<char*>(&tail), sizeof(tail));
    if (!input || tail != head) throw std::runtime_error("Truncated or invalid vmatrix component record");
}

inline void read(const std::string& filename, const int nbands, const int mask, double* result)
{
    if (nbands < 1 || mask < 1 || mask > 7) throw std::runtime_error("Invalid vmatrix component request");
    const long long pairs = static_cast<long long>(nbands) * (nbands - 1) / 2;
    const long long bytes = pairs * 6 * sizeof(double);
    if (bytes > std::numeric_limits<int>::max()) throw std::runtime_error("vmatrix component record too large");
    std::ifstream input(filename.c_str(), std::ios::binary);
    if (!input) throw std::runtime_error("Cannot open " + filename);
    int header[3] = {};
    const int header_bytes = sizeof(header);
    read_record(input, reinterpret_cast<char*>(header), header_bytes);
    if (header[0] != 1 || header[1] != nbands || header[2] != 3)
        throw std::runtime_error("Unsupported vmatrix component version, band count or component count");
    const std::size_t values = static_cast<std::size_t>(pairs) * 6;
    std::vector<double> amplitudes(values, 0.0);
    std::vector<double> component(values);
    const int record_bytes = static_cast<int>(bytes);
    for (int index = 0; index < 3; ++index)
    {
        read_record(input, reinterpret_cast<char*>(component.data()), record_bytes);
        if ((mask & (1 << index)) != 0)
        {
            for (std::size_t i = 0; i < values; ++i)
            {
                if (!std::isfinite(component[i])) throw std::runtime_error("Non-finite vmatrix component");
                amplitudes[i] += component[i];
            }
        }
    }
    if (input.peek() != std::ifstream::traits_type::eof())
        throw std::runtime_error("Unexpected data after vmatrix components");
    for (long long pair = 0; pair < pairs; ++pair)
    {
        result[pair] = 0.0;
        for (int coordinate = 0; coordinate < 6; ++coordinate)
        {
            const double value = amplitudes[6 * pair + coordinate];
            result[pair] += value * value;
        }
    }
}
}
#endif
