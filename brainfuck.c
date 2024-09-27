#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<stdint.h>

// #define DEBUG

enum ErrorFlags {
	NONE = 0,
	END_OF_STREAM = 1 << 0,
	END_OF_IO_BUF = 1 << 2,
};

struct Stack {
	uint8_t* stack[256];
	int index;
};

struct Stack stack_new()
{
	struct Stack o;
	o.index = 0;
	return o;
}

void stack_push(struct Stack* s, uint8_t* cursor)
{
	if (s->index == 256)
	{
		fprintf(stderr, "Stack overflow\n");
		abort();
	}

	s->stack[s->index] = cursor;

	s->index++;
}

uint8_t* stack_pop(struct Stack* s)
{
	if (!s->index)
	{
		fprintf(stderr, "Stack empty\n");
		abort();
	}

	uint8_t* out = s->stack[s->index];

	s->index--;

	return out;
}

uint8_t* stack_top(struct Stack* s)
{
	if (!s->index)
	{
		fprintf(stderr, "Stack empty\n");
		abort();
	}
	return s->stack[s->index-1];
}

short is_valid_instruction(uint8_t c)
{
	switch (c)
	{
		case '+':
		case '-':
		case '>':
		case '<':
		case '[':
		case ']':
		case '.':
		case ',':

			return 1;

		default: {}
	}

	return 0;
}

struct Machine {
	long size;
	uint8_t* const cells;
	uint8_t* const bound_start;
	uint8_t* const bound_end;

	uint8_t* ptr;
	uint8_t* instr;
	
	uint8_t* cursor;

	uint8_t* input;
	
	struct Stack stack;

	enum ErrorFlags flags;
};

struct Machine machine_new(long size, uint8_t* instr, char* input)
{
	uint8_t* cells = malloc(size);
	memset(cells, 0, size);

	struct Machine o = {
		.size = size,
		.cells = cells,

		.bound_start = cells-1,
		.bound_end = cells+size,
		
		.ptr = cells,
		
		.instr = instr,
		
		.cursor = instr,
		.flags = NONE,

		.input = input,
		
		.stack = stack_new()
	};

	return o;
}


short machine_verify_instr(struct Machine* m)
{
	if (!m->instr) return 0;

	long depth = 0;

	for (uint8_t* c = m->instr; *c; c++)
	{
		if (*c == '[') depth += 1;
		if (*c == ']') depth -= 1;
	}

	return depth == 0;
}

uint8_t* skip_loop(uint8_t* instr)
{
	long depth = 0;
	uint8_t* c = instr;
	while (*c)
	{
		if (*c == '[') depth++;
		if (*c == ']') depth--;

		if (*c == ']' && depth == 0) return c;

		c++;
	}

	fprintf(stderr, "No matching bracket found.\n");
	abort();

	return instr;
}

void inc(uint8_t* x)
{
	*x = (*x + 1) & 0xFF;
}

void dec(uint8_t* x)
{
	*x = (*x - 1) & 0xFF;
}

short machine_run_single(struct Machine* m)
{

#ifdef DEBUG
	printf("%02d ", m->ptr - m->cells);
	printf("Cells: ");
	for (int i = 0; i < 48; i++)
	{
		if ((m->cells+i) == m->ptr)
			printf("*%02x", m->cells[i]);
		else
			printf(" %02x", m->cells[i]);
	}

	printf("\n");

	for (uint8_t* c = m->instr; *c; c++)
	{
		if (is_valid_instruction(*c))
		{	
			if (c == m->cursor) printf("*%c", *c);
			else printf(" %c", *c);
		}
	}
	printf("\n");
#endif

	if (m->flags) return 0;
	
	switch (*m->cursor)
	{
		case '+': inc(m->ptr); break;
		case '-': dec(m->ptr); break;
	
		case '>': ++(m->ptr); break;
		case '<': --(m->ptr); break;
		
		case '.': printf("%c", *m->ptr); fflush(stdout); break;
		case ',': 

			*m->ptr = *m->input;

			m->input += (*m->input != 0);

			break;
		
		case '[':
			if (*m->ptr == 0) m->cursor = skip_loop(m->cursor); 
			else stack_push(&m->stack, m->cursor);
			break;

		case ']': 
			if (*m->ptr != 0) m->cursor = stack_top(&m->stack);
			else stack_pop(&m->stack); 
			break;

		case '\0': m->flags |= END_OF_STREAM; break;

		default: {}
	}

#ifdef DEBUG

	if (*m->cursor == '.') usleep(100*10000);

	if (is_valid_instruction(*m->cursor)) usleep(10*1000);
#endif

	++m->cursor;

	if (m->ptr == m->bound_start) m->ptr = m->bound_end-1;
	if (m->ptr == m->bound_end)   m->ptr = m->bound_start+1;

	return !m->flags;
}

void machine_run(struct Machine* m)
{
	if (!m->instr)
	{
		fprintf(stderr, "Instruction stream is NULL.\n");
		abort();
	}

	if (!machine_verify_instr(m))
	{
		fprintf(stderr, "Invalid instructions. Possibly incomplete pairs of backets?\n");
		abort();
	}

	while (machine_run_single(m)) ;
}

void machine_destroy(struct Machine* m)
{
	free(m->cells);
}

uint8_t const * const hello_program = "\
	>++++++++[<+++++++++>-]<.>++++[<+++++++>-]<+.+++++++..+++.>>++++++[<+++++++>-]<+\
	+.------------.>++++++[<+++++++++>-]<+.<.+++.------.--------.>>>++++[<++++++++>-\
	]<+.\
";


int main(int argc, uint8_t** argv)
{
	uint8_t* ins = 0;
	uint8_t* input = 0;

	if (argc == 1)
	{
		long size = strlen(hello_program)+1;
		ins = malloc(size);
		memcpy(ins, hello_program, size);
	}

	if (argc == 2)
	{
		FILE* file = fopen(argv[1], "r");
		if (!file)
		{
			fprintf(stderr, "Invalid file path given.\n");
			abort();
		}

		fseek(file, 0, SEEK_END);
		long size = ftell(file)+1;
		rewind(file);
		ins = malloc(size);
		ins[size-1] = '\0';

		if (!ins)
		{
			fprintf(stderr, "Failed to allocate the input stream.\n");
			abort();
		}

		fread(ins, sizeof(uint8_t), size, file);

		printf("Input: %s\n", ins);

		input = malloc(255);
		printf("Enter your input (max 255 uint8_ts): ");
		fgets(input, 255, stdin);
	}

	struct Machine m = machine_new(512, ins, input);

	machine_run(&m);

	machine_destroy(&m);

	free(ins);
	free(input);

	return 0;
}

