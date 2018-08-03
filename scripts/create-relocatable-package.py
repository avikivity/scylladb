#!/usr/bin/python3

#
# Copyright (C) 2018 ScyllaDB
#

#
# This file is part of Scylla.
#
# Scylla is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Scylla is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
#

import argparse, tarfile, os, os.path, subprocess, collections

Library = collections.namedtuple('Library', ['lib', 'libfile'])

def ldd(executable):
    for ldd_line in subprocess.run(['ldd', executable],
                                   universal_newlines=True,
                                   stdout=subprocess.PIPE).stdout.splitlines():
        elements = ldd_line.split()
        if len(elements) < 3 or elements[1] != '=>':
            continue
        if elements[0].startswith('libc.so'):
            continue
        yield Library(elements[0], os.path.realpath(elements[2]))

ap = argparse.ArgumentParser(description='Create a relocatable scylla package.')
ap.add_argument('dest',
                help='Destination file (tar format)')
ap.add_argument('--mode', dest='mode', default='release',
                help='Build mode (debug/release) to use')

args = ap.parse_args()

executables = ['build/{}/scylla'.format(args.mode),
               'build/{}/iotune'.format(args.mode)]

output = args.dest

libs = set()
for exe in executables:
    libs |= set(ldd(exe))

ar = tarfile.open(output, mode='w')

for exe in executables:
    ar.add(exe, arcname='bin/' + os.path.basename(exe))
for lib in libs:
    ar.add(lib.libfile, arcname='lib/' + lib.lib)
ar.add('conf')
ar.add('dist')



