# define REG_R8		0
# define REG_R9		1
# define REG_R10	2
# define REG_R11	3
# define REG_R12	4
# define REG_R13	5
# define REG_R14	6
# define REG_R15	7
# define REG_RDI	8
# define REG_RSI	9
# define REG_RBP	10
# define REG_RBX	11
# define REG_RDX	12
# define REG_RAX	13
# define REG_RCX	14
# define REG_RSP	15
# define REG_RIP	16

#define OFF 20 // magic! offsetof(ucontext_t, uc_mcontext)

#define C_SYMBOL_NAME(name) name
#define C_LABEL(name) name

#define ENTRY(name)				\
	.text;					\
	.align 32;				\
	.global C_SYMBOL_NAME(name);		\
	C_LABEL(name):

	