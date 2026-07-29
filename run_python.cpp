#include <stdio.h>
#include <unistd.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include "Python.h"
#include "numpy/arrayobject.h"
// COW COW #include "/home/laser/.local/lib/python3.10/site-packages/numpy/_core/include/numpy/arrayobject.h"

#include "opencv2/opencv.hpp"

void	python_init()
{
	Py_Initialize();
}

PyObject *python_create_module_from_string(char *module_name, char *code_string)
{
	PyObject *module = NULL;
	PyObject *compiled_code = Py_CompileString(code_string, "<string>", Py_file_input);
	if(compiled_code != nullptr)
	{
		module = PyImport_ExecCodeModule(module_name, compiled_code);
		Py_DECREF(compiled_code);
	}
	return(module);
}

int		python_kludge()
{
	import_array1(-1);
	return(0);
}

PyObject	*python_create_code_module(char *name, char *code)
{
	PyObject *module = NULL;
	PyObject *sys_path = PySys_GetObject("path");
	PyList_Append(sys_path, PyUnicode_FromString("."));
	python_kludge();
	if(code == NULL)
	{
		module = PyImport_ImportModule(name);
	}
	else
	{
		module = python_create_module_from_string(name, code);
	}
	return(module);
}

PyObject	*python_discover_function(PyObject *module, char *name)
{
	// get dictionary of available items in the module
	PyObject *dictionary = PyModule_GetDict(module);

	// grab the functions we are interested in
	PyObject *function = PyDict_GetItemString(dictionary, name);

	return(function);
}

PyObject	*python_call_function(PyObject *function, PyObject *args)
{
	PyObject *r = PyEval_CallObject(function, args);
	return(r);
}

PyObject	*python_prepare_mat(cv::Mat img)
{
	int width = img.cols;
	int height = img.rows;
	int depth = img.channels();

	// total number of elements (here it's an RGB image of size 640x480)
	const unsigned int nElem = width * height * depth;

	// create an array of apropriate datatype
	uchar *m = img.data;

	// the dimensions of the matrix
	npy_intp mdim[] = { height, width, depth };

	// convert the cv::Mat to numpy.array
	PyObject *mat = PyArray_SimpleNewFromData(3, mdim, NPY_UINT8, (void *)m);

	// create a Python-tuple of arguments for the function call
	// "()" means "tuple". "O" means "object"
	PyObject *args = Py_BuildValue("(O)", mat);

	return(args);
}

void	python_xderefrence(void *ptr)
{
	if(ptr != NULL)
	{
		Py_XDECREF(ptr);
	}
}

void	python_derefrence(void *ptr)
{
	if(ptr != NULL)
	{
		Py_DECREF(ptr);
	}
}

void	*python_prepare_code(char *module_name, char *function_name, char *code)
{
static int init = 0;

	if(init == 0)
	{
		python_init();
		init = 1;
	}
	PyObject *function = NULL;
	PyObject *module = python_create_code_module(module_name, code);
	if(module != NULL)
	{
		function = python_discover_function(module, function_name);
	}
	return((void *)function);
}

void	python_run_frame_filter(void *in_function, cv::Mat mat)
{
	void *rr = NULL;
	PyObject *function = (PyObject *)in_function;
	PyObject *frame = python_prepare_mat(mat);
	if(frame != NULL)
	{
		PyObject *r = python_call_function(function, frame);
		if(r != NULL)
		{
			Py_DECREF(r);
		}
		Py_DECREF(frame);
	}
}

void	*python_run(void *in_function)
{
	void *rr = NULL;
	PyObject *function = (PyObject *)in_function;
	PyObject *r = python_call_function(function, NULL);
	rr = (void *)r;
	return(rr);
}

void	python_filter_mat(cv::Mat mat, char *code)
{
static int init = 0;

	if(init == 0)
	{
		python_init();
		init = 1;
	}
	PyObject *module = python_create_code_module("python_frame_filter", code);
	if(module != NULL)
	{
		PyObject *function = python_discover_function(module, "python_filter");
		if(function != NULL)
		{
			PyObject *frame = python_prepare_mat(mat);
			if(frame != NULL)
			{
				PyObject *r = python_call_function(function, frame);
				if(r != NULL)
				{
					Py_DECREF(r);
				}
				Py_DECREF(frame);
			}
			Py_XDECREF(function);
		}
		Py_DECREF(module);
	}
}
