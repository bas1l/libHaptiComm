#!/bin/bash

sudo ./build/0_executables/experiment/subitation/subitation --cfg libHaptiComm/4_configuration/configBDev_subitation.cfg --dirdict libHaptiComm/4_configuration/dict/ --direxp results/subitation/ --firstname "$1" --lastname "$2" --seqnumb "$3"
