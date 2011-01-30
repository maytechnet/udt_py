#include <iostream>
#include <udt.h>
#include <Python.h>
#include <arpa/inet.h>

#include "udt_py.h"

PyObject* pyudt_socket_accept(PyObject *self, PyObject *args, PyObject *kwargs);

#define PY_TRY_CXX \
try \
{

#define PY_CATCH_CXX(ret_val) \
} \
catch(py_udt_error& e) \
{ \
    try \
    { \
        UDT::ERRORINFO& udt_err = UDT::getlasterror(); \
        PyErr_SetString(PyExc_RuntimeError, udt_err.getErrorMessage()); \
        UDT::getlasterror().clear(); \
        return ret_val; \
    } \
    catch(...)\
    {\
        PyErr_SetString(PyExc_RuntimeError, "UDT error"); \
        UDT::getlasterror().clear(); \
        return ret_val; \
    }\
} \
catch(std::exception& e) \
{ \
    PyErr_SetString(PyExc_RuntimeError, e.what()); \
    return ret_val; \
} \
catch(...) \
{\
    PyErr_SetString(PyExc_RuntimeError, "C++ error"); \
    return ret_val; \
}

PyObject* pyudt_cleanup(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    if(!PyArg_ParseTuple(args, ""))
    {
        return NULL;
    }

    if (UDT::cleanup() == UDT::ERROR)
    {
        throw py_udt_error();
    }

    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_startup(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    if(!PyArg_ParseTuple(args, ""))
    {
        return NULL;
    }

    if (UDT::startup() == UDT::ERROR)
    {
        throw py_udt_error();
    }

    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}

RecvBuffer::RecvBuffer(unsigned int len)
{
    head = new char[len + 1];
    buf_len = 0;
    max_buf_len = len;
    head[max_buf_len] = '\0';
}

RecvBuffer::~RecvBuffer()
{
    delete[] head;
}

char *RecvBuffer::get_head()
{
    return head;
}

unsigned int RecvBuffer::get_max_len()
{
    return max_buf_len;
}

unsigned int RecvBuffer::get_buf_len()
{
    return buf_len;
}

unsigned int RecvBuffer::set_buf_len(unsigned int new_len)
{
    /* FIXME - overflow check required */
    buf_len = new_len;
    head[buf_len] = '\0';
    return buf_len;
}

static PyObject* pyudt_socket_get_family(PyObject *py_socket)
{
    return Py_BuildValue("i", ((pyudt_socket_object*)py_socket)->family);
}

static PyObject* pyudt_socket_get_type(PyObject *py_socket)
{
    return Py_BuildValue("i", ((pyudt_socket_object*)py_socket)->type);
}

static PyObject* pyudt_socket_get_proto(PyObject *py_socket)
{
    return Py_BuildValue("i", ((pyudt_socket_object*)py_socket)->proto);
}

PyObject* pyudt_socket_connect(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    char *address;
    int port = 0;
    
    sockaddr_in serv_addr;

    pyudt_socket_object* py_socket  = ((pyudt_socket_object*)self);
    if(!PyArg_ParseTuple(args, "(si)", &address, &port))
    {
        return NULL;
    }

    serv_addr.sin_family = py_socket->family;
    serv_addr.sin_port = htons(port);
    int res = inet_pton(py_socket->family, address, &serv_addr.sin_addr);

    if(res == 0)
    {
        PyErr_SetString(PyExc_ValueError, "bad address");
        return NULL;
    }
    else if(res == -1 && errno == EAFNOSUPPORT)
    {
        PyErr_SetString(PyExc_ValueError, "address family not supported");
        return NULL;
    }

    memset(&(serv_addr.sin_zero), '\0', 8);

    if (UDT::ERROR == UDT::connect(
            py_socket->cc_socket, 
            (sockaddr*)&serv_addr, sizeof(serv_addr)))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}


PyObject* pyudt_socket_bind(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    char *address;
    int port;
    
    sockaddr_in serv_addr;

    pyudt_socket_object* py_socket  = ((pyudt_socket_object*)self);
    if(!PyArg_ParseTuple(args, "(si)", &address, &port))
    {
        return NULL;
    }

    int res = inet_pton(py_socket->family, address, &serv_addr.sin_addr);

    serv_addr.sin_family = py_socket->family;
    serv_addr.sin_port = htons(port);

    if(res == 0)
    {
        PyErr_SetString(PyExc_ValueError, "bad address");
        return NULL;
    }
    else if(res == -1 && errno == EAFNOSUPPORT)
    {
        PyErr_SetString(PyExc_ValueError, "address family not supported");
        return NULL;
    }

    memset(&(serv_addr.sin_zero), '\0', 8);
        
    if (UDT::ERROR == UDT::bind(
            py_socket->cc_socket, 
            (sockaddr*)&serv_addr, sizeof(serv_addr)))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_close(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    if(UDT::close(((pyudt_socket_object*)self)->cc_socket))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;
PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_listen(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int backlog;
    if(!PyArg_ParseTuple(args, "i", &backlog))
    {
        return NULL;
    }

    if(UDT::listen(((pyudt_socket_object*)self)->cc_socket, backlog))
    {
        throw py_udt_error();
    }
    Py_RETURN_NONE;

PY_CATCH_CXX(NULL)
}
    
int pyudt_socket_init(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    int family;
    int type; 
    int proto;

    if(!PyArg_ParseTuple(args, "iii", &family, &type, &proto))
    {
        return -1;
    }

    if(family != AF_INET)
    {
        PyErr_SetString(PyExc_ValueError, "UDT only supports AF_INET addresses");
        return -1;
    }

    UDTSOCKET cc_socket = UDT::socket(family, type, proto);
    if(cc_socket == -1)
    {
        PyErr_SetString(PyExc_RuntimeError, "socket creation returned -1");
        return -1;
    }

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    py_socket->cc_socket = cc_socket;
    py_socket->family = family;
    py_socket->type = type;
    py_socket->proto = proto;

    return 0;
PY_CATCH_CXX(-1)
}

PyObject* pyudt_socket_send(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    char* buf;
    int   buf_len;
    int   flags;
    int   send_len = 0;

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "z#i", &buf, &buf_len, &flags))
    {
        return NULL;
    }
    
    send_len = UDT::send(py_socket->cc_socket, buf, buf_len, flags);

    if(send_len == UDT::ERROR) 
    {
        throw py_udt_error();
    }
    return Py_BuildValue("i", send_len);

PY_CATCH_CXX(NULL)
}


PyObject* pyudt_socket_sendmsg(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    char* buf;
    int   buf_len;
    int   ttl;
    int   send_len = 0;
    int   in_order = 0;

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "z#ii", &buf, &buf_len, &ttl, &in_order))
    {
        return NULL;
    }
    
    send_len = UDT::sendmsg(py_socket->cc_socket, buf, buf_len, ttl, in_order);

    if(send_len == UDT::ERROR) 
    {
        throw py_udt_error();
    }
    return Py_BuildValue("i", send_len);

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_recv(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    int len;
    int flags;
    int recv_len = 0;

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "ii", &len, &flags))
    {
        return NULL;
    }
    
    RecvBuffer recv_buf(len);
    recv_len = UDT::recv(py_socket->cc_socket, recv_buf.get_head(), len, flags);

    if(recv_len == UDT::ERROR) 
    {
        throw py_udt_error();
    }

    recv_buf.set_buf_len(recv_len);
    return Py_BuildValue("z#", recv_buf.get_head(), recv_len);

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_recvmsg(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX

    int len;
    int recv_len = 0;

    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "i", &len))
    {
        return NULL;
    }
    
    RecvBuffer recv_buf(len);
    recv_len = UDT::recvmsg(py_socket->cc_socket, recv_buf.get_head(), len);

    if(recv_len == UDT::ERROR) 
    {
        throw py_udt_error();
    }

    recv_buf.set_buf_len(recv_len);
    return Py_BuildValue("z#", recv_buf.get_head(), recv_len);

