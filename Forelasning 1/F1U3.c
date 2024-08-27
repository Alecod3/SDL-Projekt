#include <stdio.h>

int main(void)
{
    float r1, r2, rtot, rsum;
    
    printf("Resistans hos R1 (ohm) : ");
    scanf("%f", &r1);

    printf("Resistans hos R2 (ohm) : ");
    scanf("%f", &r2);

    rsum = r1 + r2;
    rtot = (r1 * r2) / rsum;
    
    printf("Parallellkoppling: %6.5f ohm\n", rtot);
    printf("Seriekoppling: %6.5f ohm\n", rsum);
    return 0;
}