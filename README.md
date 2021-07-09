# STR-X-v1.1
v1.1
Digital Guitar Amp

CHANGES

//==================

-macOS support (Catalina/Big Sur/M1)

-4x oversampling with minimal-latency, polyphase IIR filters, with the option for offline render-only 4x oversampling with linear-phase FIR filters

-"De-kinked" preamp waveshaping transfer curve for more accurate asymmetrical distortion

-Expanded range of tone controls

-Allow for using v1.0 tone controls with "Use Legacy Tone Controls" option

-Fully resizable GUI (VST3 and AU only)

Utilizes the Class B valve waveshaper as described in Will Pirkle's book, "Designing Audio Effect Plugins in C++: Addendum A19 Vacuum Tube & Distortion Emulation Part 2"
This waveshaper is taken from a patent application (abandoned as a patent) by Mark Poletti: "US 2008/0049950 Nonlinear Processor for Audio Signals"

Additionally, this plugin uses code for converting parameters from knob values to processing values, taken from Will Pirkle's fxobjects header file,
available at https://www.willpirkle.com/Downloads/fxobjects_1.1.zip

Special thanks to Will Pirkle, Joshua Hodges of The Audio Programmer, and the JUCE Team.
