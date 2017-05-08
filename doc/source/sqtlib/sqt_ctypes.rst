.. _sqt_ctypes:

==================
C types library
==================

The system library exposes operating system facilities like environment variables,
date time manipulation etc..

--------------
Squirrel API
--------------

++++++++++++++
Global Symbols
++++++++++++++

.. js:data:: ctypes

    A table containing all basic C types and classes.

	C classes are: basictype, basicarray, basicmember, basicstruct and basicvalue.

    Predefined C types are:
	
+-------------+----------------------------------------+
| uint8_t     | As in stdint.h                         |
+-------------+----------------------------------------+
| int8_t      | As in stdint.h                         |
+-------------+----------------------------------------+
| uint16_t    | As in stdint.h                         |
+-------------+----------------------------------------+
| int16_t     | As in stdint.h                         |
+-------------+----------------------------------------+
| uint32_t    | As in stdint.h                         |
+-------------+----------------------------------------+
| int32_t     | As in stdint.h                         |
+-------------+----------------------------------------+
| uint64_t    | If squirrel is comiled as 64bit        |
+-------------+----------------------------------------+
| int64_t     | If squirrel is comiled as 64bit        |
+-------------+----------------------------------------+
| voidptr     | Void pointer                           |
+-------------+----------------------------------------+
| float       | Compiler dependant                     |
+-------------+----------------------------------------+
| double      | Compiler dependant                     |
+-------------+----------------------------------------+
| SQInteger   | As defined by squirrel                 |
+-------------+----------------------------------------+
| SQFloat     | As defined by squirrel                 |
+-------------+----------------------------------------+

++++++++++++++++
Class basictype
++++++++++++++++

    Base class of all C types

.. js:class:: basictype( descr)

    :param userpointer descr: Pointer to base types descriptor.
	
.. note:: This class is not designed to be insantiated from script.

.. js:function:: size()

    Returns size in bytes of type.

.. js:function:: align()

    Returns align in bytes of type.

.. js:function:: name()

    Returns a string nameof type.

.. js:function:: array( length)

    :param int length: Length of array in elements.
	
    Returns new array of this type with `length`.

.. js:function:: member( name)

    :param string name: Name of member.
	
    Returns new member object of this type with `name`.

++++++++++++++++++
Class basicarray
++++++++++++++++++

    Array type class

.. js:class:: basicarray( oftype, length)

    :param basetype oftype: Type of array element.
    :param int length: Length of array in elements.

	Create new array type.	

.. js:function:: len()

    Returns length of array in elements.

.. js:function:: type()

    Returns type of array element.

.. js:function:: refmember( data, offset, index)

    :param userpointer data: Userdata or uerpointer to data
    :param int offset: Offset in data where array begins.
    :param int index: Index of array element.
	
    Returns new basicvalue representing array element.

++++++++++++++++++
Class basicmember
++++++++++++++++++

	Class to represent C member (name, type, offset)

.. js:class:: basicmember( name, type)

    :param string name: Member name.
    :param basetype type: Type of member.

	Create new member.	

.. js:function:: type()

    Returns type of member.

.. js:function:: name()

    Returns name of member.

.. js:function:: offset()

    Returns offset of member. For unassigned embers it is -1, for assigned to structure members it is offset inside struct.

++++++++++++++++++
Class basicstruct
++++++++++++++++++

	Structure type class.

.. js:class:: basicstruct( name, members)

    :param string name: Structure name.
    :param array members: Array of members. If ommited, structure without members is creted. Members can be added lated with setmembers().

	Create new structure.

.. js:function:: setmembers( members)

    :param array members: Array of members.
	
	Set structure members.

.. js:function:: refmember( data, offset, index)

    :param userpointer data: Userdata or uerpointer to data
    :param int offset: Offset in data where structure begins.
    :param string index: Name of member.
	
.. js:function:: name()

    Returns name of member.

.. js:function:: len()

    Returns count of members.

++++++++++++++++++
Class basicvalue
++++++++++++++++++

    Representation of C data.

.. js:class:: basicvalue( type, data, offset)

    :param basictype type: Type of value.
    :param userpointer data: Userdata or uerpointer to data
    :param int offset: Offset in data where value begins.
	
.. js:function:: set( val)

    :param val: Value to be assigned.
	
	Assigns new value to C data. `val` can be any compatible squirrel type or basicvalue.

.. js:function:: _set( index, val)

    :param index: Index to be assigned to.
    :param val: Value to be assigned.
	
	Assigns new value at `index` in C data. `val` can be any compatible squirrel type or basicvalue.
	
.. js:function:: get()

    returns squirrel representation of value stored in C data.

.. js:function:: _get( index)

    :param index: Index in C data.
	
    returns squirrel representation value stored at `index` in C data.

.. js:function:: getref( index)

    :param index: Index in C data.
	
    returns basicvalue representing C data stored at `index`.

.. js:function:: clear()

    Clears (set to sero) C data, returns this basicvalue.

.. js:function:: _cloned( other)

	Allocates new userdata containing copy of data from cloned object.

--------------
C API
--------------

.. _sqstd_register_systemlib:

.. c:function:: SQRESULT sqstd_register_systemlib(HSQUIRRELVM v)

    :param HSQUIRRELVM v: the target VM
    :returns: an SQRESULT
    :remarks: The function aspects a table on top of the stack where to register the global library functions.

    initialize and register the system library in the given VM.