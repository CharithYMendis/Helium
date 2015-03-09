import os
import subprocess
import glob
import time
from stat import S_ISREG, ST_CTIME, ST_MODE
import getopt
import sys

parent_folder = os.environ.get('EXALGO_PARENT_FOLDER')

def run_instrace(program, executable, in_image, out_image, phase):
        os.chdir(parent_folder + '/dr_clients/utility')
        args = ['m32', program, executable, '1',
                phase, 'filter_' + executable + '.exe.log',
                'func', in_image, out_image]
        command = 'run_tests.bat '
        for arg in args:
                command += arg + ' '
        print command
        p = subprocess.Popen(command)
        p.communicate()
        
        
if __name__ == '__main__':
        executable = ''
        program = ''
        in_image = ''
        out_image = ''
        stage = ''
        
        try:
                opts, args = getopt.getopt(sys.argv[1:],"e:p:i:o:s:h",
                                           ["exec=","program=","in_image=","out_image=","stage="])
        except getopt.GetoptError:
                print 'test.py -e <exec>'
                sys.exit(2)
        for opt, arg in opts:
                if opt in ("-e","--exec"):
                        executable = arg
                elif opt in ("-p","--program"):
                        program = arg
                elif opt in ("-i","--in_image"):
                        in_image = arg
                elif opt in ("-o","--out_image"):
                        out_image = arg
                elif opt in ("-s","--stage"):
                        stage = arg
                elif opt == "-h":
                        print 'test.py -e <exec>'

        run_instrace(program, executable, in_image, out_image, stage)
                
        
        
    
