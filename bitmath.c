//Timothy Alexander
//September 16, 2011
//BitMath Simplification

/*
	When performing low-level operations such as setting registers in the cpu, it is advantageous
	to take a few shortcuts, as bitmath can be confusing to understand when trying to decifer the function of a program. Using shift operations with simple logic allows for much easier bit manipulation than setting the register manually each and every time. To simply this process even simpler I have a few preprocessor commands to make the process semantically simple.
	
	Setting a bit: number |=  (1 << x);
	Clearing a bit: number &= ~(1 << x);
	Toggling a bit: number ^=  (1 << x);
	Checking a bit: bit = number & (1 << x);
*/

/* a=target variable, b=bit number to act upon 0<->n */
#define BIT_SET(a,b)   ((a) |=  (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b)  ((a) ^=  (1<<(b)))
#define BIT_CHECK(a,b) ((a) &   (1<<(b)))

/* x = target variable, y = mask */
#define BITMASK_SET(x,y)   ((x) |=  (y))
#define BITMASK_CLEAR(x,y) ((x) &= ~(y))
#define BITMASK_FLIP(x,y)  ((x) ^=  (y))
#define BITMASK_CHECK(x,y) ((x) &   (y))