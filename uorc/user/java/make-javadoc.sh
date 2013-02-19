#!/bin/bash

UPLOADDIR=/tmp/orc-upload
DATE=`date +"%Y%m%d"`

javadoc -sourcepath src -subpackages orc -d $UPLOADDIR/orc-doc

rsync -avzrP $UPLOADDIR/orc-doc/ edo.eecs.umich.edu:/var/www/orcboard.org/javadoc
