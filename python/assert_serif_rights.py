#!/bin/env python

# Copyright 2018 by BBN Technologies Corp.
# All Rights Reserved.

"""
This module creates a copy of a SERIF directory or source file, with
copyrights updated and commercial code removed (unless otherwise
specified).  It also removes .vssscc and .dsp files, plus a few other
strays.

It provides the following fuctions:

    - copy_and_cleanup_source_dir()
    - copy_and_cleanup_source_file()

"""

######################################################################
DEFAULT_COPYRIGHT_NOTICE = '''\
Copyright 2018 by Raytheon BBN Technologies Corp.

Use, duplication, or disclosure by the Government is subject to
restrictions as set forth in the Rights in Noncommercial Computer
Software and Noncommercial Computer Software Documentation clause at
DFARS 252.227-7014.

     Raytheon BBN Technologies Corp.
     10 Moulton St.
     Cambridge, MA 02138
     617-873-3411
'''
COE_COPYRIGHT_NOTICE = '''\
Copyright 2018 by Raytheon BBN Technologies Corp. All Rights Reserved.

This Software is licensed to the COE in accordance with license
#A09248-BBN-SLA and all applicable amendments.

     Raytheon BBN Technologies Corp.
     10 Moulton St.
     Cambridge, MA 02138
     617-873-3411
'''
CONTRACT_4100426631_COPYRIGHT_NOTICE = '''\
Copyright 2018 Raytheon BBN Technologies Corp.

Protect in accordance with Contract No. 4100426631 executed by the
Parties on June 20, 2012.  All Rights Reserved.

     Raytheon BBN Technologies Corp.
     10 Moulton St.
     Cambridge, MA 02138
     617-873-3411
'''
GENERIC_CONTRACT_COPYRIGHT_NOTICE = '''\
Copyright 2018 Raytheon BBN Technologies Corp.

Protect in accordance with Contract No. %(contract_num)s fully-executed
by %(contract_parties)s on %(contract_date)s.
All Rights Reserved.

     Raytheon BBN Technologies Corp.
     10 Moulton St.
     Cambridge, MA 02138
     617-873-3411
'''
COMPUTABLE_INSIGHTS_LLC_COPYRIGHT_NOTICE = '''\
Copyright 2018 by Raytheon BBN Technologies Corp. All Rights Reserved.

This Software is licensed to Computable Insights LLC in accordance with
license #A13088-BBN-SLA and all applicable amendments.

     Raytheon BBN Technologies Corp.
     10 Moulton St.
     Cambridge, MA 02138
     617-873-3411
'''
SIMPLE_COPYRIGHT_NOTICE = '''\
Copyright 2018 Raytheon BBN Technologies Corp. All Rights Reserved.
'''
EAR99_COPYRIGHT_NOTICE = '''\
Copyright 2018 Raytheon BBN Technologies Corp. All Rights Reserved.

This document contains information whose export or disclosure to 
Non U.S. Persons, wherever located, is subject to the 
Export Administration Regulations (EAR) (15 C.F.R. Sections 730-774).

Specifically, Raytheon BBN Technologies conducted an internal review 
and determined this information is export controlled as EAR99. 

Exports contrary to U.S. law are prohibited.
'''
######################################################################

"""
If any of the following substrings appears in an input path, then
we will not report copyright warnings for that file.  This is used
to supress warnings for code that's distributed with Serif, but
was not written by BBN.
"""
EXCLUDE_COPYRIGHT_CHECKS = [
    '/expts/Serif/scoring/XML/',
    '/expts/Serif/scoring/ace05-eval-v14.pl',
    '/expts/Serif/scoring/ace07-eval-v01.pl',
    '/expts/Serif/scoring/ace08-eval-v08.pl',
    '/expts/Serif/python/compat.py',
    ]

######################################################################

import os, re, shutil
allow_commercial_code = False

def get_script_copyright_notice(copyright_notice):
    div = '#'*75+'\n'
    body = ''.join('# %-71s #\n' % line for line in
                   copyright_notice.split('\n'))
    return div+body+div+'\n'

def get_c_copyright_notice(copyright_notice):
    div = '/*' + (73*'*') + '*/\n'
    body = ''.join('/* %-71s */\n' % line for line in
                   copyright_notice.split('\n'))
    return div+body+div+'\n'

############### Script ##################

