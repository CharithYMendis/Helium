import argparse
import localize
import extract
import common
import os,shutil


def parse_arguments():
        
        parser = argparse.ArgumentParser(description="Helium automation script")
        #create argument group for the required arguments
        required = parser.add_argument_group('required','mandatory arguments')
        required.add_argument('--path','-p',required=True, help='full path to the executable')
        required.add_argument('--stage','-s', required=True, help='which Helium stage should be run',
                              choices=['all','local','extract','buildex'])
        required.add_argument('--exe','-e',required=True, help='Program Name')

        #optional, but recommended arguments
        recommended = parser.add_argument_group('recommended','these arguments are mandatory for certain stages')
        #recommended.add_argument('--word','-w', help='filter phrase (localization)')
        recommended.add_argument('--f_image','-f', help='filter image (localization)')
        recommended.add_argument('--tsize','-t', help='for non-image programs specify a size of the buffer (localization)')
        recommended.add_argument('--in_image','-i', help='input image (extraction)')
        recommended.add_argument('--out_image','-o', help='output image (extraction)')
        recommended.add_argument('--clean','-c',help='cleans the output directory',default=False,action='store_true')
        
        #optional arguments, tweak if something goes wrong or debugging is needed
        optional = parser.add_argument_group('optional', 'debugging and tweaking arguments')
        optional.add_argument('--debug','-d', default=False, action='store_true', help='turns on debugging')
        optional.add_argument('--args' ,'-a', help='arguments for the executable')
        optional.add_argument('--debug_level','-dl', default='2', help='specifies the debug level')
        optional.add_argument('--buildex_args','-ba', help='specifies overriding optional arguments to buildex in double quotes')
       
        args = parser.parse_args()
        print args
        return args

def delete_output():
        folder = os.environ.get('EXALGO_OUTPUT_FOLDER')
        for the_file in os.listdir(folder):
            file_path = os.path.join(folder, the_file)
            try:
                if os.path.isfile(file_path):
                    os.unlink(file_path)
                #elif os.path.isdir(file_path): shutil.rmtree(file_path)
            except Exception, e:
                print e

def main():

        args = parse_arguments()

        #populating some variables
        exec_name = common.get_executable_name(args.path)
        pathname = args.exe
        path = '\"' + args.path + '\"'

        total_size = '0'
        dump = '1'
        if args.tsize is not None:
                total_size = str(args.tsize)
                dump = '0'
                
        buildex_opts = ''
        if args.buildex_args != None:
                buildex_opts = args.buildex_args

        #check stage-wise arguments
        if args.stage == 'all' or args.stage == 'local':
                if args.f_image == None:
                        raise argparse.ArgumentTypeError('need to specify f_image for localization')
        if args.stage == 'all' or args.stage == 'extract' or args.stage == 'buildex':
                if args.in_image == None or args.out_image == None:
                        raise argparse.ArgumentTypeError('need to specify in_image and out_image for extraction')

        #delete files
        if args.clean:
                delete_output()

        
        if args.stage == 'all' or args.stage == 'local':
                
                #localization
                #1. get code coverage with and without the filter (2 runs)
                localize.run_code_coverage(path,common.xstr(args.args))

                #2. get the code coverage difference
                localize.run_code_diff(exec_name, pathname)

                #3. run memtrace and profiling
                localize.run_profiling(path, exec_name, args.args, args.debug, args.f_image)

                #4. mem analysis to get the filter function
                localize.run_filter_funcs(exec_name, args.f_image, args.debug, args.debug_level, total_size)

        if args.stage == 'all' or args.stage == 'extract':
                
                #extraction
                #1. run instrace
                extract.run_instrace(path, exec_name, args.args, args.debug, args.in_image)

        if args.stage == 'all' or args.stage == 'extract' or args.stage == 'buildex':
                
                #2. run buildex for expression extraction
                if buildex_opts != '':
                        extract.run_buildex(exec_name, args.in_image, args.out_image, args.debug, args.debug_level, dump, buildex_opts)
                else:
                        extract.run_buildex(exec_name, args.in_image, args.out_image, args.debug, args.debug_level, dump, '')
   
        
if __name__ == '__main__':

        main()
       
