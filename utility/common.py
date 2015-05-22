def get_executable_name(path):

        splits = path.split('\\')
        return splits[-1]

def xstr(s):
    if s is None:
        return ''
    else:
        return str(s)

def boolstr(b):
        if b:
                return '1'
        else:
                return '0'
