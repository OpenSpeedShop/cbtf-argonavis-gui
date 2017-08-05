#!/bin/bash
set -x

export INSTALL_ROOT=/opt/OSS/new_gui
export KRELL_ROOT_MRNET=/opt/OSS/krellroot_v2.3.1
export KRELL_ROOT_XERCES=/opt/OSS/krellroot_v2.3.1
export CBTF_ROOT=/opt/OSS/cbtf_only_v2.3.1
export CBTF_KRELL_ROOT=/opt/OSS/cbtf_krell_only_v2.3.1
export CBTF_ARGONAVIS_ROOT=/opt/OSS/cbtf_argonavis_only_v2.3.1
export OSS_CBTF_ROOT=/opt/OSS/osscbtf_only_v2.3.1
export GRAPHVIZ_ROOT=/opt/OSS/graphviz-2.41.0
export QTGRAPHLIB_ROOT=/opt/OSS/QtGraph-1.0.0
#export QTGRAPHLIB_ROOT=/opt/OSS/QtGraphCleanLibs

#export BOOST_ROOT=/opt/OSS/krellroot_v2.3.1
export BOOST_ROOT=/usr

export QTDIR=/usr/lib64/qt4

#export PATH=$QTDIR/bin:$PATH
export LD_LIBRARY_PATH=/opt/OSS/osscbtf_only_v2.3.1/lib64:/opt/OSS/cbtf_only_v2.3.1/lib64:/opt/OSS/cbtf_krell_only_v2.3.1/lib64:/opt/OSS/cbtf_argonavis_only_v2.3.1/lib64:/opt/OSS/krellroot_v2.3.1/lib:/opt/OSS/krellroot_v2.3.1/lib64:$LD_LIBRARY_PATH

make clean
${QTDIR}/bin/qmake -o Makefile openss-gui.pro
make 
make install


