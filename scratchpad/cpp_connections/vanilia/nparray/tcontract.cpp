#include "Python.h"
#include <iostream>
#include "numpy/arrayobject.h"
#include <math.h>

static PyObject * integrate3(PyObject * module, PyObject * args)
{
    PyObject * argy=NULL;        // Regular Python/C API
    PyArrayObject * yarr=NULL;   // Extended Numpy/C API
    double dx,dy,dz;

    std::cout << "in func" <<std::endl;

    // "O" format -> read argument as a PyObject type into argy (Python/C API)
    if (!PyArg_ParseTuple(args, "Oddd", &argy,&dx,&dy,&dz))
    {
        PyErr_SetString(PyExc_ValueError, "Error parsing arguments.");
        return NULL;
    }

    std::cout << "parsed" << std::endl;
    // Determine if it's a complex number array (Numpy/C API)
    int DTYPE = PyArray_ObjectType(argy, NPY_FLOAT); 
    int iscomplex = PyTypeNum_ISCOMPLEX(DTYPE);      
    std::cout << "Is complex" << iscomplex << std::endl;

    // parse python object into numpy array (Numpy/C API)
    yarr = (PyArrayObject *)PyArray_FROM_OTF(argy, DTYPE, NPY_ARRAY_IN_ARRAY);
    if (yarr==NULL) {
        Py_INCREF(Py_None);
        return Py_None;
    }

    //just assume this for 3 dimensional array...you can generalize to N dims
    if (PyArray_NDIM(yarr) != 3) {
        Py_CLEAR(yarr);
        PyErr_SetString(PyExc_ValueError, "Expected 3 dimensional integrand");
        return NULL;
    }

    npy_intp * dims = PyArray_DIMS(yarr);
    npy_intp i,j,k,m;
    double * p;

    //initialize variable to hold result
    Py_complex result = {.real = 0, .imag = 0};
    std::cout << "Is complex" << iscomplex << std::endl;

    if (iscomplex) {
        for (i=0;i<dims[0];i++) 
            for (j=0;j<dims[1];j++) 
                for (k=0;k<dims[1];k++) {
                    p = (double*)PyArray_GETPTR3(yarr, i,j,k);
                    result.real += *p;
                    result.imag += *(p+1);
                }
    } else {
        for (i=0;i<dims[0];i++) 
            for (j=0;j<dims[1];j++) 
                for (k=0;k<dims[1];k++) {
                    p = (double*)PyArray_GETPTR3(yarr, i,j,k);
                    result.real += *p;
                }
    }

    //multiply by step size
    result.real *= (dx*dy*dz);
    result.imag *= (dx*dy*dz);

    Py_CLEAR(yarr);

    //copy result into returnable type with new reference
    if (iscomplex) {
        return Py_BuildValue("D", &result);
    } else {
        return Py_BuildValue("d", result.real);
    }

};

static PyObject *
example_wrapper(PyObject *dummy, PyObject *args)
{
    PyObject *arg1=NULL, *arg2=NULL, *out=NULL;
    PyObject *arr1=NULL, *arr2=NULL, *oarr=NULL;
    double *dptr;

    if (!PyArg_ParseTuple(args, "OOO!", &arg1, &arg2,
        &PyArray_Type, &out)) return NULL;

    arr1 = PyArray_FROM_OTF(arg1, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
    if (arr1 == NULL) return NULL;
    arr2 = PyArray_FROM_OTF(arg2, NPY_DOUBLE, NPY_ARRAY_IN_ARRAY);
    if (arr2 == NULL) goto fail;
#if NPY_API_VERSION >= 0x0000000c
    oarr = PyArray_FROM_OTF(out, NPY_DOUBLE, NPY_ARRAY_INOUT_ARRAY2);
#else
    oarr = PyArray_FROM_OTF(out, NPY_DOUBLE, NPY_ARRAY_INOUT_ARRAY);
#endif
    if (oarr == NULL) goto fail;
    

    std::cout << "arr1" << std::endl;
    dptr = (double *)PyArray_DATA(arr1);
    std::cout << "arrval = " << *dptr << std::endl;
    std::cout << "arrval = " << *(dptr+1) << std::endl;
    std::cout << "arrval = " << *(dptr+2) << std::endl;
    std::cout << "arrval = " << *dptr+3 << std::endl;
    std::cout << "arrval = " << *dptr+4 << std::endl;


    /* code that makes use of arguments */
    /* You will probably need at least
       nd = PyArray_NDIM(<..>)    -- number of dimensions
       dims = PyArray_DIMS(<..>)  -- npy_intp array of length nd
                                     showing length in each dim.
       dptr = (double *)PyArray_DATA(<..>) -- pointer to data.

       If an error occurs goto fail.
     */

    Py_DECREF(arr1);
    Py_DECREF(arr2);
    Py_DECREF(oarr);
    Py_INCREF(Py_None);
    return Py_None;

 fail:
    Py_XDECREF(arr1);
    Py_XDECREF(arr2);
    Py_XDECREF(oarr);
    return NULL;
}

static PyMethodDef tcontract_Methods[] = {
    {"integrate3",  integrate3, METH_VARARGS,
     "Pass 3D numpy array (double or complex) and dx,dy,dz step size. Returns Reimman integral"},
    {"example",  example_wrapper, METH_VARARGS,
     "Example from https://numpy.org/doc/stable/user/c-info.how-to-extend.html"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};


static struct PyModuleDef module = {
   PyModuleDef_HEAD_INIT,
   "tcontract",   /* name of module */
   NULL, /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   tcontract_Methods
};

PyMODINIT_FUNC
PyInit_tcontract(void)
{
    // Called on import
    // Returns a pointer to module, which is insected into `sys.modules`
    import_array(); // needed for numpy to work
    return PyModule_Create(&module);
}
