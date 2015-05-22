import os
import subprocess
import glob
import time
from stat import S_ISREG, ST_CTIME, ST_MODE
import sys
import getopt

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

def run_code_coverage(program):
	os.chdir(parent_folder + '/preprocess/code_cov')
	print 'Please run the application without the filter'
        p = subprocess.Popen('code_cov.bat m32 ' + program)
        p.communicate()
        print 'Now please run the application with the filter'
        p = subprocess.Popen('code_cov.bat m32 ' + program)
        p.communicate()
        
def run_code_diff(files,executable):
        print 'Now running the code diff tool'
        command = 'run.bat ';
        for file in files:
                command += file + ' '
        command += executable

	os.chdir(parent_folder + '/preprocess/code_diff/utility')
        p = subprocess.Popen(command)
        p.communicate()

def run_profiling(program, executable, in_image):
        os.chdir(parent_folder + '/dr_clients/utility')
        args = ['m32', program, executable, '1',
                'profile_memtrace', diff_file_name, 'bb', in_image,
                in_image]
        command = 'run_tests.bat '
        for arg in args:
                command += arg + ' '
        p = subprocess.Popen(command)
        p.communicate()
        

def run_filter_funcs(executable, in_image):
        os.chdir(parent_folder + '/preprocess/filter_funcs/build32/bin')
        args = ['-exec ' + executable,
                '-in_image ' + in_image,
                '-out_image ' + in_image,
                '-debug 1',
                '-debug_level 5',
                '-mode 1',
                '-total_size 0',
                '-threshold 80']
        command = 'filter.exe '
        for arg in args:
                command += arg + ' '
        p = subprocess.Popen(command)
        p.communicate()
        
        
if __name__ == '__main__':
        executable = ''
        program = ''
        filter = ''
	print sys.argv
	in_image = ''
        try:
                opts, args = getopt.getopt(sys.argv[1:],"e:p:f:h:i:",["exec=","program="])
        except getopt.GetoptError:
                print 'test.py -e <exec>'
                sys.exit(2)
        for opt, arg in opts:
                print opt, arg
                if opt in ("-e","--exec"):
                        executable = arg
                elif opt in ("-p","--program"):
                        program = arg
                elif opt == "-h":
                        print 'test.py -e <exec>'
                elif opt == "-f":
                        filter = arg
		elif opt == "-i":
                        in_image = arg

        print in_image
        
        run_code_coverage(program)
        files = get_coverage_files(executable)
        run_code_diff(files, filter)
        run_profiling(program, executable, in_image)
        run_filter_funcs(executable,in_image)
        
    
