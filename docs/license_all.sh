#!/bin/bash

for f in $( find ../ -name "*.hpp" -o -name "*.cpp") ;do
	cat license_header.txt $f > $f.new
  mv $f.new $f
done