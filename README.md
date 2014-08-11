Intelligent Motor Controller Simple Axis
=====

This repository contains a simple implementation of the Intelligent Motor Control (IMC) Axis module, which runs the motor for one axis on a 3D printer, and interfaces with other Axis nodes and the Master node.

The Master code can be found at https://github.com/BenW0/IMC_Master_Marlin

A much longer description of the project can be found in IMC Framework.pdf.

# Target Platform

The Axis code runs on a Teensy 3.1 from PJRC.com. See pcb for a schematic of the development board.

# Building

This code compiles using a Makefile. At the top of the Makefile are two paths that need to be set. First, download Teensyduino from PJRC.com. Configure TEENSY_PATH to point to the root folder of the Arduino install. Set VENDOR to point to the header files needed to compile Teensy code, which are available from https://github.com/matthewSorensen/minimal-k20-env. Note that this code compiles to C, not C++ like Arduino/Teensyduino programs. 
