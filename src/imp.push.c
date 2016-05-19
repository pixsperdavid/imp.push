#undef check

#include "libusbhepers.h"
#include "jit.common.h"



// Struct definition
typedef struct _imp_push
{
	t_object object;

	t_systhread thread;
	t_systhread_mutex mutex;
	t_bool isThreadCancel;

	
	t_uint8* frame_buffer;
	t_uint8* frame_bufferA;
	t_uint8* frame_bufferB;

	libusb_device_handle* device;

} t_imp_push;

// Prototypes
BEGIN_USING_C_LINKAGE
t_jit_err imp_push_init();
t_imp_push* imp_push_new();
void imp_push_free(t_imp_push* x);
t_jit_err imp_push_matrix_calc(t_imp_push* x, void* inputs, void* outputs);
void imp_push_mask_buffer(t_imp_push* x);
void* imp_push_threadproc(t_imp_push *x);
void imp_push_drawframe(t_imp_push* x);
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
	
	t_atom args[2];
	jit_atom_setlong(args, PUSH2_DISPLAY_WIDTH);
	jit_atom_setlong(args + 1, PUSH2_DISPLAY_HEIGHT);

	void* input = jit_object_method(mop, _jit_sym_getinput, 1);
	jit_object_method(input, _jit_sym_mindim, 2, &args);
	jit_object_method(input, _jit_sym_maxdim, 2, &args);
	jit_object_method(input, _jit_sym_ioproc, jit_mop_ioproc_copy_adapt);
	
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
		x->frame_bufferA = sysmem_newptrclear(PUSH2_DISPLAY_IMAGE_BUFFER_SIZE);
		x->frame_bufferB = sysmem_newptrclear(PUSH2_DISPLAY_IMAGE_BUFFER_SIZE);
		x->frame_buffer = x->frame_bufferA;

		systhread_mutex_new(&x->mutex, 0);

		x->device = push2_open_device();

		systhread_create((method)imp_push_threadproc, x, 0, 0, 0, &x->thread);

	}
	return x;
}

void imp_push_free(t_imp_push* x)
{
	x->isThreadCancel = true;
	uint* value;
	systhread_join(x->thread, &value);

	systhread_mutex_free(x->mutex);

	if (x->device != NULL)
		push2_close_device(x->device);

	sysmem_freeptr(x->frame_buffer);
	
}

t_jit_err imp_push_matrix_calc(t_imp_push* x, void* inputs, void* outputs)
{
	t_jit_err			err = JIT_ERR_NONE;
	long				in_savelock;
	t_jit_matrix_info	in_minfo;
	char				*in_bp;
	long				i;
	long				dimcount;
	long				planecount;
	long				dim[JIT_MATRIX_MAX_DIMCOUNT];
	void				*in_matrix;

	in_matrix = jit_object_method(inputs, _jit_sym_getindex, 0);

	if (x && in_matrix)
	{
		in_savelock = (long)jit_object_method(in_matrix, _jit_sym_lock, 1);

		jit_object_method(in_matrix, _jit_sym_getinfo, &in_minfo);

		jit_object_method(in_matrix, _jit_sym_getdata, &in_bp);

		if (!in_bp)
		{
			err = JIT_ERR_INVALID_INPUT;
			goto out;
		}

		//get dimensions/planecount
		dimcount = in_minfo.dimcount;
		planecount = in_minfo.planecount;

		char* src = in_bp;

		systhread_mutex_lock(x->mutex);
		uint16_t* dst = (uint16_t*)x->frame_buffer;
		x->frame_buffer = x->frame_buffer == x->frame_bufferA ? x->frame_bufferB : x->frame_bufferA;
		systhread_mutex_unlock(x->mutex);

		for(int l = 0; l < PUSH2_DISPLAY_HEIGHT; ++l)
		{
			for (int r = 0; r < PUSH2_DISPLAY_WIDTH; ++r)
			{
				*dst++ = (*(src + 1) >> 3) & (*(src + 2) >> 2) & (*(src + 3) >> 3);
				src += 4;
			}

			dst += PUSH2_DISPLAY_LINE_GUTTER_SIZE / 2;
		}

		imp_push_mask_buffer(x);

	}
	else
	{
		return JIT_ERR_INVALID_PTR;
	}

out:
	jit_object_method(in_matrix, _jit_sym_lock, in_savelock);
	return err;
}

void imp_push_mask_buffer(t_imp_push* x)
{
	for(int l = 0; l < PUSH2_DISPLAY_HEIGHT; ++l)
	{
		int offset = l * PUSH2_DISPLAY_LINE_SIZE;

		for(int r = 0; r < PUSH2_DISPLAY_LINE_SIZE - PUSH2_DISPLAY_LINE_GUTTER_SIZE; r += 4)
		{
			x->frame_buffer[offset + r] = x->frame_buffer[offset + r] ^ 0xE7;
			x->frame_buffer[offset + r + 1] = x->frame_buffer[offset + r + 1] ^ 0xF3;
			x->frame_buffer[offset + r + 2] = x->frame_buffer[offset + r + 2] ^ 0xE7;
			x->frame_buffer[offset + r + 3] = x->frame_buffer[offset + r + 3] ^ 0xFF;
		}
	}
}

void* imp_push_threadproc(t_imp_push *x)
{
	uint8_t* buffer_to_send;

	while(!x->isThreadCancel)
	{
		systhread_mutex_lock(x->mutex);
		buffer_to_send = x->frame_buffer;
		systhread_mutex_unlock(x->mutex);

		if (x->device != NULL)
			push2_send_frame(x->device, buffer_to_send);

		systhread_sleep(1000 / PUSH2_DISPLAY_FRAMERATE);
	}
}