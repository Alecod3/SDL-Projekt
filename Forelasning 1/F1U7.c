#include <stdio.h>
#include <string.h>

int main()
{
    char a [11];
    char b [9] = "";
    
    printf("Enter a date (mm/dd/yyyy): ");
    scanf("%10s", a);
    
    int j = 0;
    for(int i = 0; i < 10 && j <= 7; i++)
    {
        if(a[i] != '/')
        {
            b[j] = a[i];
            j++;
        }
    }

    char c [9] = "";
    j = 0;
    
    for(int k = 0; k <= 1; k++)
    {
        c[6 + k] = b[k];
    }

    for(int k = 2; k <= 3; k++)
    {
        c[4 + j] = b[k];
        j++;
    }

    for(int k = 4; k <= 7; k++)
    {
        c[k - 4] = b[k];
    }

    printf("You entered the date: %s", c);
    return 0;
}