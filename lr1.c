#include <stdio.h>
#include <math.h>

#define N 5

int main()
{
    // data input
    double x1[N] = {1500, 2000, 2500, 3000, 3500};
    double x2[N] = {3, 4, 5, 6, 7};
    double y[N] = {200000, 250000, 300000, 350000, 400000};

    // Step 1: Initialize variables for storing the sum of x1, x2, y, x1y, x2y, x1^2, x2^2, x1x2
    double sum_x1 = 0, sum_x2 = 0, sum_y = 0;
    double sum_x1y = 0, sum_x2y = 0;
    double sum_x1_squared = 0, sum_x2_squared = 0, sum_x1x2 = 0;

    // Step 2: Iterate through the data and calculate the sum of x1, x2, y, x1y, x2y, x1^2, x2^2, x1x2
    for (int i = 0; i < N; i++)
    {
        sum_x1 += x1[i];
        sum_x2 += x2[i];
        sum_y += y[i];
        sum_x1y += x1[i] * y[i];
        sum_x2y += x2[i] * y[i];
        sum_x1_squared += x1[i] * x1[i];
        sum_x2_squared += x2[i] * x2[i];
        sum_x1x2 += x1[i] * x2[i];
    }

    // Step 3: Calculate the means of x1, x2 and y
    double mean_x1 = sum_x1 / N;
    double mean_x2 = sum_x2 / N;
    double mean_y = sum_y / N;

    // Step 4: Calculate the coefficients b1, b2 and b3
    double b1 = (N * sum_x1x2 - sum_x1 * sum_x2) / (N * sum_x1_squared - sum_x1 * sum_x1);
    double b2 = (N * sum_x2y - sum_x2 * sum_y) / (N * sum_x2_squared - sum_x2 * sum_x2);
    double b3 = mean_y - b1 * mean_x1 - b2 * mean_x2;

    // Step 5: Print the result
    printf("The line of best fit is: y = %lf + %lfx1 + %lfx2\n", b3, b1, b2);

    return 0;
}
