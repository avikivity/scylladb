#!/bin/sh -e
export RPMBUILD=`pwd`/build/rpmbuild

CURL_ARGS=()
if [ -n "$JENKINS_URL" ]; then
    CURL_ARGS=("--silent")
fi

do_install()
{
    local pkgs=$(ls $RPMBUILD/RPMS/*/$pkg | grep -v debuginfo)
    sudo yum install -y "${pkgs[@]}" 2> build/err || if [ "`cat build/err`" != "Error: Nothing to do" ]; then cat build/err; exit 1;fi
    echo Install $name done
}

curl() {
    echo curl "${CURL_ARGS[@]}" "$@"
    command curl --location "${CURL_ARGS[@]}" "$@"
}

sudo yum install -y curl yum-utils rpm-build rpmdevtools patch make tar gcc gcc-c++ epel-release
mkdir -p build/srpms
cd build/srpms

if [ ! -f binutils-2.25-15.fc23.src.rpm ]; then
    curl -O https://kojipkgs.fedoraproject.org//packages/binutils/2.25/15.fc23/src/binutils-2.25-15.fc23.src.rpm
fi

if [ ! -f isl-0.14-4.fc23.src.rpm ]; then
    curl -O https://kojipkgs.fedoraproject.org//packages/isl/0.14/4.fc23/src/isl-0.14-4.fc23.src.rpm
fi

if [ ! -f gcc-5.3.1-2.fc23.src.rpm ]; then
    curl -O https://kojipkgs.fedoraproject.org//packages/gcc/5.3.1/2.fc23/src/gcc-5.3.1-2.fc23.src.rpm
fi

if [ ! -f boost-1.58.0-11.fc23.src.rpm ]; then
    curl -O https://kojipkgs.fedoraproject.org//packages/boost/1.58.0/11.fc23/src/boost-1.58.0-11.fc23.src.rpm
fi

if [ ! -f ninja-build-1.6.0-2.fc23.src.rpm ]; then
    curl -O https://kojipkgs.fedoraproject.org//packages/ninja-build/1.6.0/2.fc23/src/ninja-build-1.6.0-2.fc23.src.rpm
fi

if [ ! -f ragel-6.8-5.fc23.src.rpm ]; then
   curl -O https://kojipkgs.fedoraproject.org//packages/ragel/6.8/5.fc23/src/ragel-6.8-5.fc23.src.rpm
fi

if [ ! -f gdb-7.10.1-30.fc23.src.rpm ]; then
   curl -O https://kojipkgs.fedoraproject.org//packages/gdb/7.10.1/30.fc23/src/gdb-7.10.1-30.fc23.src.rpm
fi

if [ ! -f pyparsing-2.0.3-2.fc23.src.rpm ]; then
   curl -O https://kojipkgs.fedoraproject.org//packages/pyparsing/2.0.3/2.fc23/src/pyparsing-2.0.3-2.fc23.src.rpm
fi

cd -

sudo yum install -y python34
sudo ln -sf /usr/bin/python3.4 /usr/bin/python3

if [ ! -f $RPMBUILD/RPMS/noarch/scylla-env-1.0-1.el7.centos.noarch.rpm ]; then
    cd dist/redhat/centos_dep
    tar cpf $RPMBUILD/SOURCES/scylla-env-1.0.tar scylla-env-1.0
    cd -
    yum-builddep -y dist/redhat/centos_dep/scylla-env.spec
    rpmbuild --define "_topdir $RPMBUILD" -ba dist/redhat/centos_dep/scylla-env.spec
fi
do_install scylla-env-1.0-1.el7.centos.noarch.rpm

if [ ! -f $RPMBUILD/RPMS/x86_64/scylla-binutils-2.25-15.el7.centos.x86_64.rpm ]; then
    rpm --define "_topdir $RPMBUILD" -ivh build/srpms/binutils-2.25-15.fc23.src.rpm
    patch $RPMBUILD/SPECS/binutils.spec < dist/redhat/centos_dep/binutils.diff
    yum-builddep -y $RPMBUILD/SPECS/binutils.spec
    rpmbuild --define "_topdir $RPMBUILD" -ba $RPMBUILD/SPECS/binutils.spec
fi
do_install scylla-binutils-2.25-15.el7.centos.x86_64.rpm

if [ ! -f $RPMBUILD/RPMS/x86_64/scylla-isl-0.14-4.el7.centos.x86_64.rpm ]; then
    rpm --define "_topdir $RPMBUILD" -ivh build/srpms/isl-0.14-4.fc23.src.rpm
    patch $RPMBUILD/SPECS/isl.spec < dist/redhat/centos_dep/isl.diff
    yum-builddep -y $RPMBUILD/SPECS/isl.spec
    rpmbuild --define "_topdir $RPMBUILD" -ba $RPMBUILD/SPECS/isl.spec
fi
do_install scylla-isl-0.14-4.el7.centos.x86_64.rpm
do_install scylla-isl-devel-0.14-4.el7.centos.x86_64.rpm

if [ ! -f $RPMBUILD/RPMS/x86_64/scylla-gcc-5.3.1-2.el7.centos.x86_64.rpm ]; then
    rpm --define "_topdir $RPMBUILD" -ivh build/srpms/gcc-5.3.1-2.fc23.src.rpm
    patch $RPMBUILD/SPECS/gcc.spec < dist/redhat/centos_dep/gcc.diff
    yum-builddep -y $RPMBUILD/SPECS/gcc.spec
    rpmbuild --define "_topdir $RPMBUILD" -ba $RPMBUILD/SPECS/gcc.spec
fi
do_install scylla-*5.3.1-2*

if [ ! -f $RPMBUILD/RPMS/x86_64/scylla-boost-1.58.0-11.el7.centos.x86_64.rpm ]; then
    rpm --define "_topdir $RPMBUILD" -ivh build/srpms/boost-1.58.0-11.fc23.src.rpm
    patch $RPMBUILD/SPECS/boost.spec < dist/redhat/centos_dep/boost.diff
    yum-builddep -y $RPMBUILD/SPECS/boost.spec
    rpmbuild --define "_topdir $RPMBUILD" -ba $RPMBUILD/SPECS/boost.spec
fi
do_install scylla-boost*

if [ ! -f $RPMBUILD/RPMS/x86_64/scylla-ninja-build-1.6.0-2.el7.centos.x86_64.rpm ]; then
   rpm --define "_topdir $RPMBUILD" -ivh build/srpms/ninja-build-1.6.0-2.fc23.src.rpm
   patch $RPMBUILD/SPECS/ninja-build.spec < dist/redhat/centos_dep/ninja-build.diff
   yum-builddep -y $RPMBUILD/SPECS/ninja-build.spec
   rpmbuild --define "_topdir $RPMBUILD" -ba $RPMBUILD/SPECS/ninja-build.spec
fi
do_install scylla-ninja-build-1.6.0-2.el7.centos.x86_64.rpm

if [ ! -f $RPMBUILD/RPMS/x86_64/scylla-ragel-6.8-5.el7.centos.x86_64.rpm ]; then
    rpm --define "_topdir $RPMBUILD" -ivh build/srpms/ragel-6.8-5.fc23.src.rpm
    patch $RPMBUILD/SPECS/ragel.spec < dist/redhat/centos_dep/ragel.diff
    yum-builddep -y $RPMBUILD/SPECS/ragel.spec
    rpmbuild --define "_topdir $RPMBUILD" -ba $RPMBUILD/SPECS/ragel.spec
fi
do_install scylla-ragel-6.8-5.el7.centos.x86_64.rpm

if [ ! -f $RPMBUILD/RPMS/x86_64/scylla-gdb-7.10.1-30.el7.centos.x86_64.rpm ]; then
    rpm --define "_topdir $RPMBUILD" -ivh build/srpms/gdb-7.10.1-30.fc23.src.rpm
    patch $RPMBUILD/SPECS/gdb.spec < dist/redhat/centos_dep/gdb.diff
    yum-builddep -y $RPMBUILD/SPECS/gdb.spec
    rpmbuild --define "_topdir $RPMBUILD" -ba $RPMBUILD/SPECS/gdb.spec
fi
do_install scylla-gdb-7.10.1-30.el7.centos.x86_64.rpm

if [ ! -f $RPMBUILD/RPMS/noarch/python34-pyparsing-2.0.3-2.el7.centos.noarch.rpm ]; then
    rpm --define "_topdir $RPMBUILD" -ivh build/srpms/pyparsing-2.0.3-2.fc23.src.rpm
    patch $RPMBUILD/SPECS/pyparsing.spec < dist/redhat/centos_dep/pyparsing.diff
    yum-builddep -y $RPMBUILD/SPECS/pyparsing.spec
    rpmbuild --define "_topdir $RPMBUILD" -ba $RPMBUILD/SPECS/pyparsing.spec
fi
do_install python34-pyparsing-2.0.3-2.el7.centos.noarch.rpm

if [ ! -f $RPMBUILD/RPMS/noarch/scylla-antlr3-tool-3.5.2-1.el7.centos.noarch.rpm ]; then
   mkdir build/scylla-antlr3-tool-3.5.2
   cp dist/redhat/centos_dep/antlr3 build/scylla-antlr3-tool-3.5.2
   cd build/scylla-antlr3-tool-3.5.2
   curl -O http://www.antlr3.org/download/antlr-3.5.2-complete-no-st3.jar
   cd -
   cd build
   tar cJpf $RPMBUILD/SOURCES/scylla-antlr3-tool-3.5.2.tar.xz scylla-antlr3-tool-3.5.2
   cd -
   yum-builddep -y dist/redhat/centos_dep/scylla-antlr3-tool.spec
   rpmbuild --define "_topdir $RPMBUILD" -ba dist/redhat/centos_dep/scylla-antlr3-tool.spec
fi
do_install scylla-antlr3-tool-3.5.2-1.el7.centos.noarch.rpm

if [ ! -f $RPMBUILD/RPMS/x86_64/scylla-antlr3-C++-devel-3.5.2-1.el7.centos.x86_64.rpm ];then
   curl -o build/3.5.2.tar.gz https://github.com/antlr/antlr3/archive/3.5.2.tar.gz
   mv build/3.5.2.tar.gz $RPMBUILD/SOURCES
   yum-builddep -y dist/redhat/centos_dep/scylla-antlr3-C++-devel.spec
   rpmbuild --define "_topdir $RPMBUILD" -ba dist/redhat/centos_dep/scylla-antlr3-C++-devel.spec
fi
do_install scylla-antlr3-C++-devel-3.5.2-1.el7.centos.x86_64.rpm
