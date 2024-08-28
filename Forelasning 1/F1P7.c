#include <stdio.h>

int main(void)
{
    int amountLeft, amountOfBills;

    printf("Enter a dollar amount: ");
    scanf("%d", &amountLeft);

    amountOfBills = amountLeft / 20; 
    printf("$20 bills: %d ", amountOfBills);
    amountLeft = amountLeft % 20;
    printf("\n");
    
    amountOfBills = amountLeft / 10; 
    printf("$10 bills: %d ", amountOfBills);
    amountLeft = amountLeft % 10;
    printf("\n");
    
    amountOfBills = amountLeft / 5; 
    printf("$5 bills: %d ", amountOfBills);
    amountLeft = amountLeft % 5;
    printf("\n");
    
    amountOfBills = amountLeft / 1; 
    printf("$1 bills: %d ", amountOfBills);
    amountLeft = amountLeft % 1;
    printf("\n");
    
    return 0;
}