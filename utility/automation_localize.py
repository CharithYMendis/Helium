import os
import subprocess
import glob
import time
from stat import S_ISREG, ST_CTIME, ST_MODE

parent_folder = os.environ.get('EXALGO_PARENT_FOLDER')
diff_file_name = 'diff_photoshop.txt'

def get_sorted_on_time(files):
        entries = ((os.stat(path), path) for path in files)
        entries = ((stat[ST_CTIME], path)
           for stat, path in entries if S_ISREG(stat[ST_MODE]))
        return sorted(entries)

def get_coverage_files():
        folder = os.environ.get('EXALGO_OUTPUT_FOLDER')
        filter_folder = os.environ.get('EXALGO_FILTER_FOLDER')
        
        files = glob.glob(folder + '/drcov.Photoshop*')
        entries = get_sorted_on_time(files)
        entries = [x[1] for x in entries]
        entries = entries[::-1]
        return [entries[0],entries[1],filter_folder + '\\' + diff_file_name]

def run_code_coverage():
	os.chdir('../preprocess/code_cov')
	print 'Please run the application with the filter'
        p = subprocess.Popen('code_cov.bat m32 photoshop')
        p.communicate()
        print 'Now please run the application with the filter'
        p = subprocess.Popen('code_cov.bat m32 photoshop')
        p.communicate()
        
def run_code_diff(args):
        print 'Now running the code diff tool'
        command = 'run.bat ';
        for file in args:
                command += file + ' '
        command += 'Photoshop'
        os.chdir('../code_diff/utility')
        p = subprocess.Popen(command)
        p.communicate()

def run_profiling():
        os.chdir(parent_folder + '/dr_clients/utility')
        args = ['m32', 'photoshop', 'Photoshop', '1',
                'profile_memtrace', diff_file_name, 'bb', 'low.png',
                'low.png']
        command = 'run_tests.bat '
        for arg in args:
                command += arg + ' '
        print command
        p = subprocess.Popen(command)
        p.communicate()
        

def run_filter_funcs():
        os.chdir(parent_folder + '/preprocess/filter_funcs/build32/bin')
        args = ['-exec Photoshop',
                '-in_image low.png',
                '-out_image low.png',
                '-debug 1',
                '-debug_level 3',
                '-mode 1',
                '-total_size 0',
                '-threshold 80']
        command = 'filter.exe '
        for arg in args:
                command += arg + ' '
        p = subprocess.Popen(command)
        p.communicate()
        
        
if __name__ == '__main__':
        run_code_coverage()
        files = get_coverage_files()
        run_code_diff(files)
        run_profiling()
        run_filter_funcs()
        
    
