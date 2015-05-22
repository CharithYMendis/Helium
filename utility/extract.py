import os
import subprocess
import drclient
import common

parent_folder = os.environ.get('EXALGO_PARENT_FOLDER')

'''
this function invokes the executable under instruction tracing
'''
def run_instrace(path, executable, args, debug, in_image):
        filter_folder = os.environ.get('EXALGO_FILTER_FOLDER')
        drclient.run_drclients(path, executable, args, debug, 'funcwrap,instrace,memdump',
                      'func',filter_folder + '\\filter_' + executable + '.log' , in_image, 'instrace')
        drclient.run_drclients(path, executable, args, debug, 'funcwrap,instrace',
                      'func',filter_folder + '\\filter_' + executable + '.log' , in_image, 'ins_distrace')
        
def run_buildex(executable, in_image, out_image, debug, debug_level, dump, opts):
        parent_folder = os.environ.get('EXALGO_PARENT_FOLDER')
        os.chdir(parent_folder + '/postprocess/buildex/build32/bin')
        command = 'buildex.exe ';
        args = [ '-exec ' + executable,
                 '-in_image ' + in_image,
                 '-out_image ' + out_image,
                 '-debug ' + common.boolstr(debug),
                 '-debug_level ' + debug_level,
                 '-dump ' + dump,
                  ]
        for arg in args:
                command += ' ' + arg
        command += ' ' + opts
        print command
        p = subprocess.Popen(command)
        p.communicate()
                 
                 
        
        
                
        
        
    
