#include <stdio.h>
#include <stdlib.h>

/* IMPLEMENT ME: Declare your functions here */
int add (int a, int b);
int sub(int a, int b);
int mul(int a, int b);
int division(int a, int b);
int (*ptr[4]) (int x, int y);

int main (void)
{
	int a = 6;
	int b = 3;
	int input;
	int output;
	ptr[0] = add;
	ptr[1] = sub;
	ptr[2] = mul;
	ptr[3] = division;
	printf("Operand 'a' : 6 | Operand 'b' : 3\n");
	printf("Specify the operation to perform (0 : add 1 : subtract | 2 : Multiply | 3 : divide): ");
	scanf("%d", &input);
	output = (*ptr[input]) (a, b);
	printf("x = %d", output);
	printf("\n");
	return 0;
}

/* IMPLEMENT ME: Define your functions here */
int add (int a, int b) { printf ("Adding 'a' and 'b'\n"); return a + b; }
int sub(int a, int b) { printf ("Subtracting 'b' from 'a'\n"); return a - b; }
int mul(int a, int b) { printf ("Multiplying 'a' and 'b'\n"); return a * b; }
int division(int a, int b) { printf ("Dividing 'a' by 'b'\n"); return a / b; }
