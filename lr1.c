#include <stdio.h>

#define N 5

int main()
{
    // data input
    double x1[N] = {1500, 2000, 2500, 3000, 3500};
    double x2[N] = {3, 4, 5, 6, 7};
    double y[N] = {200000, 250000, 300000, 350000, 400000};

    // Calculate means of x1, x2, and y
    double sum_x1 = 0, sum_x2 = 0, sum_y = 0;
    for(int i = 0; i < N; i++){
        sum_x1 += x1[i];
        sum_x2 += x2[i];
        sum_y += y[i];
    }
    double mean_x1 = sum_x1 / N;
    double mean_x2 = sum_x2 / N;
    double mean_y = sum_y / N;

    // Calculate coefficients b1 and b2
    double ss_xy1 = 0, ss_xx1 = 0, ss_xy2 = 0, ss_xx2 = 0;
    for(int i = 0; i < N; i++){
        ss_xy1 += (x1[i] - mean_x1) * (y[i] - mean_y);
        ss_xx1 += (x1[i] - mean_x1) * (x1[i] - mean_x1);
        ss_xy2 += (x2[i] - mean_x2) * (y[i] - mean_y);
        ss_xx2 += (x2[i] - mean_x2) * (x2[i] - mean_x2);
    }
    double b1 = ss_xy1 / ss_xx1;
    double b2 = ss_xy2 / ss_xx2;

    // Calculate b0 (intercept)
double b0 = mean_y - b1 * mean_x1 - b2 * mean_x2;

// Print the result
printf("The line of best fit is: y = %lf + %lfx1 + %lfx2\n", b0, b1, b2);


    return 0;
}
