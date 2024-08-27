#include <stdio.h>
#include <math.h>
#define M_PI           3.14159265358979323846 // Behövs inte? 

int main(void)
{
    float v,r;

    printf("Radie? ");
    scanf("%f", &r);

    v = (4.0 / 3.0) *  M_PI * r * r * r;
    
    printf("Volymen av sfaren med radien %.2f m blir %.2f kubikmeter", r, v);
    return 0;
}