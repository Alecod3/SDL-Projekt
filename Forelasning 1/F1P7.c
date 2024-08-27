#include <stdio.h>

int main(void)
{
    int a, b;

    printf("Enter a dollar amount: ");
    scanf("%d", &a);

    b = a / 20; 
    printf("$20 bills: %d ", b);
    a = a % 20;
    printf("\n");
    
    b = a / 10; 
    printf("$10 bills: %d ", b);
    a = a % 10;
    printf("\n");
    
    b = a / 5; 
    printf("$5 bills: %d ", b);
    a = a % 5;
    printf("\n");
    
    b = a / 1; 
    printf("$1 bills: %d ", b);
    a = a % 1;
    printf("\n");
    
    return 0;
}