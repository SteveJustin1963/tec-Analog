\ Define constants
5 CONSTANT N
1000 CONSTANT SCALE

\ Data input
CREATE x1 1500 , 2000 , 2500 , 3000 , 3500 ,
CREATE x2 3 , 4 , 5 , 6 , 7 ,
CREATE y 200000 , 250000 , 300000 , 350000 , 400000 ,

\ Calculate means of x1, x2, and y
VARIABLE mean_x1
VARIABLE mean_x2
VARIABLE mean_y

: CALCULATE-MEANS ( array -- mean )
    0 N 0 DO
        I CELLS +  @ + 
    LOOP
    N / ;

x1 CALCULATE-MEANS mean_x1 !
x2 CALCULATE-MEANS mean_x2 !
y CALCULATE-MEANS mean_y !

\ Calculate coefficients b1 and b2
VARIABLE b1
VARIABLE b2
VARIABLE ss_xy
VARIABLE ss_xx

: CALCULATE-COEFFICIENTS ( array mean -- b )
    0 ss_xy !
    0 ss_xx !
    N 0 DO
        I CELLS + @ mean_x1 @ -  ( x - mean_x )
        I CELLS y + @ mean_y @ -  ( y - mean_y )
        * ss_xy +!                ( ss_xy = ss_xy + (x - mean_x)(y - mean_y) )
        
        I CELLS + @ mean_x1 @ - DUP * ( (x - mean_x)^2 )
        ss_xx +!                  ( ss_xx = ss_xx + (x - mean_x)^2 )
    LOOP
    ss_xy @ ss_xx @ / ;   \ Calculate the coefficient b = ss_xy/ss_xx

x1 mean_x1 @ CALCULATE-COEFFICIENTS b1 !
x2 mean_x2 @ CALCULATE-COEFFICIENTS b2 !

\ Calculate b3 (intercept)
mean_y @ b1 @ mean_x1 @ * SCALE / - b2 @ mean_x2 @ * SCALE / - mean_y @ + VARIABLE b3

\ Print the result
: PRINT-RESULT
    ." The line of best fit is: y = " b3 @ . 
    ." + " b1 @ . ."x1 + " b2 @ . ."x2" CR ;

PRINT-RESULT
