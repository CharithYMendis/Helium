#include <Windows.h>
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <iostream>
#include <stdint.h>
#include "common_defines.h"
#include "utilities.h"

#define EPSILON 1e-6

using namespace std;

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems);


/* debug and helper functions */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

std::vector<cmd_args_t *> get_command_line_args(int argc, char ** argv){

	std::vector<cmd_args_t *> args;

	for (int i = 1; i < argc; i += 2){

		std::string name(argv[i]);
		std::string value(argv[i+1]);
	
		assert(name[0] == '-'); /* this ensures that first command line arg is actually a name*/

		cmd_args_t * arg = new cmd_args_t;
		arg->name = name;
		arg->value = value;
		args.push_back(arg);

	}

	return args;


}

vector<string> get_all_files_in_folder(string folder)
{
	vector<string> names;
	char search_path[200];
	sprintf(search_path, "%s\\*.*", folder.c_str());
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(search_path, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			// read all (real) files in current folder, delete '!' read other 2 default folder . and ..
			if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				names.push_back(fd.cFileName);
			}
		} while (::FindNextFile(hFind, &fd));
		::FindClose(hFind);
	}


	return names;
}

string get_standard_folder(string type){


	char * folder = NULL;
	if (type.compare("filter") == 0){
		folder = getenv(FILTER_FOLDER_ENV_VAR);
	}
	else if (type.compare("parent") == 0){
		folder = getenv(PARENT_FOLDER_ENV_VAR);
	}
	else if (type.compare("output") == 0){
		folder = getenv(OUTPUT_FOLDER_ENV_VAR);
	}
	else if (type.compare("log") == 0){
		folder = getenv(LOG_FOLDER_ENV_VAR);
	}
	else if (type.compare("halide") == 0){
		folder = getenv(HALIDE_FOLDER_ENV_VAR);
	}
	else if (type.compare("image") == 0){
		folder = getenv(IMAGE_FOLDER_ENV_VAR);
	}

	assert(folder != NULL);
	string folder_s(folder);

	
	return folder_s;

}


bool is_prefix(string str, string prefix){
	for (int i = 0; i < prefix.size(); i++){
		if (i >= str.size()) return false;
		if (str[i] != prefix[i]) return false;
	}
	return true;
}


void print_progress(uint32_t * count, uint32_t mod){

	if (debug && debug_level >= 3){
		(*count)++;
		if ( (*count % mod)  == 0){
			printf("-> progress - %d\n", *count);
		}
	}
}

bool is_overlapped(uint64_t start_1, uint64_t end_1, uint64_t start_2, uint64_t end_2){
	
	/* check whether there is an overlap between the two regions */

	bool one_in_two = (start_1 >= start_2) && (end_1 <= end_2);
	bool two_in_one = (start_2 >= start_1) && (end_2 <= end_1);

	bool partial_overlap = ((start_1 >= start_2) && (start_1 <= end_2)) || ((end_1 >= start_2) && (end_1 <= end_2));

	return one_in_two || two_in_one || partial_overlap;

}

int get_rank_matrice_row_echelon(vector < vector<double> > A){

	for (int i = 0; i < A.size(); i++){
		vector<double> row = A[i];
		int zeros = true;
		for (int j = 0; j < row.size(); j++){
			if (abs(row[j]) > EPSILON){
				zeros = false;
				break;
			}
		}

		if (zeros){
			return i;
		}

	}

	return A.size();

}

bool is_b_consistent(vector<double> b, uint32_t start){

	for (int i = start; i < b.size(); i++){
		if (b[i] > EPSILON){
			return false;
		}
	}

	return true;

}

void print_system(vector<vector<double> > A, vector<double> b){
	DEBUG_PRINT(("A\n"), 2);
	printout_matrices(A);
	DEBUG_PRINT(("b\n"), 2);
	printout_vector(b);
}

vector<double> solve_linear_eq(vector<vector<double> > A, vector<double> b){

	int M = b.size();
	int N = A[0].size();

	//M x N matrix

	if (M < N){
		print_system(A, b);
		ASSERT_MSG(false, ("ERROR: no of equations smaller than no of unknowns\n"));
	}

	for (int p = 0; p < N; p++) {

		// find pivot row and swap
		int max = p;
		for (int i = p + 1; i < M; i++) {
			if (abs(A[i][p]) > abs(A[max][p])) {
				max = i;
			}
		}
		vector<double> temp = A[p]; A[p] = A[max]; A[max] = temp;
		double   t = b[p]; b[p] = b[max]; b[max] = t;

		// singular or nearly singular
		if (abs(A[p][p]) <= EPSILON) {
			print_system(A, b);
			ASSERT_MSG((false), ("ERROR: the matrix is singular\n"));
		}

		// pivot within A and b
		for (int i = p + 1; i < M; i++) {
			double alpha = A[i][p] / A[p][p];
			b[i] -= alpha * b[p];
			for (int j = p; j < N; j++) {
				A[i][j] -= alpha * A[p][j];
			}
			A[i][p] = 0;
		}

		//printout_matrices(A);
		//printout_vector(b);
	}

	//get rank
	int rank = get_rank_matrice_row_echelon(A);


	if (rank < N) {
		print_system(A, b);
		ASSERT_MSG((false), (" not enough independent equations\n"));
	}
	else if ( (rank > N)  || !is_b_consistent(b,rank) ){
		print_system(A, b);
		ASSERT_MSG((false), (" equations are not consistent; system may be non linear\n"));
	}

	// back substitution
	vector<double> x(rank, 0.0);


	for (int i = rank - 1; i >= 0; i--) {
		double sum = 0.0;
		for (int j = i + 1; j < rank; j++) {
			sum += A[i][j] * x[j];
		}
		x[i] = (b[i] - sum) / A[i][i];
	}
	return x;


}

void test_linear_solver(){

	double A_arr[4][2] =
	{
		{ 1, 1 },
		{ 2, 1 },
		{ 3, 1 },
		{4, 1}
	};

	double B_arr[] = { 8, 14, 20, 26 };

	std::vector<std::vector<double> > A(4, std::vector<double>(2, 0.0));
	for (int i = 0; i < 4; ++i)
	{
		A[i].assign(A_arr[i], A_arr[i] + 2);
	}

	std::vector<double> b(B_arr, B_arr + 4);

	cout << "*** matrices ***" << endl;
	printout_matrices(A);
	printout_vector(b);

	vector<double> results = solve_linear_eq(A, b);

	cout << "*** results ***" << endl;
	for (int i = 0; i < results.size(); i++){
		cout << results[i] << endl;
	}


}

void printout_matrices(vector<vector<double> >  values){
	for (int i = 0; i < values.size(); i++){
		vector<double> row = values[i];
		for (int j = 0; j < row.size(); j++){
			cout << row[j] << " ";
		}
		cout << endl;
	}
}

void printout_vector(vector<double> values){
	for (int i = 0; i < values.size(); i++){
		cout << values[i] << " ";
	}
	cout << endl;
}

int double_to_int(double value){
	if (value >= 0){
		return (int)(value + 0.5);
	}
	else{
		return (int)(value - 0.5);
	}
}

vector<string> get_vars(string name, uint32_t amount){

	vector<string> vars;
	for (int i = 0; i < amount; i++){
		vars.push_back(name + "_" + to_string(i));
	}

	return vars;

}