PY_CATCH_CXX(NULL)
}

PyObject* pyudt_socket_perfmon(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    int clear = 0;
    pyudt_socket_object* py_socket = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, "i", &clear))
    {
        return NULL;
    }

    UDT::TRACEINFO info;
    if(UDT::perfmon(py_socket->cc_socket, &info, clear) == UDT::ERROR)
    {
        throw py_udt_error();
    }
    /* Slow */   
    return Py_BuildValue(
        "{sLsLsLsisisisisisisisLsLsisisisisisisisdsdsdsisisisdsdsisi}",
        /* Aggregate Values */
        "msTimeStamp",   
        info.msTimeStamp,   
        "pktSentTotal",
        info.pktSentTotal,
        "pktRecvTotal",
        info.pktRecvTotal,
        "pktSndLossTotal",
        info.pktSndLossTotal,
        "pktRcvLossTotal",
        info.pktRcvLossTotal,
        "pktRetransTotal",
        info.pktRetransTotal,
        "pktSentACKTotal",
        info.pktSentACKTotal,
        "pktRecvACKTotal",
        info.pktRecvACKTotal,
        "pktSentNAKTotal",
        info.pktSentNAKTotal,
        "pktRecvNAKTotal",
        info.pktRecvNAKTotal,
        /* Local Values */
        "pktSent",   
        info.pktSent,   
        "pktRecv",   
        info.pktRecv,   
        "pktSndLoss",
        info.pktSndLoss,
        "pktRcvLoss",
        info.pktRcvLoss,
        "pktRetrans",
        info.pktRetrans,
        "pktSentACK",
        info.pktSentACK,
        "pktRecvACK",
        info.pktRecvACK,
        "pktSentNAK",
        info.pktSentNAK,
        "pktRecvNAK",
        info.pktRecvNAK,
        "mbpsSendRate",
        info.mbpsSendRate,
        "mbpsRecvRate",
        info.mbpsRecvRate,
        /* Instant Values */
        "usPktSndPeriod",
        info.usPktSndPeriod,
        "pktFlowWindow",
        info.pktFlowWindow,
        "pktCongestionWindow",
        info.pktCongestionWindow,
        "pktFlightSize",
        info.pktFlightSize,
        "msRTT",
        info.msRTT,
        "mbpsBandwidth",
        info.mbpsBandwidth,
        "byteAvailSndBuf",
        info.byteAvailSndBuf,
        "byteAvailRcvBuf",
        info.byteAvailRcvBuf
    );

