#undef check

#include "libusbhepers.h"
#include "jit.common.h"



// Struct definition
typedef struct _imp_push
{
	t_object object;

} t_imp_push;

// Prototypes
BEGIN_USING_C_LINKAGE
t_jit_err imp_push_init();
t_imp_push* imp_push_new();
void imp_push_free(t_imp_push* x);
END_USING_C_LINKAGE

// Globals
static t_class* s_imp_push_class = NULL;

// Class registration
t_jit_err imp_push_init()
{
	long attrflags = JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_USURP_LOW;
	t_jit_object* attr;
	t_jit_object* mop;

	s_imp_push_class = (t_class*)jit_class_new("imp_push", (method)imp_push_new, (method)imp_push_free, sizeof(t_imp_push), 0);

	// add methods

	// finalize class
	jit_class_register(s_imp_push_class);
	return JIT_ERR_NONE;
}


// **************************************************************************************************************************


t_imp_push* imp_push_new()
{
	t_imp_push* x = NULL;

	x = (t_imp_push*)jit_object_alloc(s_imp_push_class);
	if (x)
	{
		
	}
	return x;
}

void imp_push_free(t_imp_push* x)
{
	
}