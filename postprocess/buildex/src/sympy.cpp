#include "sympy.h"
#include <Python.h>
#include <string>
#include "../../dr_clients/include/output.h"
#include "node.h"
#include "canonicalize.h"
#include <stdint.h>
#include "build_abs_tree.h"
#include "print_common.h"

using namespace std;


int sym_ex(const char * input_expression, const char * output_expression){

		PyObject *pName, *pModule, *pDict, *pFunc;
		PyObject *pArgs, *pValue;
		int i;

		const char * file = "sympy";
		const char * function = "simplify";


		Py_Initialize();
		//PyRun_SimpleString("from sympy import *\n");
		pName = PyString_FromString(file);
		/* Error checking of pName left out */

		pModule = PyImport_Import(pName);
		Py_DECREF(pName);

		if (pModule != NULL) {
			pFunc = PyObject_GetAttrString(pModule, function);
			/* pFunc is a new reference */

			if (pFunc && PyCallable_Check(pFunc)) {

				pArgs = PyTuple_New(1);
				pValue = PyString_FromString(input_expression);
				printf("the string passed %s\n", input_expression);
				if (!pValue) {
					Py_DECREF(pArgs);
					Py_DECREF(pModule);
					fprintf(stderr, "Cannot convert argument\n");
					return 1;
				}
				/* pValue reference stolen here: */
				PyTuple_SetItem(pArgs, 0, pValue);
				
				pValue = PyObject_CallObject(pFunc, pArgs);
				Py_DECREF(pArgs);
				if (pValue != NULL) {
					printf("Result of call: %s\n", PyString_AsString(pValue));
					Py_DECREF(pValue);
				}
				else {
					Py_DECREF(pFunc);
					Py_DECREF(pModule);
					PyErr_Print();
					fprintf(stderr, "Call failed\n");
					return 1;
				}
			}
			else {
				if (PyErr_Occurred())
					PyErr_Print();
				fprintf(stderr, "Cannot find function \"%s\"\n", function);
			}
			Py_XDECREF(pFunc);
			Py_DECREF(pModule);
		}
		else {
			PyErr_Print();
			fprintf(stderr, "Failed to load \"%s\"\n", file);
			return 1;
		}
		Py_Finalize();
		return 0;

}


/* print the expression for simplification  */
string encode_operand(operand_t * opnd){

	string label = "";

	if (opnd->type == IMM_INT_TYPE){
		label += to_string((int32_t)opnd->value);
	}
	else if (opnd->type == IMM_FLOAT_TYPE){
		label += to_string(opnd->float_value);
	}
	else{
		if (opnd->type == MEM_HEAP_TYPE){
			label += "mh_";
		}
		else if (opnd->type == MEM_STACK_TYPE){
			label += "ms_";
		}
		else if (opnd->type == REG_TYPE){
			label += "r_" + regname_to_string(mem_range_to_reg(opnd)) + "_";
		}
		label += to_string(opnd->value) + "_" + to_string(opnd->width);
	}

	return label;

}

bool is_prefix(uint32_t operation){

	switch (operation){

	case op_add: 
	case op_sub: 
	case op_mul: 
	case op_div: 
	case op_mod: 
		return true;
	default:
		return false;
	}


}

string operation_to_string_sym(uint32_t operation){

	switch (operation){

	case op_add: return "+";
	case op_sub: return "-";
	case op_mul: return "*";
	case op_div: return "/";
	case op_mod: return "%";

	case op_assign: return "ASS";
	case op_lsh: return "LSH";
	case op_rsh: return "RSH";
	case op_not: return "NOT";
	case op_xor: return "XOR";
	case op_and: return "AND";
	case op_or: return "OR";
	case op_concat: return "CON";
	case op_signex: return "SE";
	case op_full_overlap: return "FO";
	case op_partial_overlap: return "PO";
	case op_split_l: return "SL";
	case op_split_h: return "SH";
	default: return "__";

	}

}

string get_simplify_string(Node * node){

	if (node->srcs.size() == 0){
		return encode_operand(node->symbol);
		//cout << encode_operand(node->symbol);
	}
	else{
		if (is_prefix(node->operation)){
			string ret = "(";
			for (int i = 0; i < node->srcs.size(); i++){
				ret += get_simplify_string(node->srcs[i]);
				if (i != node->srcs.size() - 1){
					ret += operation_to_string_sym(node->operation);
				}
			}
			ret += ")";
			//cout << ret;
			return ret;
		}
		else{
			string ret = operation_to_string_sym(node->operation) + "(";
			for (int i = 0; i < node->srcs.size(); i++){
				ret += get_simplify_string(node->srcs[i]);
				if (i != node->srcs.size() - 1){
					ret += ",";
				}
			}
			ret += ")";
			//cout << ret;
			return ret;
		}
	}


}