PY_CATCH_CXX(NULL)
}
static PyMethodDef pyudt_socket_methods[] = {
    {
        "accept",  
        (PyCFunction)pyudt_socket_accept, 
        METH_VARARGS, 
        "socket accept"
    },
    {
        "connect",   
        (PyCFunction)pyudt_socket_connect,  
        METH_VARARGS, 
        "socket connect"
    },
    {
        "close",   
        (PyCFunction)pyudt_socket_close,  
        METH_VARARGS, 
        "close the socket"
    },
    {
        "bind",    
        (PyCFunction)pyudt_socket_bind,  
        METH_VARARGS, 
        "socket bind"
    },
    {
        "listen",   
        (PyCFunction)pyudt_socket_listen,  
        METH_VARARGS, 
        "listen with backlog"
    },
    {
        "send",  
        (PyCFunction)pyudt_socket_send, 
        METH_VARARGS, 
        "send data"
    },
    {
        "sendmsg",  
        (PyCFunction)pyudt_socket_sendmsg, 
        METH_VARARGS, 
        "send msg"
    },
    {
        "recv",  
        (PyCFunction)pyudt_socket_recv, 
        METH_VARARGS, 
        "recv data"
    },
    {
        "recvmsg",  
        (PyCFunction)pyudt_socket_recvmsg, 
        METH_VARARGS, 
        "recv msg"
    },
    {
        "perfmon",  
        (PyCFunction)pyudt_socket_perfmon, 
        METH_VARARGS, 
        "get perfmon stats"
    },
    {NULL}  /* Sentinel */
};

static PyGetSetDef pyudt_socket_getset[] = {
    {
        (char*)"family", 
        (getter)pyudt_socket_get_family, 
        NULL,
        (char*)"address family",
        NULL
    },
    {
        (char*)"type", 
        (getter)pyudt_socket_get_type, 
        NULL,
        (char*)"address type",
        NULL
    },
    {
        (char*)"proto", 
        (getter)pyudt_socket_get_proto, 
        NULL,
        (char*)"address protocol",
        NULL
    },
    {NULL}  /* Sentinel */
};


static PyMethodDef pyudt_methods[] = {
    {
        (char*)"startup", 
        (PyCFunction)pyudt_startup, 
        METH_VARARGS,
        (char*)"startup-up UDT library",
    },
    {
        (char*)"cleanup", 
        (PyCFunction)pyudt_cleanup, 
        METH_VARARGS,
        (char*)"clean-up UDT library",
    },
    {NULL, NULL, 0, NULL}   
};

