#undef check

#include "libusbhepers.h"
#include "jit.common.h"



// Struct definition
typedef struct _imp_push
{
	t_object object;
	t_uint8* frame_buffer;

} t_imp_push;

// Prototypes
BEGIN_USING_C_LINKAGE
t_jit_err imp_push_init();
t_imp_push* imp_push_new();
void imp_push_free(t_imp_push* x);
t_jit_err imp_push_matrix_calc(t_imp_push* x, void* inputs, void* outputs);
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

	// add matrix operator (mop)
	mop = (t_jit_object *)jit_object_new(_jit_sym_jit_mop, 1, 0);
	jit_mop_single_type(mop, _jit_sym_char);
	jit_mop_single_planecount(mop, 4);

	void* input = jit_object_method(mop, _jit_sym_getinput, 1);
	jit_attr_setlong(input, _jit_sym_mindimcount, 2);
	jit_attr_setlong(input, _jit_sym_maxdimcount, 2);
	
	jit_class_addadornment(s_imp_push_class, mop);

	// add methods
	jit_class_addmethod(s_imp_push_class, (method)imp_push_matrix_calc, "matrix_calc", A_CANT, 0);

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
		x->frame_buffer = sysmem_newptrclear(PUSH2_DISPLAY_IMAGE_BUFFER);
	}
	return x;
}

void imp_push_free(t_imp_push* x)
{
	sysmem_freeptr(x->frame_buffer);
}

t_jit_err imp_push_matrix_calc(t_imp_push* x, void* inputs, void* outputs)
{
	t_jit_err			err = JIT_ERR_NONE;
	long				in_savelock;
	long				out_savelock;
	t_jit_matrix_info	in_minfo;
	t_jit_matrix_info	out_minfo;
	char				*in_bp;
	char				*out_bp;
	long				i;
	long				dimcount;
	long				planecount;
	long				dim[JIT_MATRIX_MAX_DIMCOUNT];
	void				*in_matrix;
	void				*out_matrix;

	in_matrix = jit_object_method(inputs, _jit_sym_getindex, 0);
	out_matrix = jit_object_method(outputs, _jit_sym_getindex, 0);

	if (x && in_matrix && out_matrix)
	{
		in_savelock = (long)jit_object_method(in_matrix, _jit_sym_lock, 1);
		out_savelock = (long)jit_object_method(out_matrix, _jit_sym_lock, 1);

		jit_object_method(in_matrix, _jit_sym_getinfo, &in_minfo);
		jit_object_method(out_matrix, _jit_sym_getinfo, &out_minfo);

		jit_object_method(in_matrix, _jit_sym_getdata, &in_bp);
		jit_object_method(out_matrix, _jit_sym_getdata, &out_bp);

		if (!in_bp)
		{
			err = JIT_ERR_INVALID_INPUT;
			goto out;
		}
		if (!out_bp)
		{
			err = JIT_ERR_INVALID_OUTPUT;
			goto out;
		}
		if (in_minfo.type != out_minfo.type)
		{
			err = JIT_ERR_MISMATCH_TYPE;
			goto out;
		}

		//get dimensions/planecount
		dimcount = out_minfo.dimcount;
		planecount = out_minfo.planecount;

	}
	else
	{
		return JIT_ERR_INVALID_PTR;
	}

out:
	jit_object_method(out_matrix, _jit_sym_lock, out_savelock);
	jit_object_method(in_matrix, _jit_sym_lock, in_savelock);
	return err;
}