def copy_and_cleanup_source_dir(src, dst, copyright_notice=DEFAULT_COPYRIGHT_NOTICE, verbose=False, extensions=None):
    """
    @return: A list of warnings.
    """
    warnings = []
    if not os.path.exists(src):
        raise ValueError('Directory %r not found!' % src)
    if not os.path.exists(dst):
        os.makedirs(dst)
    for item in os.listdir(src):
        item_src_path = os.path.join(src, item)
        item_dst_path = os.path.join(dst, item)
        if os.path.isdir(item_src_path):
            if re.search('\.svn|\.dir|Release$|Debug$|CMakeFiles', item):
                if verbose:
                    print "Skipping %s" % item_src_path
                continue
            else:
                warnings += copy_and_cleanup_source_dir(
                    item_src_path, item_dst_path, copyright_notice, extensions=extensions)
        elif os.path.isfile(item_src_path):
            extension = None
            if item_src_path.rfind(".") != -1:
                extension = item_src_path[item_src_path.rfind("."):]

            if re.search('\.vssscc|\.vspscc|\.vsscc|\.dsp$|\.ncb$|^Changelog\.xls$|^fix_vcproj_guids\.py$', item):
                if verbose:
                    print "Skipping %s" % item_src_path
            elif re.search('\.vcproj|\.sln|\.class|\.doc|\.par', item) or (extensions and extension not in extensions):   
                if verbose:
                    print "Copying %s" % item_src_path
                shutil.copyfile(item_src_path, item_dst_path)
            else:
                if verbose:
                    print "Copying and cleaning up %s" % item_src_path
                warnings += copy_and_cleanup_source_file(
                    item_src_path, item_dst_path, copyright_notice)
    return warnings

def copy_and_cleanup_source_file(infile, outfile, copyright_notice=DEFAULT_COPYRIGHT_NOTICE):
    """
    @return: A list of warnings.
    """
    warnings = []
    has_copyright = False
    has_rights = False
    commercial_if = 0
    if_count = 0
    exclude_copyright_checks = any(p in infile
                                   for p in EXCLUDE_COPYRIGHT_CHECKS)
    s = open(infile, 'r')
    out = open(outfile, 'w')
    script_copyright_notice = get_script_copyright_notice(copyright_notice)
    c_copyright_notice = get_c_copyright_notice(copyright_notice)
    copyright_one_liner = re.search('Copyright.*', copyright_notice).group(0)

    for line in s:
        if re.search('^\s*\#if', line):
            if_count += 1
        if re.search('COMMERCIAL_AND_DEVELOPED_EXCLUSIVELY_UNDER_PRIVATE_FUNDING', line) and not allow_commercial_code:
            commercial_if = if_count
            print "Ommitting commercial code: %s" % infile
        if (commercial_if == 0):
            if re.search('^\s*\# Copyright.*BBN', line):
                out.write(script_copyright_notice)
                line = ""
                has_copyright = True
                if re.search('all rights reserved', script_copyright_notice, re.I):
                    has_rights = True
            elif re.match(r'^\s*print [\'"]Copyright[^\'"]+BBN[^\'"]*[\'"]\s*$', line):
                line = re.sub(r'^(\s*print )([\'"]Copyright[^\'"]+BBN[^\'"]*[\'"])(\s*)', r'\1"%s"\3' % copyright_one_liner, line)
            elif re.search(r'HttpResponse', line):
                pass
            elif re.search('Copyright.*BBN', line) and not re.search('\#define', line):
                out.write(c_copyright_notice)
                line = ""
                has_copyright = True
                if re.search('all rights reserved', c_copyright_notice, re.I):
                    has_rights = True
            else:
                if re.search('Copyright', line) and not re.search('displayVersionAndCopyright', line) and not re.search('\.cmake$|rijndael|\.c$|\.h$', infile) and not exclude_copyright_checks:
                    warnings.append("copyright mentioned in unexpected setting (%r): %s\n" % (infile, line))
            out.write(line)
        if re.search('all rights reserved', line, re.I):
            has_rights = True
        if re.search('^\s*\#endif', line):
            if commercial_if == if_count:
                commercial_if = 0
            if_count -= 1
    if not exclude_copyright_checks:
        if has_copyright and not has_rights:
            warnings.append("Copyright but no rights reserved: %s\n" % infile)
        if not has_copyright:
            warnings.append("No copyright: %s\n" % infile)

    out.close()
    s.close()
    return warnings

if __name__ == '__main__':
    copy_and_cleanup_source_dir('', '')
