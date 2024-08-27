#include <stdio.h>

int main(void)
{
    int distance, speed, time;
    
    printf("Hur snabbt? ");
    scanf("%d", &speed);

    printf("Hur lang tid? ");
    scanf("%d", &time);

    distance = speed * time;

    printf("Om du fardas i %d h med hastigheten %d km/h kommer du %d km\n",time, speed, distance);
    printf("Lycka till pa farden \n");
    return 0;
}