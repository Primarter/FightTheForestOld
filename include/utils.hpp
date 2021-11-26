#pragma once

float randUniform(void)
{
    return (static_cast<double>(rand()) / (RAND_MAX));
}