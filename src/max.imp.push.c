#include "jit.common.h"
#include "max.jit.mop.h"


// Struct definition
typedef struct _max_imp_push
{
	t_object object;
	void* obex;

} t_max_imp_push;


// Prototypes
BEGIN_USING_C_LINKAGE
t_jit_err imp_push_init(void);
void* max_imp_push_new(t_symbol* s, long argc, t_atom* argv);
void max_imp_push_free(t_max_imp_push* x);
END_USING_C_LINKAGE


// Globals
static void* s_max_imp_push_class = NULL;


// Class registration
void ext_main(void* r)
{
	t_class *max_class, *jit_class;

	imp_push_init();

	max_class = class_new("imp.push", (method)max_imp_push_new, (method)max_imp_push_free, sizeof(t_max_imp_push), NULL, A_GIMME, 0);
	max_jit_class_obex_setup(max_class, calcoffset(t_max_imp_push, obex));

	jit_class = (t_class*)jit_class_findbyname(gensym("imp_push"));
	max_jit_class_wrap_standard(max_class, jit_class, 0); // attrs & methods for getattributes, dumpout, maxjitclassaddmethods, etc

	class_addmethod(max_class, (method)max_jit_mop_assist, "assist", A_CANT, 0); // standard matrix-operator (mop) assist fn

	class_register(CLASS_BOX, max_class);
	s_max_imp_push_class = max_class;
}


// **************************************************************************************************************************


void* max_imp_push_new(t_symbol* s, long argc, t_atom* argv)
{
	t_max_imp_push* x;
	void* o;

	x = (t_max_imp_push *)max_jit_object_alloc((t_class*)s_max_imp_push_class, gensym("imp_push"));
	if (x)
	{
		o = jit_object_new(gensym("imp_push"));
		if (o)
		{
			max_jit_mop_setup_simple(x, o, argc, argv);
			max_jit_attr_args(x, argc, argv);
		}
		else
		{
			jit_object_error((t_object *)x, "imp.push: could not allocate object");
			object_free((t_object *)x);
			x = NULL;
		}
	}
	return x;
}



void max_imp_push_free(t_max_imp_push* x)
{
	jit_object_free(max_jit_obex_jitob_get(x));
	max_jit_object_free(x);
}
