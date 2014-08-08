#include <Windows.h>
#include "utilities.h"

using namespace std;

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

bool is_prefix(string str, string prefix){
	for (int i = 0; i < prefix.size(); i++){
		if (i >= str.size()) return false;
		if (str[i] != prefix[i]) return false;
	}
	return true;
}













