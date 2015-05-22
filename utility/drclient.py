import os
import subprocess
import common


def get_drclient_command(client_args,exec_args):
        dr32 = os.environ.get('DYNAMORIO_32_RELEASE_HOME')
        dr_path = dr32 + '/bin32/drrun.exe'
        command = dr_path + ' -root ' + dr32 + ' -syntax_intel -c exalgo.dll ' + client_args + ' -- ' + exec_args
        return command

def get_filter_mode(filter_string):
        filter_list = ['bb','module','range','func','neg_module','none','app_pc','nudge']
        return str(filter_list.index(filter_string) + 1)


def get_instrace_mode(instrace_string):
        if instrace_string == '':
                return ''
        else:
                instrace_list = ['opndtrace','opcodetrace','disasmtrace','instrace','ins_distrace']
                return str(instrace_list.index(instrace_string) + 1)

        
def create_client_args(clients,executable,
                       filter_file,filter_mode,
                       instrace_mode,in_image,
                       debug):

        filter_mode_num = get_filter_mode(filter_mode)
        filter_string = filter_file + ' ' + filter_mode_num
        instrace_mode_num = get_instrace_mode(instrace_mode)

        output_folder = os.environ.get('EXALGO_OUTPUT_FOLDER')
        log_folder = os.environ.get('EXALGO_LOG_FOLDER')
        filter_folder = os.environ.get('EXALGO_FILTER_FOLDER')

        md_app_pc_file = filter_folder + '\\filter_' + executable + '_app_pc.log'

        debug_value = common.boolstr(debug)
        
        client_args = '-logdir ' + log_folder + ' -debug ' + debug_value + ' -log 1 -exec ' + executable
        split = clients.split(',')
        
        for client in split:
                if client == 'functrace':
                        client_args += ' -functrace ' + filter_string
                if client == 'profile':
                        client_args += ' -profile ' + filter_string + ' ' + output_folder + ' ' + in_image 
                if client == 'memtrace':
                        client_args += ' -memtrace ' + filter_string + ' ' + output_folder + ' ' + in_image
                if client == 'funcwrap':
                        client_args += ' -funcwrap ' + filter_file
                if client == 'cpuid':
                        client_args += ' -cpuid'
                if client == 'inscount':
                        client_args += ' -inscount ' + filer_string
                if client == 'instrace':
                        client_args += ' -instrace ' + filter_string + ' ' + output_folder + ' 600000 ' + instrace_mode_num + ' ' + in_image
                if client == 'memdump':
                        client_args += ' -memdump ' +  filter_string + ' ' + md_app_pc_file + ' ' + output_folder
                if client == 'funcreplace':
                        client_args += ' -funcreplace ' + filter_string
                if client == 'misc':
                        client_args += ' -misc ' + filter_string

        return client_args
                        
        

        
def run_drclients(path,executable,args,
                  debug,clients,
                  filter_mode,filter_file,
                  input_image,instrace_mode):

        client_args = create_client_args(clients,executable,filter_file,filter_mode,instrace_mode,input_image,debug)
        dr_client_command = get_drclient_command(client_args,path + ' ' + common.xstr(args))

        parent_folder = os.environ.get('EXALGO_PARENT_FOLDER')
        os.chdir(parent_folder + '/dr_clients/build32/bin')
        p = subprocess.Popen(dr_client_command)
        p.communicate()
        
        
        
