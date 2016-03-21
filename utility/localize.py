import os
import subprocess
import glob
import time
from stat import S_ISREG, ST_CTIME, ST_MODE

import common
import drclient


parent_folder = os.environ.get('EXALGO_PARENT_FOLDER')
diff_file_name = 'diff_photoshop.txt'

def get_sorted_on_time(files):
        entries = ((os.stat(path), path) for path in files)
        entries = ((stat[ST_CTIME], path)
           for stat, path in entries if S_ISREG(stat[ST_MODE]))
        return sorted(entries)

def get_coverage_files(executable):
        output_folder = os.environ.get('EXALGO_OUTPUT_FOLDER')
        filter_folder = os.environ.get('EXALGO_FILTER_FOLDER')
        
        files = glob.glob(output_folder + '/drcov.' + executable + '*')
        entries = get_sorted_on_time(files)
        entries = [x[1] for x in entries]
        entries = entries[::-1]
        return [entries[0],entries[1],filter_folder + '\\' + diff_file_name]
'''
This function runs the application under code coverage DynamoRIO tool
'''
def run_code_coverage(executable, args):
	os.chdir(parent_folder + '/preprocess/code_cov')
	print 'Please run the application without the filter'
	print 'code_cov_no_prep.bat m32 ' + executable + ' ' + args
        p = subprocess.Popen('code_cov_no_prep.bat m32 ' + executable + ' ' + args)
        p.communicate()
        print 'Now please run the application with the filter'
        print 'code_cov_no_prep.bat m32 ' + executable + ' ' + args
        p = subprocess.Popen('code_cov_no_prep.bat m32 ' + executable + ' ' + args)
        p.communicate()

'''
This function gets the code coverage diffs
'''
def run_code_diff(executable, pathname):

        files = get_coverage_files(executable)
        os.chdir(parent_folder + '/preprocess/code_diff/build32/bin')
        print 'Now running the code diff tool'
        command = 'code_diff.exe';
        command += ' -first ' + files[0]
        command += ' -second ' + files[1]
        command += ' -output ' + files[2]
        command += ' -exec ' + pathname
        p = subprocess.Popen(command)
        p.communicate()


'''
this function invokes the executable under profiling
'''
def run_profiling(path, executable, args, debug, in_image):
        filter_folder = os.environ.get('EXALGO_FILTER_FOLDER')
        drclient.run_drclients(path, executable, args, debug, 'profile,memtrace',
                      'bb',filter_folder + '\\' + diff_file_name, in_image, '')
        
'''
this function runs the filter funcs to get the localized filter
'''
def run_filter_funcs(executable, in_image, debug, debug_level, total_size):
        os.chdir(parent_folder + '/preprocess/filter_funcs/build32/bin')
        args = ['-exec ' + executable,
                '-in_image ' + in_image,
                '-out_image ' + in_image,
                '-debug ' + common.boolstr(debug),
                '-debug_level ' + debug_level,
                '-mode 1',
                '-total_size ' + total_size,
                '-threshold 80']
        command = 'filter.exe '
        for arg in args:
                command += arg + ' '
        p = subprocess.Popen(command)
        p.communicate()
        
        
    
