# A.P.E.L Scan

Accurate Photon-Emitting Liquid Scan - Senior Design Project for UNLV Spring 2017

## Description
This is the program for the microcontroller of our senior design project, the A.P.E.L. Scan.  The A.P.E.L. Scan is a device that detects and displays the amount of photons emitted by a liquid using a Silicon Photomultiplier (SiPM).  Its intended application is for determining the amount of bacteria present in a blood sample, where the bacteria is stained with a flourescent dye.

It flows a liquid through transparent tubing over the SiPM inside of a light-tight enclosure.  The current pulses emitted by the SiPM are summed and averaged using a transimpedance amplifier and integrator, which produce a DC voltage level.  The DC voltage level is then input to the microcontroller.

The microcontroller determines the amount of photons absorbed by the SiPM with a function depending on the peak voltage level.

## Components used with microcontroller
* **Pushbuttons**: *Start* - to start test and store peak voltage until 'Stop' is pushed.  *Stop* - to end the test and display the result.  *Read Bias* - to read the voltage bias across the SiPM for adjustmet.
* **LCD**: for displaying results, bias, etc.

## Authors

* **Tyler Huddleston** - *Initial work* - [tylerhudd](https://github.com/tylerhudd)

## Built With
* Atmel Studio