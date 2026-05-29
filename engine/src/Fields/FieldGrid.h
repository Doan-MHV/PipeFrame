#ifndef PIPEFRAME_SIMULATIONFIELDGRID_H
#define PIPEFRAME_SIMULATIONFIELDGRID_H

#include <vector>

struct FieldGrid
{
    int rows = 0;
    int cols = 0;
    float cellWorldSize = 1.0f;
    std::vector<double> values;
};

#endif // PIPEFRAME_SIMULATIONFIELDGRID_H
