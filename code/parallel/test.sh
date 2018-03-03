#!/bin/bash

clear

make clean
make

time bin/gmpr -c -i bin/Text.txt -o bin/coded
time bin/gmpr -d -i bin/coded -o bin/decoded.txt

#gedit bin/decoded.txt
