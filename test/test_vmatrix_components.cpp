#include "vmatrix_components.h"
#include <cassert>
#include <cstdio>
#include <iostream>
#include <functional>

void rejected(const std::function<void()>& action)
{
    bool failed = false;
    try { action(); } catch (const std::runtime_error&) { failed = true; }
    assert(failed);
}
void record(std::ofstream& out, const void* data, int size)
{
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    out.write(static_cast<const char*>(data), size);
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
}
int main()
{
    using namespace VMatrixComponents;
    assert(selection_mask("all") == -1);
    assert(selection_mask("mgga,momentum") == 5);
    for (const std::string name : {"", "total", "bad", "momentum,", "momentum,momentum", "momentum,,mgga"})
        rejected([&]() { selection_mask(name); });
    const std::string file = "/tmp/test-vmatrix-components.dat";
    const int header[3] = {1, 3, 3};
    double component[3][18];
    {
        std::ofstream out(file, std::ios::binary);
        record(out, header, sizeof(header));
        for (int c = 0; c < 3; ++c)
        {
            for (int i = 0; i < 18; ++i) component[c][i] = (c == 1 ? -1.0 : 1.0) * (c + 1) * (i + 1);
            record(out, component[c], sizeof(component[c]));
        }
    }
    for (int mask = 1; mask < 8; ++mask)
    {
        double result[3];
        read(file, 3, mask, result);
        for (int pair = 0; pair < 3; ++pair)
        {
            double expected = 0;
            for (int axis = 0; axis < 6; ++axis)
            {
                double sum = 0;
                for (int c = 0; c < 3; ++c) if (mask & (1 << c)) sum += component[c][6 * pair + axis];
                expected += sum * sum;
            }
            assert(result[pair] == expected);
        }
    }
    double all_three[3];
    double explicit_three[3];
    read(file, 3, selection_mask("all"), all_three);
    read(file, 3, 7, explicit_three);
    for (int i = 0; i < 3; ++i) assert(all_three[i] == explicit_three[i]);
    rejected([&]() { read(file, 3, 8, all_three); }); // Older files have no EXX record.
    assert(selection_mask("exx") == 8);
    assert(selection_mask("momentum,nonlocal,exx") == 11);
    {
        std::ofstream out(file, std::ios::binary);
        const int four_header[3] = {1, 3, 4};
        record(out, four_header, sizeof(four_header));
        for (int c = 0; c < 3; ++c) record(out, component[c], sizeof(component[c]));
        record(out, component[2], sizeof(component[2]));
    }
    for (int mask = 1; mask < 16; ++mask)
    {
        double result[3];
        read(file, 3, mask, result);
        for (int pair = 0; pair < 3; ++pair)
        {
            double expected = 0;
            for (int axis = 0; axis < 6; ++axis)
            {
                double sum = 0;
                for (int c = 0; c < 4; ++c)
                {
                    if (mask & (1 << c)) sum += component[c == 3 ? 2 : c][6 * pair + axis];
                }
                expected += sum * sum;
            }
            assert(result[pair] == expected);
        }
    }
    double all_four[3];
    double explicit_four[3];
    read(file, 3, selection_mask("all"), all_four);
    read(file, 3, 15, explicit_four);
    for (int i = 0; i < 3; ++i) assert(all_four[i] == explicit_four[i]);
    double result[6];
    rejected([&]() { read(file, 4, 7, result); });
    rejected([&]() { read(file + ".missing", 3, 7, result); });
    {
        std::ofstream out(file, std::ios::binary | std::ios::app);
        out.put('x');
    }
    rejected([&]() { read(file, 3, 7, result); });
    {
        std::ofstream out(file, std::ios::binary);
        record(out, header, sizeof(header));
        record(out, component[0], sizeof(component[0]));
    }
    rejected([&]() { read(file, 3, 7, result); });
    std::remove(file.c_str());
    std::cout << "Three/four-component subsets, EXX, interference, backward compatibility and invalid files passed\n";
}