static PyTypeObject pyudt_socket_type = {
    PyObject_HEAD_INIT(NULL)
    0,                              /* ob_size */
    "_udt.socket",                  /* tp_name */
    sizeof(pyudt_socket_object),    /* tp_basicsize */
    0,                              /* tp_itemsize */
    0,                              /* tp_dealloc */
    0,                              /* tp_print */
    0,                              /* tp_getattr */
    0,                              /* tp_setattr */
    0,                              /* tp_compare */
    0,                              /* tp_repr */
    0,                              /* tp_as_number */
    0,                              /* tp_as_sequence */
    0,                              /* tp_as_mapping */
    0,                              /* tp_hash */
    0,                              /* tp_call */
    0,                              /* tp_str */
    0,                              /* tp_getattro */
    0,                              /* tp_setattro */
    0,                              /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/* tp_flags */
    "UDT socket",                   /* tp_doc */
    0,                              /* tp_traverse */
    0,                              /* tp_clear */
    0,                              /* tp_richcompare */
    0,                              /* tp_weaklistoffset */
    0,                              /* tp_iter */
    0,                              /* tp_iternext */
    pyudt_socket_methods,           /* tp_methods */
    0,                              /* tp_members */
    pyudt_socket_getset,            /* tp_getset */
    0,                              /* tp_base */
    0,                              /* tp_dict */
    0,                              /* tp_descr_get */
    0,                              /* tp_descr_set */
    0,                              /* tp_dictoffset */
    (initproc)pyudt_socket_init,    /* tp_init */
    0,                              /* tp_alloc */
    0,                              /* tp_new */

};

PyObject* pyudt_socket_accept(PyObject *self, PyObject *args, PyObject *kwargs)
{
PY_TRY_CXX
    // char *address;
    // int port;
    int namelen;
    
    sockaddr_in client_addr;

    pyudt_socket_object* py_socket  = ((pyudt_socket_object*)self);

    if(!PyArg_ParseTuple(args, ""))
    {
        return NULL;
    }

    UDTSOCKET cc_client = UDT::accept(
        py_socket->cc_socket, 
        (sockaddr*)&client_addr, 
        &namelen
    );

    if (cc_client == UDT::ERROR)
    {
        throw py_udt_error();
    }

    pyudt_socket_object* py_client = (pyudt_socket_object*)PyObject_New(
            pyudt_socket_object, 
            &pyudt_socket_type
    );

    py_client->cc_socket = cc_client;
    py_client->family = client_addr.sin_family;
    py_client->type =  py_socket->type;     // FIXME
    py_client->proto = py_socket->proto;     // FIXME
    return (PyObject*)py_client;


PY_CATCH_CXX(NULL)
}


PyMODINIT_FUNC
init_udt(void)
{
    PyObject *module;

    pyudt_socket_type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&pyudt_socket_type) < 0)
        return;

    module = Py_InitModule("_udt", pyudt_methods);
    if (module == NULL)
    {
        return;
    }
    Py_INCREF(&pyudt_socket_type);
    PyModule_AddObject(module, "socket", (PyObject *)&pyudt_socket_type);

    if(PyModule_AddIntConstant(module, "UDT_MSS",         UDT_MSS)      == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_SNDSYN",      UDT_SNDSYN)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_RCVSYN",      UDT_RCVSYN)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_CC",          UDT_CC)       == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_FC",          UDT_FC)       == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_SNDBUF",      UDT_SNDBUF)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_RCVBUF",      UDT_RCVBUF)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_LINGER",      UDT_LINGER)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDP_SNDBUF",      UDP_SNDBUF)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDP_RCVBUF",      UDP_RCVBUF)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_MAXMSG",      UDT_MAXMSG)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_MSGTTL",      UDT_MSGTTL)   == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_RENDEZVOUS",  UDT_RENDEZVOUS) == -1 ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_SNDTIMEO",    UDT_SNDTIMEO) == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_RCVTIMEO",    UDT_RCVTIMEO) == -1   ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_REUSEADDR",   UDT_REUSEADDR) == -1  ) {return;}
    if(PyModule_AddIntConstant(module, "UDT_MAXBW",       UDT_MAXBW)    == -1   ) {return;}
}