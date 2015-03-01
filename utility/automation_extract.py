import os
import subprocess
import glob
import time
from stat import S_ISREG, ST_CTIME, ST_MODE

parent_folder = os.environ.get('EXALGO_PARENT_FOLDER')


def run_instrace_memdump():
        return

def run_filter_funcs(in_image, out_image):
        os.chdir(parent_folder + '/postprocess/buildex/build32/bin')
        args = ['-exec Photoshop',
                '-in_image low.png',
                '-out_image low.png',
                '-debug 1',
                '-debug_level 3',
                '-mode 1',
                '-total_size 0',
                '-threshold 80']
        command = 'buildex.exe '
        for arg in args:
                command += arg + ' '
        p = subprocess.Popen(command)
        p.communicate()
        
        
if __name__ == '__main__':
        try:
                opts, args - getopt.getopt(sys.argv[1:],"i:o:")
        
    
