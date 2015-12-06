/*
  Copyright (C) 2015 by Syohei YOSHIDA

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <emacs-module.h>

#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#undef INCLUDE_ENCODING
#include <mruby/string.h>
#include <mruby/khash.h>
#include <mruby/hash.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/compile.h>
#include <mruby/value.h>

int plugin_is_GPL_compatible;

static bool
eq_type(emacs_env *env, emacs_value type, const char *type_str)
{
	return env->eq(env, type, env->intern(env, type_str));
}

static mrb_value
elisp_to_mruby(emacs_env *env, mrb_state *mrb, emacs_value v)
{
	emacs_value type = env->type_of(env, v);

	if (eq_type(env, type, "integer")) {
		return mrb_fixnum_value((mrb_int)(env->extract_integer(env, v)));
	} else if (eq_type(env, type, "float")) {
		return mrb_float_value(mrb, env->extract_float(env, v));
	} else if (eq_type(env, type, "string")) {
		ptrdiff_t size = 0;
		env->copy_string_contents(env, v, NULL, &size);

		char *str = malloc(size);
		env->copy_string_contents(env, v, str, &size);
		mrb_value ret = mrb_str_new(mrb, str, size-1);
		free(str);
		return ret;
	} else if (env->eq(env, v, env->intern(env, "t"))) {
		return mrb_true_value();
	} else if (env->eq(env, v, env->intern(env, "nil"))) {
		return mrb_false_value();
	} else if (eq_type(env, type, "vector")) {
		mrb_value array = mrb_ary_new(mrb);

		for (int i = 0; i < (int)(env->vec_size(env, v)); ++i) {
			mrb_ary_push(mrb, array, elisp_to_mruby(env, mrb, env->vec_get(env, v, i)));
		}

		return array;
	} else if (eq_type(env, type, "hash-table")) {
		mrb_value hash = mrb_hash_new(mrb);
		emacs_value Fhash_table_keys = env->intern(env, "hash-table-keys");
		emacs_value Fgethash = env->intern(env, "gethash");
		emacs_value keys_args[] = {v};
		emacs_value keys = env->funcall(env, Fhash_table_keys, 1, keys_args);

		for (int i = 0; i < (int)(env->vec_size(env, keys)); ++i) {
			emacs_value key = env->vec_get(env, keys, i);
			emacs_value args[] = {key, v};
			emacs_value v = env->funcall(env, Fgethash, 2, args);

			mrb_hash_set(mrb, hash,
				     elisp_to_mruby(env, mrb, key),
				     elisp_to_mruby(env, mrb, v));
		}

		return hash;
	}

	return mrb_nil_value();
}

static emacs_value
mruby_to_elisp(emacs_env *env, mrb_state *mrb, mrb_value v)
{
	switch (mrb_type(v)) {
	case MRB_TT_TRUE:
		return env->intern(env, "t");
	case MRB_TT_FALSE:
		return env->intern(env, "nil");
	case MRB_TT_FLOAT:
		return env->make_float(env, mrb_float(v));
	case MRB_TT_FIXNUM:
		return env->make_integer(env, mrb_fixnum(v));
	case MRB_TT_ARRAY: {
		int len = (int)RARRAY_LEN(v);
		if (len == 0)
			return env->intern(env, "nil");

		emacs_value *array = malloc(sizeof(emacs_value) * len);
		if (array == NULL)
			return env->intern(env, "nil");

		for (int i = 0; i < len; ++i) {
			array[i] = mruby_to_elisp(env, mrb, mrb_ary_ref(mrb, v, (mrb_int)i));
		}

		emacs_value Fvector = env->intern(env, "vector");
		emacs_value vec = env->funcall(env, Fvector, len, array);
		free(array);
		return vec;
	}
	case MRB_TT_HASH: {
		emacs_value Fmake_hash_table = env->intern(env, "make-hash-table");
		emacs_value Qtest = env->intern(env, ":test");
		emacs_value Fequal = env->intern(env, "equal");
		emacs_value make_hash_args[] = {Qtest, Fequal};
		emacs_value hash = env->funcall(env, Fmake_hash_table, 2, make_hash_args);

		emacs_value Fputhash = env->intern(env, "puthash");

		mrb_value keys = mrb_hash_keys(mrb, v);
		for (int i = 0; i < (int)RARRAY_LEN(keys); ++i) {
			mrb_value key = mrb_ary_ref(mrb, keys, (mrb_int)i);
			mrb_value val = mrb_hash_get(mrb, v, key);
			const char *key_str = mrb_string_value_ptr(mrb, key);
			int key_len = (int)mrb_string_value_len(mrb, key);

			emacs_value key_lisp = env->make_string(env, key_str, key_len);
			emacs_value puthash_args[] = {
				key_lisp, mruby_to_elisp(env, mrb, val), hash};
			env->funcall(env, Fputhash, 3, puthash_args);
		}
		return hash;
	}
        case MRB_TT_STRING: {
		const char *str = mrb_string_value_ptr(mrb, v);
		return env->make_string(env, str, mrb_string_value_len(mrb, v));
	}
	default:
		return env->intern(env, "nil");
	}
}

static void
mruby_free(void *arg)
{
	mrb_close((mrb_state*)arg);
}

static emacs_value
Fmruby_init(emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
	mrb_state *mrb = mrb_open();
	return env->make_user_ptr(env, mruby_free, mrb);
}

static emacs_value
Fmruby_eval(emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
	mrb_state *mrb = env->get_user_ptr(env, args[0]);
	if (mrb == NULL)
		return env->intern(env, "nil");

	emacs_value code = args[1];
	ptrdiff_t size = 0;
	char *code_buf = NULL;

	env->copy_string_contents(env, code, NULL, &size);
	code_buf = malloc(size);
	if (code_buf == NULL)
		return env->intern (env, "nil");
	env->copy_string_contents(env, code, code_buf, &size);

	mrbc_context *ctx = mrbc_context_new(mrb);
	struct mrb_parser_state *ps = mrb_parse_string(mrb, code_buf, ctx);
	struct RProc *proc = mrb_generate_code(mrb, ps);
	mrb_pool_close(ps->pool);

	mrb_value argv = mrb_ary_new(mrb);
	for (int i = 2; i < (int)nargs; ++i) {
		mrb_ary_push(mrb, argv, elisp_to_mruby(env, mrb, args[i]));
	}
	mrb_define_global_const(mrb, "ARGV", argv);

	mrb_value val = mrb_run(mrb, proc, mrb_top_self(mrb));
	mrbc_context_free(mrb, ctx);

	free(code_buf);

	return mruby_to_elisp(env, mrb, val);
}

static emacs_value
Fmruby_send(emacs_env *env, ptrdiff_t nargs, emacs_value args[], void *data)
{
	mrb_state *mrb = env->get_user_ptr(env, args[0]);
	if (mrb == NULL)
		return env->intern(env, "nil");

	mrb_value obj = elisp_to_mruby(env, mrb, args[1]);

	emacs_value Fsymbol_name = env->intern(env, "symbol-name");
	emacs_value symbol_name_args[] = {args[2]};
	emacs_value func_name = env->funcall(env, Fsymbol_name, 1, symbol_name_args);

	ptrdiff_t size = 0;
	env->copy_string_contents(env, func_name, NULL, &size);
	char *func_name_buf = (char*)malloc((size_t)size);
	if (func_name_buf == NULL)
		return env->intern(env, "nil");
	env->copy_string_contents(env, func_name, func_name_buf, &size);

	int mruby_nargs = (int)nargs - 3;
	mrb_value *mruby_argv = malloc(sizeof(mrb_value) * mruby_nargs);
	for (int i = 0; i < mruby_nargs; ++i) {
		mruby_argv[i] = elisp_to_mruby(env, mrb, args[i+3]);
	}

	mrb_value val = mrb_funcall_argv(mrb, obj, mrb_intern_cstr(mrb, func_name_buf),
					 mruby_nargs, mruby_argv);

	free(func_name_buf);
	free(mruby_argv);

	return mruby_to_elisp(env, mrb, val);
}

static void
bind_function(emacs_env *env, const char *name, emacs_value Sfun)
{
	emacs_value Qfset = env->intern(env, "fset");
	emacs_value Qsym = env->intern(env, name);
	emacs_value args[] = { Qsym, Sfun };

	env->funcall(env, Qfset, 2, args);
}

static void
provide(emacs_env *env, const char *feature)
{
	emacs_value Qfeat = env->intern(env, feature);
	emacs_value Qprovide = env->intern (env, "provide");
	emacs_value args[] = { Qfeat };

	env->funcall(env, Qprovide, 1, args);
}

int
emacs_module_init(struct emacs_runtime *ert)
{
	emacs_env *env = ert->get_environment(ert);

#define DEFUN(lsym, csym, amin, amax, doc, data) \
	bind_function (env, lsym, env->make_function(env, amin, amax, csym, doc, data))

	DEFUN("mruby-init", Fmruby_init, 0, 0, "Initialize mruby state", NULL);
	DEFUN("mruby-eval", Fmruby_eval, 2, emacs_variadic_function, "Evaluate mruby code", NULL);
	DEFUN("mruby-send", Fmruby_send, 3, emacs_variadic_function, "Send object", NULL);

#undef DEFUN

	provide(env, "mruby-core");
	return 0;
}
