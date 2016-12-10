#!/bin/bash

echo "#include <QString>" > $2
echo "" >> $2
echo "const QString OSS_CBTF_ROOT = \"$1\";" >> $2
echo "" >> $2

